package com.example.ebrail;

import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

// Helper utility to request and check runtime permissions for BLE on Android 12+ or older devices
public class PermissionUtil {
    public static final int PERM_REQUEST_CODE = 1001;

    // Return the set of runtime permissions appropriate for the current SDK
    public static String[] getRequiredPermissions() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) { // Android 12+
            return new String[]{
                    android.Manifest.permission.BLUETOOTH_SCAN,
                    android.Manifest.permission.BLUETOOTH_CONNECT,
                    android.Manifest.permission.ACCESS_FINE_LOCATION
            };
        } else {
            // Older devices used location to discover BLE devices
            return new String[]{
                    android.Manifest.permission.ACCESS_FINE_LOCATION
            };
        }
    }

    public static boolean hasRequiredPermissions(Context ctx) {
        String[] perms = getRequiredPermissions();
        for (String p : perms) {
            if (ContextCompat.checkSelfPermission(ctx, p) != PackageManager.PERMISSION_GRANTED) {
                return false;
            }
        }
        return true;
    }

    public static void requestRequiredPermissions(Activity activity) {
        ActivityCompat.requestPermissions(activity, getRequiredPermissions(), PERM_REQUEST_CODE);
    }

    public static boolean permissionsGranted(int[] grantResults) {
        if (grantResults == null || grantResults.length == 0) return false;
        for (int r : grantResults) if (r != PackageManager.PERMISSION_GRANTED) return false;
        return true;
    }
}
