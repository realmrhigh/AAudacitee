package com.example.audioapp.ui;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;
import com.example.audioapp.R;
import com.example.audioapp.audio.AudioBuffer;
import com.example.audioapp.audio.AudioEngine;
import com.example.audioapp.audio.AudioFileLoader;
import com.example.audioapp.engine.ProcessingModeDetector;

public class MainActivity extends AppCompatActivity {

    private static final int AUDIO_PERMISSION_REQUEST_CODE = 1;
    private static final int FILE_PICKER_REQUEST_CODE = 2;

    private TimelineView timelineView;
    private Handler handler = new Handler();
    private Runnable playbackPositionUpdater;
    private ProcessingModeDetector processingModeDetector;

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

    // Used to load the 'audioapp' library on application startup.
    static {
        System.loadLibrary("audioapp");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        timelineView = findViewById(R.id.timeline_view);
        processingModeDetector = new ProcessingModeDetector();

        setupTransportControls();
        setupEQControls();
        setupMasterVolume();

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO)
                != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.RECORD_AUDIO},
                    AUDIO_PERMISSION_REQUEST_CODE);
        } else {
            AudioEngine.native_create();
            AudioEngine.native_start();
        }

        playbackPositionUpdater = new Runnable() {
            @Override
            public void run() {
                timelineView.setPlaybackPosition(AudioEngine.native_getPlaybackPosition());
                handler.postDelayed(this, 100);
            }
        };
    }

    private void setupTransportControls() {
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
    }

    private void setupEQControls() {
        int[] bandIds = {R.id.eq_band_1, R.id.eq_band_2, R.id.eq_band_3, 
                        R.id.eq_band_4, R.id.eq_band_5, R.id.eq_band_6};
        
        for (int i = 0; i < 6; i++) {
            final int bandIndex = i;
            View bandView = findViewById(bandIds[i]);
            
            // Set band title
            TextView bandTitle = bandView.findViewById(R.id.band_title);
            bandTitle.setText(BAND_NAMES[i]);
            
            // Setup enable/disable switch
            Switch enableSwitch = bandView.findViewById(R.id.band_enable);
            enableSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> 
                AudioEngine.native_setEQBandEnabled(bandIndex, isChecked));
            
            // Setup frequency control
            SeekBar frequencySlider = bandView.findViewById(R.id.frequency_slider);
            TextView frequencyValue = bandView.findViewById(R.id.frequency_value);
            
            frequencySlider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
                @Override
                public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                    if (fromUser) {
                        float frequency = MIN_FREQUENCIES[bandIndex] + 
                            (MAX_FREQUENCIES[bandIndex] - MIN_FREQUENCIES[bandIndex]) * progress / 100f;
                        AudioEngine.native_setEQBandFrequency(bandIndex, frequency);
                        updateFrequencyDisplay(frequencyValue, frequency);
                    }
                }
                
                @Override public void onStartTrackingTouch(SeekBar seekBar) {}
                @Override public void onStopTrackingTouch(SeekBar seekBar) {}
            });
            
            // Setup gain control (-12dB to +12dB)
            SeekBar gainSlider = bandView.findViewById(R.id.gain_slider);
            TextView gainValue = bandView.findViewById(R.id.gain_value);
            
            gainSlider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
                @Override
                public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                    if (fromUser) {
                        float gain = (progress - 120) / 10f; // -12dB to +12dB
                        AudioEngine.native_setEQBandGain(bandIndex, gain);
                        gainValue.setText(String.format("%.1f dB", gain));
                    }
                }
                
                @Override public void onStartTrackingTouch(SeekBar seekBar) {}
                @Override public void onStopTrackingTouch(SeekBar seekBar) {}
            });
            
            // Setup Q factor control (0.1 to 10.0)
            SeekBar qSlider = bandView.findViewById(R.id.q_slider);
            TextView qValue = bandView.findViewById(R.id.q_value);
            
            qSlider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
                @Override
                public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                    if (fromUser) {
                        float q = 0.1f + (progress / 100f) * 9.9f; // 0.1 to 10.0
                        AudioEngine.native_setEQBandQ(bandIndex, q);
                        qValue.setText(String.format("%.1f", q));
                    }
                }
                
                @Override public void onStartTrackingTouch(SeekBar seekBar) {}
                @Override public void onStopTrackingTouch(SeekBar seekBar) {}
            });
            
            // Set initial values
            int freqProgress = (int)((DEFAULT_FREQUENCIES[i] - MIN_FREQUENCIES[i]) / 
                (MAX_FREQUENCIES[i] - MIN_FREQUENCIES[i]) * 100);
            frequencySlider.setProgress(freqProgress);
            updateFrequencyDisplay(frequencyValue, DEFAULT_FREQUENCIES[i]);
        }
    }
    
    private void setupMasterVolume() {
        SeekBar masterVolume = findViewById(R.id.master_volume);
        masterVolume.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if (fromUser) {
                    float volume = progress / 100f;
                    AudioEngine.native_setMasterVolume(volume);
                }
            }
            
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });
    }
    
    private void updateFrequencyDisplay(TextView textView, float frequency) {
        if (frequency >= 1000) {
            textView.setText(String.format("%.1f kHz", frequency / 1000f));
        } else {
            textView.setText(String.format("%.0f Hz", frequency));
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == AUDIO_PERMISSION_REQUEST_CODE) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                AudioEngine.native_create();
                AudioEngine.native_start();
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
                    timelineView.setAudioBuffer(audioBuffer);
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
        handler.post(playbackPositionUpdater);
    }

    @Override
    protected void onStop() {
        super.onStop();
        processingModeDetector.setInForeground(false);
        handler.removeCallbacks(playbackPositionUpdater);
        AudioEngine.native_stop();
    }
}
