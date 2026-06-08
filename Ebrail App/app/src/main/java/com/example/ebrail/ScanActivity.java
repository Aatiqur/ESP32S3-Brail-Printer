package com.example.ebrail;

import android.bluetooth.BluetoothDevice;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import java.util.ArrayList;
import java.util.List;

// Activity that scans for BLE devices and allows connecting to the selected device
public class ScanActivity extends AppCompatActivity implements DeviceAdapter.Listener {

    private BleManager bleManager;
    private DeviceAdapter adapter;
    private final List<Device> deviceList = new ArrayList<>();

    private TextView tvStatus;
    private ProgressBar progressScan;
    private Button btnScanToggle;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        WindowCompat.setDecorFitsSystemWindows(getWindow(), false);
        WindowInsetsControllerCompat windowInsetsController =
                WindowCompat.getInsetsController(getWindow(), getWindow().getDecorView());
        if (windowInsetsController != null) {
            windowInsetsController.hide(WindowInsetsCompat.Type.systemBars());
            windowInsetsController.setSystemBarsBehavior(
                    WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
        }

        setContentView(R.layout.activity_scan);

        tvStatus = findViewById(R.id.tv_scan_status);
        progressScan = findViewById(R.id.progress_scan);
        btnScanToggle = findViewById(R.id.btn_scan_toggle);

        RecyclerView rv = findViewById(R.id.rv_devices);
        rv.setLayoutManager(new LinearLayoutManager(this));
        adapter = new DeviceAdapter(this, this);
        rv.setAdapter(adapter);

        bleManager = new BleManager(this);
        bleManager.setListener(new BleManager.Listener() {
            @Override
            public void onDeviceDiscovered(Device device) {
                // Skip devices without names to avoid cluttering the list
                if (device.getName().equalsIgnoreCase("Unknown Device") || 
                    device.getName().equalsIgnoreCase("Unknown")) {
                    return;
                }

                // dedupe by address
                boolean found = false;
                for (int i = 0; i < deviceList.size(); i++) {
                    Device d = deviceList.get(i);
                    if (d.getAddress().equals(device.getAddress())) { 
                        found = true; 
                        // update name if previously unknown
                        if (d.getName().equals("Unknown Device") && !device.getName().equals("Unknown Device")) {
                            deviceList.set(i, device);
                            runOnUiThread(() -> adapter.updateDevices(deviceList));
                        }
                        break; 
                    }
                }
                if (!found) {
                    deviceList.add(device);
                    runOnUiThread(() -> adapter.updateDevices(deviceList));
                }
            }

            @Override
            public void onScanStateChanged(boolean scanning) {
                runOnUiThread(() -> {
                    tvStatus.setText(scanning ? "Scanning..." : "Idle");
                    progressScan.setVisibility(scanning ? View.VISIBLE : View.GONE);
                    btnScanToggle.setText(scanning ? "Stop" : "Scan");
                });
            }

            @Override
            public void onConnectionStateChanged(String address, int newState) {
                // forward to main activity by starting it when connected
                if (newState == android.bluetooth.BluetoothProfile.STATE_CONNECTED) {
                    Intent it = new Intent(ScanActivity.this, MainActivity.class);
                    it.putExtra("device_address", address);
                    startActivity(it);
                    finish();
                }
            }

            @Override
            public void onServicesDiscovered(String address, List<java.util.UUID> services) {
                // no-op here
            }

            @Override
            public void onCharacteristicWrite(String address, java.util.UUID charUuid, byte[] value) {
                // no-op
            }

            @Override
            public void onCharacteristicChanged(String address, java.util.UUID charUuid, byte[] value) {
                // no-op
            }
        });

        // wire scan toggle button
        btnScanToggle.setOnClickListener(v -> {
            if (!PermissionUtil.hasRequiredPermissions(ScanActivity.this)) {
                PermissionUtil.requestRequiredPermissions(ScanActivity.this);
                return;
            }
            // toggle
            if (isScanning()) {
                bleManager.stopScan();
            } else {
                deviceList.clear();
                adapter.updateDevices(deviceList);
                boolean started = bleManager.startScan();
                if (!started) tvStatus.setText("Unable to start scan");
            }
        });

        // If permissions already granted, optionally start scanning
        if (PermissionUtil.hasRequiredPermissions(this)) {
            // leave idle until user taps Scan
        } else {
            // request permissions proactively
            PermissionUtil.requestRequiredPermissions(this);
        }
    }

    private boolean isScanning() {
        // reflect scan state from UI text or rely on BleManager internal state (no getter), so check button text
        return "Stop".equals(btnScanToggle.getText().toString());
    }

    public void onScanToggleClicked(View v) {
        // kept for compatibility with XML onClick if used
        btnScanToggle.performClick();
    }

    @Override
    public void onConnectClicked(Device device) {
        // Connect to the device
        bleManager.connect(device.getBluetoothDevice());
        tvStatus.setText("Connecting to " + device.getName());
    }

    @Override
    public void onItemClicked(Device device) {
        // same as connect for now
        onConnectClicked(device);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == PermissionUtil.PERM_REQUEST_CODE) {
            if (PermissionUtil.permissionsGranted(grantResults)) {
                Toast.makeText(this, "Permissions granted", Toast.LENGTH_SHORT).show();
            } else {
                new androidx.appcompat.app.AlertDialog.Builder(this)
                        .setTitle("Permissions Required")
                        .setMessage("Bluetooth and Location permissions are required to discover nearby devices. Please grant them to continue.")
                        .setPositiveButton("OK", null)
                        .show();
            }
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (bleManager != null) {
            bleManager.stopScan();
            bleManager.close();
        }
    }
}
