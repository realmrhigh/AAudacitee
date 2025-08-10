package com.example.audioapp.audio;

import android.app.Activity;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Intent;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.projection.AudioPlaybackCaptureConfiguration;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.os.IBinder;
import android.util.Log;

import androidx.annotation.Nullable;
import androidx.core.app.NotificationCompat;

import java.util.Objects;

public class AudioCaptureService extends Service {
    private static final String CHANNEL_ID = "AudioCaptureServiceChannel";
    private static final int NOTIFICATION_ID = 1;

    private MediaProjectionManager mediaProjectionManager;
    private MediaProjection mediaProjection;
    private AudioRecord audioRecord;
    private Thread captureThread;
    private boolean isCapturing = false;
    private AudioStreamQueue audioStreamQueue;

    @Override
    public void onCreate() {
        super.onCreate();
        mediaProjectionManager = (MediaProjectionManager) getSystemService(MEDIA_PROJECTION_SERVICE);
        createNotificationChannel();
        audioStreamQueue = AudioStreamManager.getInstance().getQueue();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Notification notification = new NotificationCompat.Builder(this, CHANNEL_ID)
                .setContentTitle("Audio Capture Service")
                .setContentText("Capturing audio in the background.")
                .build();
        startForeground(NOTIFICATION_ID, notification);

        if (intent != null && "START".equals(intent.getAction())) {
            startCapture(intent);
        } else if (intent != null && "STOP".equals(intent.getAction())) {
            stopSelf();
        }

        return START_STICKY;
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        stopCapture();
    }

    private void createNotificationChannel() {
        NotificationChannel serviceChannel = new NotificationChannel(
                CHANNEL_ID,
                "Audio Capture Service Channel",
                NotificationManager.IMPORTANCE_DEFAULT
        );
        NotificationManager manager = getSystemService(NotificationManager.class);
        manager.createNotificationChannel(serviceChannel);
    }

    private void startCapture(Intent intent) {
        if (isCapturing) {
            return;
        }
        int resultCode = intent.getIntExtra("resultCode", Activity.RESULT_CANCELED);
        Intent data = intent.getParcelableExtra("data");
        mediaProjection = mediaProjectionManager.getMediaProjection(resultCode, Objects.requireNonNull(data));

        AudioPlaybackCaptureConfiguration config = new AudioPlaybackCaptureConfiguration.Builder(mediaProjection)
            .addMatchingUsage(AudioAttributes.USAGE_MEDIA)
            .addMatchingUsage(AudioAttributes.USAGE_UNKNOWN)
            .addMatchingUsage(AudioAttributes.USAGE_GAME)
            .build();

        AudioFormat audioFormat = new AudioFormat.Builder()
            .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
            .setSampleRate(44100)
            .setChannelMask(AudioFormat.CHANNEL_IN_STEREO)
            .build();

        int bufferSize = AudioRecord.getMinBufferSize(44100, AudioFormat.CHANNEL_IN_STEREO, AudioFormat.ENCODING_PCM_16BIT);

        audioRecord = new AudioRecord.Builder()
            .setAudioFormat(audioFormat)
            .setBufferSizeInBytes(bufferSize)
            .setAudioPlaybackCaptureConfig(config)
            .build();

        audioRecord.startRecording();
        isCapturing = true;

        captureThread = new Thread(() -> {
            byte[] buffer = new byte[bufferSize];
            while (isCapturing) {
                int read = audioRecord.read(buffer, 0, buffer.length);
                if (read > 0) {
                    if (audioStreamQueue != null) {
                        audioStreamQueue.push(buffer, read);
                    }
                }
            }
        });
        captureThread.start();
    }

    private void stopCapture() {
        isCapturing = false;
        if (captureThread != null) {
            try {
                captureThread.join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            captureThread = null;
        }
        if (audioRecord != null) {
            audioRecord.stop();
            audioRecord.release();
            audioRecord = null;
        }
        if (mediaProjection != null) {
            mediaProjection.stop();
            mediaProjection = null;
        }
    }
}
