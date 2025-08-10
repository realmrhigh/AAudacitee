package com.example.audioapp.ui;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.media.projection.MediaProjectionManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RadioGroup;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;

import com.example.audioapp.R;
import com.example.audioapp.audio.AudioBuffer;
import com.example.audioapp.audio.AudioCaptureService;
import com.example.audioapp.audio.AudioEngine;
import com.example.audioapp.audio.AudioFileLoader;
import com.example.audioapp.audio.AudioStreamManager;
import com.example.audioapp.audio.AudioStreamQueue;

public class MainActivity extends AppCompatActivity {

    private static final int AUDIO_PERMISSION_REQUEST_CODE = 1;
    private static final int STORAGE_PERMISSION_REQUEST_CODE = 3;

    private ActivityResultLauncher<Intent> filePickerLauncher;
    private ActivityResultLauncher<Intent> mediaProjectionLauncher;
    private MediaProjectionManager mediaProjectionManager;
    
    // UI Components
    private TimelineView timelineView;
    private TextView fileInfoText;
    private TextView currentTimeText;
    private TextView totalTimeText;
    private SeekBar progressBar;
    private Spinner pluginSpinner;
    private Switch bypassSwitch;
    private LinearLayout eqControlsLayout;
    private ToggleButton captureButton;
    
    // Audio state
    private AudioBuffer currentAudioBuffer;
    private boolean isPlaying = false;
    private Handler handler = new Handler();
    private Runnable playbackPositionUpdater;

    // Single background thread/handler for all native calls and polling
    private HandlerThread positionThread;
    private Handler bgHandler;
    
    // EQ Band configurations
    private static final String[] BAND_NAMES = {
        "Band 1 - 80 Hz (Low Shelf)",
        "Band 2 - 200 Hz (Peak)", 
        "Band 3 - 800 Hz (Peak)",
        "Band 4 - 2 kHz (Peak)",
        "Band 5 - 5 kHz (Peak)",
        "Band 6 - 10 kHz (High Shelf)"
    };
    
    private static final float[] DEFAULT_FREQUENCIES = {80f, 200f, 800f, 2000f, 5000f, 10000f};
    private static final float[] MIN_FREQUENCIES = {20f, 100f, 400f, 1000f, 2500f, 5000f};
    private static final float[] MAX_FREQUENCIES = {200f, 500f, 1600f, 4000f, 10000f, 20000f};

    // Plugin types
    private static final String[] PLUGIN_TYPES = {
        "Parametric EQ",
        "Compressor", 
        "Leveler",
        "Transient Shaper"
    };

