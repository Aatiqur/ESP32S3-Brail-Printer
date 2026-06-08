package com.example.ebrail;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

// Central BLE helper that performs scanning, connecting, service discovery and writes
@SuppressWarnings("deprecation")
public class BleManager {
    private static final String TAG = "BleManager";

    public interface Listener {
        void onDeviceDiscovered(Device device);
        void onScanStateChanged(boolean scanning);
        void onConnectionStateChanged(String address, int newState);
        void onServicesDiscovered(String address, List<UUID> services);
        void onCharacteristicWrite(String address, UUID charUuid, byte[] value);
        void onCharacteristicChanged(String address, UUID charUuid, byte[] value);
    }

    private final Context context;
    private final BluetoothAdapter bluetoothAdapter;
    private final BluetoothLeScanner scanner;
    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private BluetoothGatt gatt;
    private BluetoothGattCharacteristic writeCharacteristic;
    private BluetoothGattCharacteristic notifyCharacteristic;
    private Listener listener;
    private boolean scanning = false;

    // store the last payload written (simple approach to avoid calling deprecated getValue())
    private byte[] lastWritePayload;

    // scan timeout (ms)
    private static final long SCAN_PERIOD_MS = 10_000L;
    private final Runnable scanStopRunnable = new Runnable() {
        @Override
        public void run() {
            stopScan();
        }
    };

    public BleManager(Context context) {
        this.context = context.getApplicationContext();
        BluetoothManager mgr = (BluetoothManager) context.getSystemService(Context.BLUETOOTH_SERVICE);
        bluetoothAdapter = mgr.getAdapter();
        scanner = bluetoothAdapter != null ? bluetoothAdapter.getBluetoothLeScanner() : null;
    }

    public void setListener(Listener listener) {
        this.listener = listener;
    }

    public boolean startScan() {
        if (scanner == null) return false;
        if (scanning) {
            // restart timeout
            mainHandler.removeCallbacks(scanStopRunnable);
            mainHandler.postDelayed(scanStopRunnable, SCAN_PERIOD_MS);
            return true;
        }

        ScanSettings settings = new ScanSettings.Builder()
                .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
                .build();
        
        scanner.startScan(null, settings, scanCallback);
        scanning = true;
        // schedule automatic stop
        mainHandler.postDelayed(scanStopRunnable, SCAN_PERIOD_MS);
        if (listener != null) mainHandler.post(() -> listener.onScanStateChanged(true));
        return true;
    }

    public void stopScan() {
        if (scanner == null) return;
        if (!scanning) return;
        scanner.stopScan(scanCallback);
        scanning = false;
        // cancel pending stop runnable
        mainHandler.removeCallbacks(scanStopRunnable);
        if (listener != null) mainHandler.post(() -> listener.onScanStateChanged(false));
    }

