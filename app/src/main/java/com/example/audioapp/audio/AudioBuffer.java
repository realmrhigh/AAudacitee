package com.example.audioapp.audio;

public class AudioBuffer {
    private final float[] data;
    private final int sampleRate;
    private final int channelCount;

    public AudioBuffer(float[] data, int sampleRate, int channelCount) {
        this.data = data;
        this.sampleRate = sampleRate;
        this.channelCount = channelCount;
    }

    public float[] getData() {
        return data;
    }

    public int getSampleRate() {
        return sampleRate;
    }

    public int getChannelCount() {
        return channelCount;
    }

    public int getFrameCount() {
        return data.length / channelCount;
    }
}
