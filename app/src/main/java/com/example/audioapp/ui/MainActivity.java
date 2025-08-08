package com.example.audioapp.ui;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import android.Manifest;
import android.app.AlertDialog;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.RadioGroup;
import android.widget.Toast;
import com.example.audioapp.R;
import com.example.audioapp.audio.AudioBuffer;
import com.example.audioapp.audio.AudioEngine;
import com.example.audioapp.audio.AudioFileLoader;
import com.example.audioapp.avst.AvstHost;
import com.example.audioapp.avst.Plugin;
import com.example.audioapp.engine.ProcessingModeDetector;

public class MainActivity extends AppCompatActivity {

    private static final int AUDIO_PERMISSION_REQUEST_CODE = 1;
    private static final int FILE_PICKER_REQUEST_CODE = 2;

    private ParametricEQView parametricEQView;
    private CompressorView compressorView;
    private AvstHost avstHost;
    private Plugin eqPlugin;
    private Plugin compressorPlugin;
    private ProcessingModeDetector processingModeDetector;

    // Used to load the 'audioapp' library on application startup.
    static {
        System.loadLibrary("audioapp");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        parametricEQView = findViewById(R.id.parametric_eq_view);
        compressorView = findViewById(R.id.compressor_view);
        avstHost = new AvstHost();
        processingModeDetector = new ProcessingModeDetector();

        Button loadFileButton = findViewById(R.id.button_load_file);
        loadFileButton.setOnClickListener(v -> {
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("audio/wav");
            startActivityForResult(intent, FILE_PICKER_REQUEST_CODE);
        });

        Button playButton = findViewById(R.id.button_play);
        playButton.setOnClickListener(v -> AudioEngine.native_setPlaying(true));

        Button pauseButton = findViewById(R.id.button_pause);
        pauseButton.setOnClickListener(v -> AudioEngine.native_setPlaying(false));

        Button stopButton = findViewById(R.id.button_stop);
        stopButton.setOnClickListener(v -> {
            AudioEngine.native_setPlaying(false);
            AudioEngine.native_setPlaybackPosition(0);
        });

        Button exportButton = findViewById(R.id.button_export);
        exportButton.setOnClickListener(v -> showExportDialog());

        Button switchPluginButton = findViewById(R.id.button_switch_plugin);
        switchPluginButton.setOnClickListener(v -> switchPluginView());

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO)
                != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.RECORD_AUDIO},
                    AUDIO_PERMISSION_REQUEST_CODE);
        } else {
            AudioEngine.native_create();
            AudioEngine.native_start();
            loadPlugins();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == AUDIO_PERMISSION_REQUEST_CODE) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                AudioEngine.native_create();
                AudioEngine.native_start();
                loadPlugins();
            } else {
                Toast.makeText(this, "Audio permission is required for this app", Toast.LENGTH_SHORT).show();
            }
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == FILE_PICKER_REQUEST_CODE && resultCode == RESULT_OK) {
            if (data != null) {
                Uri uri = data.getData();
                AudioBuffer audioBuffer = AudioFileLoader.load(getContentResolver(), uri);
                if (audioBuffer != null) {
                    AudioEngine.native_setAudioBuffer(audioBuffer);
                    Toast.makeText(this, "File loaded successfully", Toast.LENGTH_SHORT).show();
                } else {
                    Toast.makeText(this, "Failed to load file", Toast.LENGTH_SHORT).show();
                }
            }
        }
    }

    @Override
    protected void onStart() {
        super.onStart();
        processingModeDetector.setInForeground(true);
    }

    @Override
    protected void onStop() {
        super.onStop();
        processingModeDetector.setInForeground(false);
        AudioEngine.native_stop();
    }

    private void loadPlugins() {
        String eqPath = getApplicationInfo().nativeLibraryDir + "/libparametric_eq.so";
        eqPlugin = avstHost.loadPlugin(eqPath);
        if (eqPlugin != null) {
            parametricEQView.setPlugin(eqPlugin);
        } else {
            Toast.makeText(this, "Failed to load Parametric EQ plugin", Toast.LENGTH_SHORT).show();
        }

        String compressorPath = getApplicationInfo().nativeLibraryDir + "/libcompressor.so";
        compressorPlugin = avstHost.loadPlugin(compressorPath);
        if (compressorPlugin != null) {
            compressorView.setPlugin(compressorPlugin);
        } else {
            Toast.makeText(this, "Failed to load Compressor plugin", Toast.LENGTH_SHORT).show();
        }
    }

    private void switchPluginView() {
        if (parametricEQView.getVisibility() == View.VISIBLE) {
            parametricEQView.setVisibility(View.GONE);
            compressorView.setVisibility(View.VISIBLE);
        } else {
            parametricEQView.setVisibility(View.VISIBLE);
            compressorView.setVisibility(View.GONE);
        }
    }

    private void showExportDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        LayoutInflater inflater = this.getLayoutInflater();
        View dialogView = inflater.inflate(R.layout.dialog_export, null);
        builder.setView(dialogView);

        final EditText filenameEditText = dialogView.findViewById(R.id.edit_text_filename);
        final RadioGroup formatRadioGroup = dialogView.findViewById(R.id.radio_group_format);
        final EditText targetLoudnessEditText = dialogView.findViewById(R.id.edit_text_target_loudness);
        final EditText bitrateEditText = dialogView.findViewById(R.id.edit_text_bitrate);

        builder.setTitle("Export Audio")
                .setPositiveButton("Export", (dialog, which) -> {
                    String filename = filenameEditText.getText().toString();
                    if (filename.isEmpty()) {
                        Toast.makeText(this, "Filename cannot be empty", Toast.LENGTH_SHORT).show();
                        return;
                    }

                    int format = formatRadioGroup.getCheckedRadioButtonId() == R.id.radio_button_wav ?
                            AudioEngine.FORMAT_WAV : AudioEngine.FORMAT_MP3;

                    double targetLoudness = -100.0; // Default to no normalization
                    try {
                        targetLoudness = Double.parseDouble(targetLoudnessEditText.getText().toString());
                    } catch (NumberFormatException e) {
                        // Keep default
                    }

                    int bitrate = 192; // Default bitrate
                    try {
                        bitrate = Integer.parseInt(bitrateEditText.getText().toString());
                    } catch (NumberFormatException e) {
                        // Keep default
                    }

                    String filePath = getExternalFilesDir(null).getAbsolutePath() + "/" + filename;

                    boolean success = AudioEngine.native_exportFile(filePath, format, targetLoudness, bitrate);
                    if (success) {
                        Toast.makeText(this, "File exported successfully to " + filePath, Toast.LENGTH_LONG).show();
                    } else {
                        Toast.makeText(this, "Failed to export file", Toast.LENGTH_SHORT).show();
                    }
                })
                .setNegativeButton("Cancel", (dialog, which) -> dialog.cancel());

        builder.create().show();
    }
}