    private final ScanCallback scanCallback = new ScanCallback() {
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            super.onScanResult(callbackType, result);
            String name = null;
            
            // 1. Try Scan Record (most reliable for BLE local names)
            if (result.getScanRecord() != null) {
                name = result.getScanRecord().getDeviceName();
            }
            
            // 2. Try BluetoothDevice getName()
            if (name == null || name.isEmpty()) {
                try {
                    name = result.getDevice().getName();
                } catch (SecurityException e) {
                    // Ignore
                }
            }
            
            // 3. Last resort - keep it null so the UI can decide what to show (e.g. Unknown)
            // but we'll use a placeholder for the Device object if still null
            String displayName = (name != null && !name.isEmpty()) ? name : "Unknown Device";
            
            Device d = new Device(result.getDevice(), displayName, result.getRssi());
            if (listener != null) mainHandler.post(() -> listener.onDeviceDiscovered(d));
        }
    };

    public void connect(BluetoothDevice device) {
        if (device == null) return;
        // stop scanning while connecting
        stopScan();
        // close previous
        close();
        gatt = device.connectGatt(context, false, gattCallback);
    }

    public void disconnect() {
        if (gatt != null) {
            gatt.disconnect();
        }
    }

    public boolean isConnected() {
        return gatt != null && writeCharacteristic != null;
    }

    public boolean write(byte[] payload) {
        if (gatt == null || writeCharacteristic == null) return false;
        
        new Thread(() -> {
            int offset = 0;
            int chunkSize = 20; // standard BLE MTU payload size
            while (offset < payload.length) {
                int length = Math.min(payload.length - offset, chunkSize);
                byte[] chunk = new byte[length];
                System.arraycopy(payload, offset, chunk, 0, length);
                
                lastWritePayload = chunk;
                writeCharacteristic.setValue(chunk);
                boolean ok = gatt.writeCharacteristic(writeCharacteristic);
                Log.d(TAG, "write chunk queued: " + ok);
                
                // Wait briefly before sending the next chunk to avoid overflowing the BLE queue
                try {
                    Thread.sleep(50);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                offset += length;
            }
        }).start();

        return true;
    }

    public void close() {
        if (gatt != null) {
            gatt.close();
            gatt = null;
            writeCharacteristic = null;
            notifyCharacteristic = null;
        }
    }

    private final BluetoothGattCallback gattCallback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt gattLocal, int status, int newState) {
            super.onConnectionStateChange(gattLocal, status, newState);
            Log.d(TAG, "onConnectionStateChange: " + newState + " status=" + status);
            if (newState == android.bluetooth.BluetoothProfile.STATE_CONNECTED) {
                BleManager.this.gatt = gattLocal;
                gattLocal.discoverServices();
                if (listener != null) mainHandler.post(() -> listener.onConnectionStateChanged(gattLocal.getDevice().getAddress(), newState));
            } else if (newState == android.bluetooth.BluetoothProfile.STATE_DISCONNECTED) {
                if (listener != null) mainHandler.post(() -> listener.onConnectionStateChanged(gattLocal.getDevice().getAddress(), newState));
                // cleanup
                close();
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gattLocal, int status) {
            super.onServicesDiscovered(gattLocal, status);
            Log.d(TAG, "onServicesDiscovered status=" + status);
            if (status != BluetoothGatt.GATT_SUCCESS) return;
            
            UUID UART_SERVICE_UUID = UUID.fromString("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
            UUID CHAR_RX_UUID = UUID.fromString("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
            UUID CHAR_TX_UUID = UUID.fromString("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

            BluetoothGattService uartService = gattLocal.getService(UART_SERVICE_UUID);
            if (uartService != null) {
                Log.d(TAG, "UART Service found!");
                writeCharacteristic = uartService.getCharacteristic(CHAR_RX_UUID);
                notifyCharacteristic = uartService.getCharacteristic(CHAR_TX_UUID);
                
                if (writeCharacteristic != null) {
                    Log.d(TAG, "RX Characteristic found!");
                    writeCharacteristic.setWriteType(BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE);
                }
                
                if (notifyCharacteristic != null) {
                    Log.d(TAG, "TX Characteristic found! Setting up notifications...");
                    gattLocal.setCharacteristicNotification(notifyCharacteristic, true);
                    BluetoothGattDescriptor descriptor = notifyCharacteristic.getDescriptor(UUID.fromString("00002902-0000-1000-8000-00805f9b34fb"));
                    if (descriptor != null) {
                        descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
                        gattLocal.writeDescriptor(descriptor);
                    }
                }
            } else {
                Log.e(TAG, "UART Service NOT found!");
            }

            List<UUID> services = new ArrayList<>();
            if (uartService != null) services.add(UART_SERVICE_UUID);
            
            if (listener != null) mainHandler.post(() -> listener.onServicesDiscovered(gattLocal.getDevice().getAddress(), services));
        }

        @Override
        public void onCharacteristicWrite(BluetoothGatt gattLocal, BluetoothGattCharacteristic characteristic, int status) {
            super.onCharacteristicWrite(gattLocal, characteristic, status);
            byte[] val = lastWritePayload != null ? lastWritePayload : new byte[0];
            if (listener != null) mainHandler.post(() -> listener.onCharacteristicWrite(gattLocal.getDevice().getAddress(), characteristic.getUuid(), val));
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gattLocal, BluetoothGattCharacteristic characteristic) {
            super.onCharacteristicChanged(gattLocal, characteristic);
            byte[] val = characteristic.getValue();
            if (listener != null) mainHandler.post(() -> listener.onCharacteristicChanged(gattLocal.getDevice().getAddress(), characteristic.getUuid(), val));
        }
    };
}
