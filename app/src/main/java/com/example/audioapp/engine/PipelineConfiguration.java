package com.example.audioapp.engine;

public class PipelineConfiguration {
    private int sampleRate;
    private int bufferSize;
    private int quality;

    public PipelineConfiguration(int sampleRate, int bufferSize, int quality) {
        this.sampleRate = sampleRate;
        this.bufferSize = bufferSize;
        this.quality = quality;
    }

    public int getSampleRate() {
        return sampleRate;
    }

    public int getBufferSize() {
        return bufferSize;
    }

    public int getQuality() {
        return quality;
    }
}
