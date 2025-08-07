package com.example.audioapp.export;

import com.example.audioapp.audio.AudioBuffer;
import com.example.audioapp.engine.OfflineProcessingEngine;
import com.example.audioapp.engine.ProgressListener;
import com.example.audioapp.engine.CancellationToken;
import java.io.FileOutputStream;
import java.io.IOException;

public class FileExporter {
    private final OfflineProcessingEngine engine;

    public FileExporter(OfflineProcessingEngine engine) {
        this.engine = engine;
    }

    public void export(AudioBuffer inputBuffer, String path, ProgressListener listener, CancellationToken token) {
        AudioBuffer outputBuffer = engine.renderOffline(inputBuffer, listener, token);
        if (outputBuffer != null) {
            try (FileOutputStream fos = new FileOutputStream(path)) {
                WavWriter writer = new WavWriter(fos, outputBuffer.getSampleRate(), outputBuffer.getChannelCount());
                writer.write(outputBuffer.getData());
                writer.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }
}
