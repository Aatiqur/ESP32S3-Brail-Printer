package com.example.ebrail;

import android.content.Intent;
import android.content.res.ColorStateList;
import android.graphics.Color;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;

import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.UUID;

public class MainActivity extends AppCompatActivity {

    private BleManager bleManager;
    private TextView tvConnected;
    private ImageView ivConnectionStatus;
    private EditText etMessage;
    private TextView tvBraillePreview;
    private Button btnSend;
    private Spinner spinnerLang;
    private LinearLayout layoutProgress;
    private ProgressBar pbPrintProgress;
    private TextView tvProgressLabel;
    private String pendingAddress;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        WindowCompat.setDecorFitsSystemWindows(getWindow(), false);
        WindowInsetsControllerCompat windowInsetsController =
                WindowCompat.getInsetsController(getWindow(), getWindow().getDecorView());
        if (windowInsetsController != null) {
            windowInsetsController.hide(WindowInsetsCompat.Type.systemBars());
            windowInsetsController.setSystemBarsBehavior(
                    WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
        }

        EdgeToEdge.enable(this);
        setContentView(R.layout.activity_main);
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });

        tvConnected = findViewById(R.id.tv_connected_device);
        ivConnectionStatus = findViewById(R.id.iv_connection_status);
        etMessage = findViewById(R.id.et_message);
        tvBraillePreview = findViewById(R.id.tv_braille_preview);
        btnSend = findViewById(R.id.btn_send);
        spinnerLang = findViewById(R.id.spinner_lang);
        layoutProgress = findViewById(R.id.layout_progress);
        pbPrintProgress = findViewById(R.id.pb_print_progress);
        tvProgressLabel = findViewById(R.id.tv_progress_label);

        ArrayAdapter<String> langAdapter = new ArrayAdapter<>(this, android.R.layout.simple_spinner_item, new String[]{"Bengali", "English"});
        langAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinnerLang.setAdapter(langAdapter);

        // Real-time braille translation preview
        etMessage.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {}

            @Override
            public void afterTextChanged(Editable s) {
                String input = s.toString();
                if (input.isEmpty()) {
                    tvBraillePreview.setText("Brail text");
                    return;
                }
                
                List<List<String>> brailleLines = BrailleParser.translateToBrailleCells(input);
                StringBuilder preview = new StringBuilder();
                for (List<String> line : brailleLines) {
                    for (String cell : line) {
                        preview.append(cell);
                    }
                    preview.append("\n");
                }
                tvBraillePreview.setText(preview.toString().trim());
            }
        });

        bleManager = new BleManager(this);
        bleManager.setListener(new BleManager.Listener() {
            @Override
            public void onDeviceDiscovered(Device device) {}

            @Override
            public void onScanStateChanged(boolean scanning) {}

            @Override
            public void onConnectionStateChanged(String address, int newState) {
                runOnUiThread(() -> {
                    if (newState == android.bluetooth.BluetoothProfile.STATE_CONNECTED) {
                        tvConnected.setText("Connected: " + address);
                        ivConnectionStatus.setImageTintList(ColorStateList.valueOf(Color.parseColor("#4CAF50"))); // Green
                        toast("Connected: " + address);
                    } else if (newState == android.bluetooth.BluetoothProfile.STATE_DISCONNECTED) {
                        tvConnected.setText("Disconnected");
                        ivConnectionStatus.setImageTintList(ColorStateList.valueOf(Color.parseColor("#F44336"))); // Red
                        toast("Disconnected: " + address);
                    }
                });
            }

            @Override
            public void onServicesDiscovered(String address, List<UUID> services) {}

            @Override
            public void onCharacteristicWrite(String address, UUID charUuid, byte[] value) {}

            @Override
            public void onCharacteristicChanged(String address, UUID charUuid, byte[] value) {
                String msg = new String(value, StandardCharsets.UTF_8).trim();
                if (msg.startsWith("PROG:")) {
                    String progStr = msg.substring(5);
                    runOnUiThread(() -> {
                        if (progStr.equals("DONE")) {
                            tvProgressLabel.setText("Print Complete!");
                            pbPrintProgress.setProgress(pbPrintProgress.getMax());
                            pbPrintProgress.postDelayed(() -> layoutProgress.setVisibility(View.GONE), 3000);
                        } else if (progStr.contains("/")) {
                            String[] parts = progStr.split("/");
                            if (parts.length == 2) {
                                try {
                                    int current = Integer.parseInt(parts[0]);
                                    int total = Integer.parseInt(parts[1]);
                                    pbPrintProgress.setMax(total);
                                    pbPrintProgress.setProgress(current);
                                    tvProgressLabel.setText("Printing... " + current + "/" + total + " Dots");
                                } catch (NumberFormatException e) {
                                    e.printStackTrace();
                                }
                            }
                        }
                    });
                }
            }
        });

        btnSend.setOnClickListener(v -> {
            String text = etMessage.getText().toString();
            if (text.isEmpty()) {
                Toast.makeText(MainActivity.this, "Enter text first", Toast.LENGTH_SHORT).show();
                return;
            }

            // Generate Braille output cells from Java Parser
            List<List<String>> brailleCells = BrailleParser.translateToBrailleCells(text);
            
            // Convert to Arduino Commands
            String commands = BrailleParser.translateToCommands(brailleCells) + "\n";
            
            // Show progress UI
            layoutProgress.setVisibility(View.VISIBLE);
            tvProgressLabel.setText("Sending data to printer...");
            pbPrintProgress.setProgress(0);
            pbPrintProgress.setMax(100);

            // Send via BLE
            if (bleManager != null && bleManager.isConnected()) {
                boolean ok = bleManager.write(commands.getBytes(StandardCharsets.UTF_8));
                if (ok) {
                    Toast.makeText(MainActivity.this, "Sending...", Toast.LENGTH_SHORT).show();
                } else {
                    Toast.makeText(MainActivity.this, "Failed to send", Toast.LENGTH_SHORT).show();
                    layoutProgress.setVisibility(View.GONE);
                }
            } else {
                Toast.makeText(MainActivity.this, "Not connected to a device", Toast.LENGTH_SHORT).show();
                layoutProgress.setVisibility(View.GONE);
            }
        });

        String address = getIntent().getStringExtra("device_address");
        if (address != null) {
            if (!PermissionUtil.hasRequiredPermissions(this)) {
                pendingAddress = address;
                PermissionUtil.requestRequiredPermissions(this);
            } else {
                connectToAddress(address);
            }
        }
    }

    private void connectToAddress(String address) {
        try {
            android.bluetooth.BluetoothManager bm = (android.bluetooth.BluetoothManager) getSystemService(BLUETOOTH_SERVICE);
            android.bluetooth.BluetoothAdapter adapter = bm.getAdapter();
            android.bluetooth.BluetoothDevice dev = adapter.getRemoteDevice(address);
            bleManager.connect(dev);
        } catch (IllegalArgumentException e) {
            toast("Invalid device address: " + address);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == PermissionUtil.PERM_REQUEST_CODE) {
            if (PermissionUtil.permissionsGranted(grantResults)) {
                if (pendingAddress != null) {
                    connectToAddress(pendingAddress);
                    pendingAddress = null;
                }
            } else {
                new androidx.appcompat.app.AlertDialog.Builder(this)
                        .setTitle("Permissions Required")
                        .setMessage("Bluetooth and Location permissions are required to connect to BLE devices. Please grant them to continue.")
                        .setPositiveButton("OK", null)
                        .show();
            }
        }
    }

    private void toast(String s) {
        Toast.makeText(this, s, Toast.LENGTH_SHORT).show();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (bleManager != null) bleManager.close();
    }
}