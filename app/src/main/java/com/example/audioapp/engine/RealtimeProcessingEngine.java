package com.example.audioapp.engine;

import com.example.audioapp.avst.AvstHost;

public class RealtimeProcessingEngine extends ProcessingEngine {
    private final AvstHost avstHost;

    public RealtimeProcessingEngine(AvstHost avstHost) {
        this.avstHost = avstHost;
    }

    @Override
    public void process(float[] buffer, int sampleRate) {
        if (getMode() == Mode.REALTIME) {
            avstHost.process(buffer, sampleRate);
        }
    }
}
