package com.example.audioapp.engine;

public abstract class ProcessingEngine {
    public enum Mode {
        REALTIME,
        OFFLINE
    }

    private Mode mode = Mode.REALTIME;

    public void setMode(Mode mode) {
        this.mode = mode;
    }

    public Mode getMode() {
        return mode;
    }

    public abstract void process(float[] buffer, int sampleRate);
}
