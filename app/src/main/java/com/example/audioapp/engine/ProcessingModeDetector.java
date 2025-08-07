package com.example.audioapp.engine;

public class ProcessingModeDetector {
    private boolean isInForeground = true;

    public void setInForeground(boolean inForeground) {
        isInForeground = inForeground;
    }

    public ProcessingEngine.Mode getMode() {
        return isInForeground ? ProcessingEngine.Mode.REALTIME : ProcessingEngine.Mode.OFFLINE;
    }
}
