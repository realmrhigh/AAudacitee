package com.example.audioapp.export;

import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class WavWriter {
    private final FileOutputStream fos;
    private final int sampleRate;
    private final int numChannels;
    private int numBytesWritten = 0;

    public WavWriter(FileOutputStream fos, int sampleRate, int numChannels) throws IOException {
        this.fos = fos;
        this.sampleRate = sampleRate;
        this.numChannels = numChannels;
        writeWavHeader();
    }

    public void write(float[] data) throws IOException {
        byte[] byteData = new byte[data.length * 2];
        for (int i = 0; i < data.length; i++) {
            short sample = (short) (data[i] * 32767.0f);
            byteData[i * 2] = (byte) (sample & 0xFF);
            byteData[i * 2 + 1] = (byte) ((sample >> 8) & 0xFF);
        }
        fos.write(byteData);
        numBytesWritten += byteData.length;
    }

    public void close() throws IOException {
        updateWavHeader();
        fos.close();
    }

    private void writeWavHeader() throws IOException {
        byte[] header = new byte[44];
        ByteBuffer buffer = ByteBuffer.wrap(header).order(ByteOrder.LITTLE_ENDIAN);
        buffer.put("RIFF".getBytes());
        buffer.putInt(0); // chunk size
        buffer.put("WAVE".getBytes());
        buffer.put("fmt ".getBytes());
        buffer.putInt(16); // subchunk 1 size
        buffer.putShort((short) 1); // audio format
        buffer.putShort((short) numChannels); // num channels
        buffer.putInt(sampleRate); // sample rate
        buffer.putInt(sampleRate * numChannels * 2); // byte rate
        buffer.putShort((short) (numChannels * 2)); // block align
        buffer.putShort((short) 16); // bits per sample
        buffer.put("data".getBytes());
        buffer.putInt(0); // subchunk 2 size
        fos.write(header);
    }

    private void updateWavHeader() throws IOException {
        byte[] header = new byte[4];
        ByteBuffer buffer = ByteBuffer.wrap(header).order(ByteOrder.LITTLE_ENDIAN);
        buffer.putInt(36 + numBytesWritten);
        fos.getChannel().position(4).write(ByteBuffer.wrap(header));
        buffer.clear();
        buffer.putInt(numBytesWritten);
        fos.getChannel().position(40).write(ByteBuffer.wrap(header));
    }
}
