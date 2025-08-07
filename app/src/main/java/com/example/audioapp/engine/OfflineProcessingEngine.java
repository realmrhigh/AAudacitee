package com.example.audioapp.engine;

import com.example.audioapp.avst.AvstHost;

public class OfflineProcessingEngine extends ProcessingEngine {
    private final AvstHost avstHost;

    public OfflineProcessingEngine(AvstHost avstHost) {
        this.avstHost = avstHost;
    }

    @Override
    public void process(float[] buffer, int sampleRate) {
        if (getMode() == Mode.OFFLINE) {
            // Here I should use higher quality settings
            avstHost.process(buffer, sampleRate);
        }
    }
}
