package com.example.ebrail;

import android.bluetooth.BluetoothDevice;

// Simple model for a BLE device entry shown in the scan list
public class Device {
    private final BluetoothDevice bluetoothDevice;
    private final String name;
    private final int rssi;

    public Device(BluetoothDevice bluetoothDevice, String name, int rssi) {
        this.bluetoothDevice = bluetoothDevice;
        this.name = name != null && !name.isEmpty() ? name : "Unknown";
        this.rssi = rssi;
    }

    public String getName() {
        return name;
    }

    public String getAddress() {
        return bluetoothDevice.getAddress();
    }

    public int getRssi() {
        return rssi;
    }

    public BluetoothDevice getBluetoothDevice() {
        return bluetoothDevice;
    }
}

