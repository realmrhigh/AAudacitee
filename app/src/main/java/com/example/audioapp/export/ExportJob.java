package com.example.audioapp.export;

import com.example.audioapp.audio.AudioBuffer;
import com.example.audioapp.engine.ProgressListener;
import com.example.audioapp.engine.CancellationToken;

public class ExportJob implements Comparable<ExportJob> {
    private final AudioBuffer inputBuffer;
    private final String path;
    private final ProgressListener listener;
    private final CancellationToken token;
    private final int priority;

    public ExportJob(AudioBuffer inputBuffer, String path, ProgressListener listener, CancellationToken token, int priority) {
        this.inputBuffer = inputBuffer;
        this.path = path;
        this.listener = listener;
        this.token = token;
        this.priority = priority;
    }

    public AudioBuffer getInputBuffer() {
        return inputBuffer;
    }

    public String getPath() {
        return path;
    }

    public ProgressListener getListener() {
        return listener;
    }

    public CancellationToken getToken() {
        return token;
    }

    public int getPriority() {
        return priority;
    }

    @Override
    public int compareTo(ExportJob other) {
        return Integer.compare(other.priority, this.priority);
    }
}
