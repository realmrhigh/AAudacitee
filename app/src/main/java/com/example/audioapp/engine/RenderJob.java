package com.example.audioapp.engine;

import com.example.audioapp.audio.AudioBuffer;

public class RenderJob {
    private final AudioBuffer inputBuffer;
    private final ProgressListener listener;
    private final CancellationToken token;

    public RenderJob(AudioBuffer inputBuffer, ProgressListener listener, CancellationToken token) {
        this.inputBuffer = inputBuffer;
        this.listener = listener;
        this.token = token;
    }

    public AudioBuffer getInputBuffer() {
        return inputBuffer;
    }

    public ProgressListener getListener() {
        return listener;
    }

    public CancellationToken getToken() {
        return token;
    }
}
