package com.example.audioapp.audio;

public class AudioStreamManager {
    private static final AudioStreamManager INSTANCE = new AudioStreamManager();
    private final AudioStreamQueue queue;

    private AudioStreamManager() {
        // Queue size of 200 should be enough to buffer ~2 seconds of audio
        // at 44.1kHz and a buffer size of 4096 bytes, which is a safe margin.
        queue = new AudioStreamQueue(200);
    }

    public static AudioStreamManager getInstance() {
        return INSTANCE;
    }

    public AudioStreamQueue getQueue() {
        return queue;
    }

    // It's good practice to have a way to release native resources.
    public void release() {
        queue.destroy();
    }
}
