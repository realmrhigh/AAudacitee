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
import android.widget.Button;
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
