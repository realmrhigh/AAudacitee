package com.example.audioapp.engine;

import com.example.audioapp.audio.AudioBuffer;
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

    public AudioBuffer renderOffline(AudioBuffer inputBuffer, ProgressListener listener, CancellationToken token) {
        float[] inputData = inputBuffer.getData();
        float[] outputData = new float[inputData.length];
        int frameCount = inputBuffer.getFrameCount();
        int bufferSize = 4096;
        int numBuffers = frameCount / bufferSize;

        for (int i = 0; i < numBuffers; i++) {
            if (token.isCancelled()) {
                return null;
            }
            int offset = i * bufferSize;
            int size = Math.min(bufferSize, frameCount - offset);
            float[] chunk = new float[size];
            System.arraycopy(inputData, offset, chunk, 0, size);
            process(chunk, inputBuffer.getSampleRate());
            System.arraycopy(chunk, 0, outputData, offset, size);
            if (listener != null) {
                listener.onProgress((float) i / numBuffers);
            }
        }

        return new AudioBuffer(outputData, inputBuffer.getSampleRate(), inputBuffer.getChannelCount());
    }
}