    // Used to load the 'audioapp' library on application startup.
    static {
        System.loadLibrary("audioapp");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Initialize file picker launcher
        filePickerLauncher = registerForActivityResult(
            new ActivityResultContracts.StartActivityForResult(),
            result -> {
                if (result.getResultCode() == RESULT_OK && result.getData() != null) {
                    Uri uri = result.getData().getData();
                    if (uri != null) {
                        loadAudioFile(uri);
                    }
                }
            }
        );

        mediaProjectionManager = (MediaProjectionManager) getSystemService(Context.MEDIA_PROJECTION_SERVICE);

        mediaProjectionLauncher = registerForActivityResult(
            new ActivityResultContracts.StartActivityForResult(),
            result -> {
                if (result.getResultCode() == Activity.RESULT_OK) {
                    Intent serviceIntent = new Intent(this, AudioCaptureService.class);
                    serviceIntent.setAction("START");
                    serviceIntent.putExtra("resultCode", result.getResultCode());
                    serviceIntent.putExtra("data", result.getData());
                    startForegroundService(serviceIntent);
                } else {
                    Toast.makeText(this, "Screen Cast permission is required to capture audio", Toast.LENGTH_SHORT).show();
                    if (captureButton != null) {
                        captureButton.setChecked(false); // Reset the button state
                    }
                }
            }
        );

        // Initialize UI components
        initializeUI();
        
        // Setup transport controls
        setupTransportControls();
        
        // Setup plugin controls
        setupPluginControls();
        
        // Setup EQ controls (default)
        setupEQControls();

        // Ensure background handler exists early
        ensureBgHandler();

        // Request audio permission
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.RECORD_AUDIO}, AUDIO_PERMISSION_REQUEST_CODE);
        } else {
            initializeAudioEngine();
        }

        // Start playback position updating
        startPlaybackPositionUpdate();
    }

    private void ensureBgHandler() {
        if (positionThread == null) {
            positionThread = new HandlerThread("AudioBg");
            positionThread.start();
            bgHandler = new Handler(positionThread.getLooper());
        }
    }

    private void initializeUI() {
        timelineView = findViewById(R.id.timeline_view);
        fileInfoText = findViewById(R.id.file_info);
        currentTimeText = findViewById(R.id.current_time);
        totalTimeText = findViewById(R.id.total_time);
        progressBar = findViewById(R.id.progress_bar);
        pluginSpinner = findViewById(R.id.plugin_spinner);
        bypassSwitch = findViewById(R.id.plugin_bypass_switch);
        eqControlsLayout = findViewById(R.id.eq_controls_layout);
        captureButton = findViewById(R.id.button_capture);
        
        // Setup progress bar interaction
        progressBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if (fromUser && currentAudioBuffer != null) {
                    int position = (int) ((progress / 100.0f) * currentAudioBuffer.getFrameCount());
                    ensureBgHandler();
                    bgHandler.post(() -> {
                        try {
                            AudioEngine.native_setPlaybackPosition(position);
                        } catch (Exception e) {
                            android.util.Log.e("AudioApp", "Error setting playback position: " + e.getMessage());
                        }
                    });
                    // Update UI immediately
                    timelineView.setPlaybackPosition(position);
                    updateTimeDisplay(position);
                }
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {}

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {}
        });
    }

    private void setupTransportControls() {
        Button loadFileButton = findViewById(R.id.button_load_file);
        loadFileButton.setOnClickListener(v -> checkStoragePermissionAndLoadFile());

        Button playButton = findViewById(R.id.button_play);
        playButton.setOnClickListener(v -> {
            if (currentAudioBuffer != null) {
                try {
                    android.util.Log.i("AudioApp", "Play button pressed - starting playback");
                    android.util.Log.i("AudioApp", "Buffer info: frames=" + currentAudioBuffer.getFrameCount() + 
                                                    ", channels=" + currentAudioBuffer.getChannelCount() + 
                                                    ", sampleRate=" + currentAudioBuffer.getSampleRate());
                    ensureBgHandler();
                    bgHandler.post(() -> {
                        try {
                            long startTime = System.currentTimeMillis();
                            AudioEngine.native_setPlaying(true);
                            long endTime = System.currentTimeMillis();
                            android.util.Log.i("AudioApp", "native_setPlaying call completed in " + (endTime - startTime) + "ms");
                            runOnUiThread(() -> {
                                isPlaying = true;
                                android.util.Log.i("AudioApp", "Playback started successfully");
                            });
                        } catch (Exception e) {
                            android.util.Log.e("AudioApp", "Error starting playback: " + e.getMessage());
                            runOnUiThread(() -> 
                                Toast.makeText(this, "Error starting playback: " + e.getMessage(), Toast.LENGTH_SHORT).show()
                            );
                        }
                    });
                } catch (Exception e) {
                    android.util.Log.e("AudioApp", "Error in play button handler: " + e.getMessage());
                    Toast.makeText(this, "Error starting playback: " + e.getMessage(), Toast.LENGTH_SHORT).show();
                }
            } else {
                android.util.Log.w("AudioApp", "Play button pressed but no audio buffer loaded");
                Toast.makeText(this, "Please load an audio file first", Toast.LENGTH_SHORT).show();
            }
        });

        Button pauseButton = findViewById(R.id.button_pause);
        pauseButton.setOnClickListener(v -> {
            try {
                android.util.Log.i("AudioApp", "Pause button pressed");
                ensureBgHandler();
                bgHandler.post(() -> {
                    try {
                        AudioEngine.native_setPlaying(false);
                        runOnUiThread(() -> {
                            isPlaying = false;
                            android.util.Log.i("AudioApp", "Playback paused successfully");
                        });
                    } catch (Exception e) {
                        android.util.Log.e("AudioApp", "Error pausing playback: " + e.getMessage());
                    }
                });
            } catch (Exception e) {
                android.util.Log.e("AudioApp", "Error in pause button handler: " + e.getMessage());
            }
        });

        Button stopButton = findViewById(R.id.button_stop);
        stopButton.setOnClickListener(v -> {
            try {
                android.util.Log.i("AudioApp", "Stop button pressed");
                ensureBgHandler();
                bgHandler.post(() -> {
                    try {
                        AudioEngine.native_setPlaying(false);
                        AudioEngine.native_setPlaybackPosition(0);
                        runOnUiThread(() -> {
                            isPlaying = false;
                            progressBar.setProgress(0);
                            timelineView.setPlaybackPosition(0);
                            updateTimeDisplay(0);
                            android.util.Log.i("AudioApp", "Playback stopped successfully");
                        });
                    } catch (Exception e) {
                        android.util.Log.e("AudioApp", "Error stopping playback: " + e.getMessage());
                    }
                });
            } catch (Exception e) {
                android.util.Log.e("AudioApp", "Error in stop button handler: " + e.getMessage());
            }
        });

        Button exportButton = findViewById(R.id.button_export);
        exportButton.setOnClickListener(v -> showExportDialog());

        captureButton.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (isChecked) {
                Intent screenCaptureIntent = mediaProjectionManager.createScreenCaptureIntent();
                mediaProjectionLauncher.launch(screenCaptureIntent);
                AudioStreamQueue queue = AudioStreamManager.getInstance().getQueue();
                AudioEngine.native_setLiveMode(true, queue.getNativeHandle());
                AudioEngine.native_setPlaying(true);
            } else {
                Intent serviceIntent = new Intent(this, AudioCaptureService.class);
                serviceIntent.setAction("STOP");
                stopService(serviceIntent);
                AudioEngine.native_setPlaying(false);
                AudioEngine.native_setLiveMode(false, 0);
            }
        });
    }

    private void setupPluginControls() {
        // Setup plugin spinner
        ArrayAdapter<String> adapter = new ArrayAdapter<>(this, 
            android.R.layout.simple_spinner_item, PLUGIN_TYPES);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        pluginSpinner.setAdapter(adapter);
        
        pluginSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                switchPlugin(position);
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {}
        });

        // Setup bypass switch
        bypassSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> {
            // Handle plugin bypass
            Toast.makeText(this, isChecked ? "Plugin bypassed" : "Plugin active", Toast.LENGTH_SHORT).show();
        });
    }

    private void setupEQControls() {
        eqControlsLayout.removeAllViews();
        
        for (int i = 0; i < BAND_NAMES.length; i++) {
            final int bandIndex = i;
            
            // Create band container
            LinearLayout bandLayout = new LinearLayout(this);
            bandLayout.setOrientation(LinearLayout.VERTICAL);
            bandLayout.setPadding(8, 8, 8, 16);
            
            // Band title and enable switch
            LinearLayout titleLayout = new LinearLayout(this);
            titleLayout.setOrientation(LinearLayout.HORIZONTAL);
            
            TextView bandTitle = new TextView(this);
            bandTitle.setText(BAND_NAMES[i]);
            bandTitle.setTextColor(0xFFFFFFFF);
            bandTitle.setTextSize(14);
            bandTitle.setLayoutParams(new LinearLayout.LayoutParams(0, 
                LinearLayout.LayoutParams.WRAP_CONTENT, 1.0f));
            titleLayout.addView(bandTitle);
            
            Switch bandEnableSwitch = new Switch(this);
            bandEnableSwitch.setChecked(true); // Default enabled
            bandEnableSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> {
                ensureBgHandler();
                bgHandler.post(() -> {
                    AudioEngine.native_setEQBandEnabled(bandIndex, isChecked);
                });
            });
            titleLayout.addView(bandEnableSwitch);
            
            bandLayout.addView(titleLayout);
            
            // Frequency control
            LinearLayout freqLayout = new LinearLayout(this);
            freqLayout.setOrientation(LinearLayout.HORIZONTAL);
            
            TextView freqLabel = new TextView(this);
            freqLabel.setText("Freq: ");
            freqLabel.setTextColor(0xFFCCCCCC);
            freqLabel.setMinWidth(120);
            freqLayout.addView(freqLabel);
            
            SeekBar freqSeekBar = new SeekBar(this);
            freqSeekBar.setMax(100);
            freqSeekBar.setProgress(50);
            freqSeekBar.setLayoutParams(new LinearLayout.LayoutParams(0, 
                LinearLayout.LayoutParams.WRAP_CONTENT, 1.0f));
            
            TextView freqValue = new TextView(this);
            freqValue.setTextColor(0xFFFFFFFF);
            freqValue.setMinWidth(100);
            updateFrequencyDisplay(freqValue, DEFAULT_FREQUENCIES[bandIndex]);
            
            freqSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
                @Override
                public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                    if (fromUser) {
                        float frequency = MIN_FREQUENCIES[bandIndex] + 
                            (progress / 100.0f) * (MAX_FREQUENCIES[bandIndex] - MIN_FREQUENCIES[bandIndex]);
                        updateFrequencyDisplay(freqValue, frequency);
                        ensureBgHandler();
                        bgHandler.post(() -> {
                            AudioEngine.native_setEQBandFrequency(bandIndex, frequency);
                        });
                    }
                }
                @Override public void onStartTrackingTouch(SeekBar seekBar) {}
                @Override public void onStopTrackingTouch(SeekBar seekBar) {}
            });
            
            freqLayout.addView(freqSeekBar);
            freqLayout.addView(freqValue);
            bandLayout.addView(freqLayout);
            
            // Gain control
            LinearLayout gainLayout = new LinearLayout(this);
            gainLayout.setOrientation(LinearLayout.HORIZONTAL);
            
            TextView gainLabel = new TextView(this);
            gainLabel.setText("Gain: ");
            gainLabel.setTextColor(0xFFCCCCCC);
            gainLabel.setMinWidth(120);
            gainLayout.addView(gainLabel);
            
            SeekBar gainSeekBar = new SeekBar(this);
            gainSeekBar.setMax(100);
            gainSeekBar.setProgress(50);
            gainSeekBar.setLayoutParams(new LinearLayout.LayoutParams(0, 
                LinearLayout.LayoutParams.WRAP_CONTENT, 1.0f));
            
            TextView gainValue = new TextView(this);
            gainValue.setText("0.0 dB");
            gainValue.setTextColor(0xFFFFFFFF);
            gainValue.setMinWidth(100);
            
            gainSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
                @Override
                public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                    if (fromUser) {
                        float gain = (progress - 50) * 0.5f; // -25 to +25 dB range
                        gainValue.setText(String.format("%.1f dB", gain));
                        ensureBgHandler();
                        bgHandler.post(() -> {
                            AudioEngine.native_setEQBandGain(bandIndex, gain);
                        });
                    }
                }
                @Override public void onStartTrackingTouch(SeekBar seekBar) {}
                @Override public void onStopTrackingTouch(SeekBar seekBar) {}
            });
            
            gainLayout.addView(gainSeekBar);
            gainLayout.addView(gainValue);
            bandLayout.addView(gainLayout);
            
            // Q/Width control
            LinearLayout qLayout = new LinearLayout(this);
            qLayout.setOrientation(LinearLayout.HORIZONTAL);
            
            TextView qLabel = new TextView(this);
            qLabel.setText("Q: ");
            qLabel.setTextColor(0xFFCCCCCC);
            qLabel.setMinWidth(120);
            qLayout.addView(qLabel);
            
            SeekBar qSeekBar = new SeekBar(this);
            qSeekBar.setMax(100);
            qSeekBar.setProgress(30);
            qSeekBar.setLayoutParams(new LinearLayout.LayoutParams(0, 
                LinearLayout.LayoutParams.WRAP_CONTENT, 1.0f));
            
            TextView qValue = new TextView(this);
            qValue.setText("1.0");
            qValue.setTextColor(0xFFFFFFFF);
            qValue.setMinWidth(100);
            
            qSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
                @Override
                public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                    if (fromUser) {
                        float q = 0.1f + (progress / 100.0f) * 9.9f; // 0.1 to 10.0 range
                        qValue.setText(String.format("%.1f", q));
                        ensureBgHandler();
                        bgHandler.post(() -> {
                            AudioEngine.native_setEQBandQ(bandIndex, q);
                        });
                    }
                }
                @Override public void onStartTrackingTouch(SeekBar seekBar) {}
                @Override public void onStopTrackingTouch(SeekBar seekBar) {}
            });
            
            qLayout.addView(qSeekBar);
            qLayout.addView(qValue);
            bandLayout.addView(qLayout);
            
            eqControlsLayout.addView(bandLayout);
        }
    }

    private void switchPlugin(int pluginIndex) {
        switch (pluginIndex) {
            case 0: // Parametric EQ
                setupEQControls();
                break;
            case 1: // Compressor
                setupCompressorControls();
                break;
            case 2: // Leveler
                setupLevelerControls();
                break;
            case 3: // Transient Shaper
                setupTransientShaperControls();
                break;
        }
    }

    private void setupCompressorControls() {
        eqControlsLayout.removeAllViews();
        
        TextView title = new TextView(this);
        title.setText("Compressor Controls");
        title.setTextColor(0xFFFFFFFF);
        title.setTextSize(18);
        title.setPadding(0, 0, 0, 16);
        eqControlsLayout.addView(title);
        
        // Enable/Disable switch
        Switch enableSwitch = new Switch(this);
        enableSwitch.setText("Enable Compressor");
        enableSwitch.setTextColor(0xFFFFFFFF);
        enableSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> {
            ensureBgHandler();
            bgHandler.post(() -> {
                AudioEngine.native_setCompressorEnabled(isChecked);
            });
        });
        eqControlsLayout.addView(enableSwitch);
        
        addSliderControl("Threshold", -60, 0, -12, "dB", (value) -> {
            ensureBgHandler();
            bgHandler.post(() -> {
                AudioEngine.native_setCompressorThreshold(value);
            });
        });
        
        addSliderControl("Ratio", 1, 20, 4, ":1", (value) -> {
            ensureBgHandler();
            bgHandler.post(() -> {
                AudioEngine.native_setCompressorRatio(value);
            });
        });
        
        addSliderControl("Attack", 0.1f, 100, 10, "ms", (value) -> {
            ensureBgHandler();
            bgHandler.post(() -> {
                AudioEngine.native_setCompressorAttack(value);
            });
        });
        
        addSliderControl("Release", 10, 1000, 100, "ms", (value) -> {
            ensureBgHandler();
            bgHandler.post(() -> {
                AudioEngine.native_setCompressorRelease(value);
            });
        });
    }

    private void setupLevelerControls() {
        eqControlsLayout.removeAllViews();
        
        TextView title = new TextView(this);
        title.setText("Leveler Controls");
        title.setTextColor(0xFFFFFFFF);
        title.setTextSize(18);
        title.setPadding(0, 0, 0, 16);
        eqControlsLayout.addView(title);
        
        // Enable/Disable switch
        Switch enableSwitch = new Switch(this);
        enableSwitch.setText("Enable Leveler");
        enableSwitch.setTextColor(0xFFFFFFFF);
        enableSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> {
            ensureBgHandler();
            bgHandler.post(() -> {
                AudioEngine.native_setLevelerEnabled(isChecked);
            });
        });
        eqControlsLayout.addView(enableSwitch);
        
        addSliderControl("Target Level", -30, 0, -12, "dB", (value) -> {
            ensureBgHandler();
            bgHandler.post(() -> {
                AudioEngine.native_setLevelerTarget(value);
            });
        });
        
        addSliderControl("Speed", 0.1f, 10, 1, "x", (value) -> {
            ensureBgHandler();
            bgHandler.post(() -> {
                AudioEngine.native_setLevelerSpeed(value);
            });
        });
    }

    private void setupTransientShaperControls() {
        eqControlsLayout.removeAllViews();
        
        TextView title = new TextView(this);
        title.setText("Transient Shaper Controls");
        title.setTextColor(0xFFFFFFFF);
        title.setTextSize(18);
        title.setPadding(0, 0, 0, 16);
        eqControlsLayout.addView(title);
        
        // Enable/Disable switch
        Switch enableSwitch = new Switch(this);
        enableSwitch.setText("Enable Transient Shaper");
        enableSwitch.setTextColor(0xFFFFFFFF);
        enableSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> {
            ensureBgHandler();
            bgHandler.post(() -> {
                AudioEngine.native_setTransientShaperEnabled(isChecked);
            });
        });
        eqControlsLayout.addView(enableSwitch);
        
        addSliderControl("Attack", -10, 10, 0, "dB", (value) -> {
            ensureBgHandler();
            bgHandler.post(() -> {
                AudioEngine.native_setTransientAttack(value);
            });
        });
        
        addSliderControl("Sustain", -10, 10, 0, "dB", (value) -> {
            ensureBgHandler();
            bgHandler.post(() -> {
                AudioEngine.native_setTransientSustain(value);
            });
        });
    }

    private void addSliderControl(String name, float min, float max, float defaultValue, String unit, ParameterCallback callback) {
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.HORIZONTAL);
        layout.setPadding(8, 8, 8, 8);
        
        TextView label = new TextView(this);
        label.setText(name + ": ");
        label.setTextColor(0xFFCCCCCC);
        label.setMinWidth(120);
        layout.addView(label);
        
        SeekBar seekBar = new SeekBar(this);
        seekBar.setMax(100);
        seekBar.setProgress((int) ((defaultValue - min) / (max - min) * 100));
        seekBar.setLayoutParams(new LinearLayout.LayoutParams(0, 
            LinearLayout.LayoutParams.WRAP_CONTENT, 1.0f));
        
        TextView valueText = new TextView(this);
        valueText.setText(String.format("%.1f %s", defaultValue, unit));
        valueText.setTextColor(0xFFFFFFFF);
        valueText.setMinWidth(100);
        
        seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if (fromUser) {
                    float value = min + (progress / 100.0f) * (max - min);
                    valueText.setText(String.format("%.1f %s", value, unit));
                    callback.onParameterChanged(value);
                }
            }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });
        
        layout.addView(seekBar);
        layout.addView(valueText);
        eqControlsLayout.addView(layout);
    }

    private interface ParameterCallback {
        void onParameterChanged(float value);
    }

    private void updateFrequencyDisplay(TextView textView, float frequency) {
        if (frequency >= 1000) {
            textView.setText(String.format("%.1f kHz", frequency / 1000f));
        } else {
            textView.setText(String.format("%.0f Hz", frequency));
        }
    }

    private void checkStoragePermissionAndLoadFile() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            // Android 13+ uses READ_MEDIA_AUDIO
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.READ_MEDIA_AUDIO) 
                != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(this, 
                    new String[]{Manifest.permission.READ_MEDIA_AUDIO}, 
                    STORAGE_PERMISSION_REQUEST_CODE);
            } else {
                openFilePicker();
            }
        } else {
            // Android 12 and below use READ_EXTERNAL_STORAGE
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) 
                != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(this, 
                    new String[]{Manifest.permission.READ_EXTERNAL_STORAGE}, 
                    STORAGE_PERMISSION_REQUEST_CODE);
            } else {
                openFilePicker();
            }
        }
    }

    private void openFilePicker() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("audio/*");
        // Add multiple MIME types for better audio file support
        String[] mimeTypes = {"audio/wav", "audio/mp3", "audio/mpeg", "audio/m4a", "audio/aac", "audio/flac", "audio/ogg"};
        intent.putExtra(Intent.EXTRA_MIME_TYPES, mimeTypes);
        filePickerLauncher.launch(intent);
    }

    private void loadAudioFile(Uri uri) {
        // Show loading indicator
        Toast.makeText(this, "Loading audio file...", Toast.LENGTH_SHORT).show();
        
        // Load file in background thread to prevent ANR
        new Thread(() -> {
            try {
                // Add null check and validation
                if (uri == null) {
                    runOnUiThread(() -> Toast.makeText(this, "Invalid file URI", Toast.LENGTH_SHORT).show());
                    return;
                }
                
                // Try to get content info first
                String fileName = uri.getLastPathSegment();
                if (fileName == null) {
                    fileName = "Unknown file";
                }
                
                // Load the audio file with error handling
                AudioBuffer audioBuffer = null;
                try {
                    audioBuffer = AudioFileLoader.load(getContentResolver(), uri);
                } catch (SecurityException e) {
                    runOnUiThread(() -> Toast.makeText(this, "Permission denied accessing file", Toast.LENGTH_LONG).show());
                    return;
                } catch (Exception e) {
                    runOnUiThread(() -> Toast.makeText(this, "Error decoding file: " + e.getMessage(), Toast.LENGTH_LONG).show());
                    e.printStackTrace();
                    return;
                }
                
                if (audioBuffer == null) {
                    runOnUiThread(() -> Toast.makeText(this, "Failed to load file - unsupported format or corrupted file", Toast.LENGTH_LONG).show());
                    return;
                }
                
                // Validate audio buffer data
                if (audioBuffer.getData() == null || audioBuffer.getData().length == 0) {
                    runOnUiThread(() -> Toast.makeText(this, "Audio file appears to be empty", Toast.LENGTH_LONG).show());
                    return;
                }
                
                if (audioBuffer.getSampleRate() <= 0 || audioBuffer.getChannelCount() <= 0) {
                    runOnUiThread(() -> Toast.makeText(this, "Invalid audio format detected", Toast.LENGTH_LONG).show());
                    return;
                }
                
                // Update on UI thread
                final String finalFileName = fileName;
                final AudioBuffer finalAudioBuffer = audioBuffer;
                
                runOnUiThread(() -> {
                    try {
                        // Store the buffer
                        currentAudioBuffer = finalAudioBuffer;

                        // Offload heavy native buffer set to background to avoid UI freeze
                        ensureBgHandler();
                        bgHandler.post(() -> {
                            try {
                                AudioEngine.native_setAudioBuffer(finalAudioBuffer);
                                AudioEngine.native_setPlaybackPosition(0);

                                runOnUiThread(() -> {
                                    try {
                                        // Update UI safely after native work completes
                                        timelineView.setAudioBuffer(finalAudioBuffer);
                                        fileInfoText.setText("Loaded: " + finalFileName);

                                        int frameCount = finalAudioBuffer.getFrameCount();
                                        int sampleRate = finalAudioBuffer.getSampleRate();
                                        float durationSeconds = (float) frameCount / sampleRate;

                                        totalTimeText.setText(formatTime(durationSeconds));
                                        progressBar.setMax(100);
                                        progressBar.setProgress(0);
                                        timelineView.setPlaybackPosition(0);
                                        updateTimeDisplay(0);

                                        Toast.makeText(this, String.format("File loaded: %.1fs, %d Hz, %d ch",
                                                durationSeconds, sampleRate, finalAudioBuffer.getChannelCount()), Toast.LENGTH_SHORT).show();
                                    } catch (Exception e) {
                                        Toast.makeText(this, "Error updating UI: " + e.getMessage(), Toast.LENGTH_LONG).show();
                                        e.printStackTrace();
                                    }
                                });
                            } catch (Exception e) {
                                runOnUiThread(() -> {
                                    Toast.makeText(this, "Error setting audio in engine: " + e.getMessage(), Toast.LENGTH_LONG).show();
                                });
                            }
                        });

                    } catch (Exception e) {
                        Toast.makeText(this, "Unexpected error on UI thread: " + e.getMessage(), Toast.LENGTH_LONG).show();
                        e.printStackTrace();
                    }
                });
                
            } catch (Exception e) {
                runOnUiThread(() -> Toast.makeText(this, "Unexpected error loading file: " + e.getMessage(), Toast.LENGTH_LONG).show());
                e.printStackTrace();
            }
        }).start();
    }

    private void showExportDialog() {
        if (currentAudioBuffer == null) {
            Toast.makeText(this, "Please load an audio file first", Toast.LENGTH_SHORT).show();
            return;
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        LayoutInflater inflater = getLayoutInflater();
        View dialogView = inflater.inflate(android.R.layout.simple_list_item_1, null);
        
        // Simple export dialog for now
        builder.setTitle("Export Audio")
            .setMessage("Export current audio with applied effects?")
            .setPositiveButton("Export WAV", (dialog, which) -> {
                Toast.makeText(this, "Export functionality coming soon", Toast.LENGTH_SHORT).show();
            })
            .setNegativeButton("Cancel", null)
            .show();
    }

    private void startPlaybackPositionUpdate() {
        ensureBgHandler();
        playbackPositionUpdater = new Runnable() {
            @Override
            public void run() {
                try {
                    if (isPlaying && currentAudioBuffer != null) {
                        int position = 0;
                        try {
                            position = AudioEngine.native_getPlaybackPosition();
                        } catch (Exception e) {
                            android.util.Log.e("AudioApp", "Error getting playback position: " + e.getMessage());
                        }
                        int frameCount = currentAudioBuffer.getFrameCount();
                        final int posFinal = position;
                        runOnUiThread(() -> {
                            try {
                                if (posFinal < frameCount && isPlaying) {
                                    float progress = (float) posFinal / frameCount * 100;
                                    progressBar.setProgress((int) progress);
                                    timelineView.setPlaybackPosition(posFinal);
                                    updateTimeDisplay(posFinal);
                                } else if (posFinal >= frameCount && isPlaying) {
                                    // Playback finished
                                    ensureBgHandler();
                                    bgHandler.post(() -> {
                                        try {
                                            AudioEngine.native_setPlaying(false);
                                            runOnUiThread(() -> {
                                                isPlaying = false;
                                                android.util.Log.i("AudioApp", "Playback finished");
                                            });
                                        } catch (Exception e) {
                                            android.util.Log.e("AudioApp", "Error stopping playback at end: " + e.getMessage());
                                        }
                                    });
                                }
                            } catch (Exception e) {
                                android.util.Log.e("AudioApp", "Error updating UI in position updater: " + e.getMessage());
                            }
                        });
                    }
                } catch (Exception e) {
                    android.util.Log.e("AudioApp", "Error in playback position updater: " + e.getMessage());
                }
                // Schedule next update on background handler
                try {
                    bgHandler.postDelayed(this, 100);
                } catch (Exception e) {
                    android.util.Log.e("AudioApp", "Error scheduling next position update: " + e.getMessage());
                }
            }
        };
        bgHandler.post(playbackPositionUpdater);
    }

    private void updateTimeDisplay(int position) {
        if (currentAudioBuffer != null) {
            float currentSeconds = (float) position / currentAudioBuffer.getSampleRate();
            currentTimeText.setText(formatTime(currentSeconds));
        }
    }

    private String formatTime(float seconds) {
        int minutes = (int) (seconds / 60);
        int secs = (int) (seconds % 60);
        return String.format("%02d:%02d", minutes, secs);
    }

    private void initializeAudioEngine() {
        try {
            android.util.Log.i("AudioApp", "Initializing audio engine...");
            AudioEngine.native_create();
            android.util.Log.i("AudioApp", "Audio engine created successfully");
            
            AudioEngine.native_start();
            android.util.Log.i("AudioApp", "Audio engine started successfully");
        } catch (Exception e) {
            android.util.Log.e("AudioApp", "Error initializing audio engine: " + e.getMessage());
            e.printStackTrace();
            Toast.makeText(this, "Failed to initialize audio engine", Toast.LENGTH_LONG).show();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == AUDIO_PERMISSION_REQUEST_CODE) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                initializeAudioEngine();
            } else {
                Toast.makeText(this, "Audio permission is required for this app", Toast.LENGTH_SHORT).show();
            }
        } else if (requestCode == STORAGE_PERMISSION_REQUEST_CODE) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                openFilePicker();
            } else {
                Toast.makeText(this, "Storage permission is required to load audio files", Toast.LENGTH_SHORT).show();
            }
        }
    }

    @Override
    protected void onStart() {
        super.onStart();
    }

    @Override
    protected void onStop() {
        super.onStop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (playbackPositionUpdater != null) {
            handler.removeCallbacks(playbackPositionUpdater);
            if (bgHandler != null) {
                bgHandler.removeCallbacks(playbackPositionUpdater);
            }
        }
        AudioEngine.native_stop();
        if (positionThread != null) {
            try {
                positionThread.quitSafely();
            } catch (Exception ignored) {}
            positionThread = null;
            bgHandler = null;
        }
    }
}
