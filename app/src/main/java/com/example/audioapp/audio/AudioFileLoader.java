package com.example.audioapp.audio;

import android.content.ContentResolver;
import android.net.Uri;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class AudioFileLoader {

    public static AudioBuffer load(ContentResolver contentResolver, Uri uri) {
        try (InputStream inputStream = contentResolver.openInputStream(uri)) {
            // Read the WAV header
            byte[] header = new byte[44];
            inputStream.read(header);

            ByteBuffer headerBuffer = ByteBuffer.wrap(header).order(ByteOrder.LITTLE_ENDIAN);

            // Check the RIFF, WAVE, and fmt markers
            if (headerBuffer.getInt(0) != 0x52494646 ||
                headerBuffer.getInt(8) != 0x57415645 ||
                headerBuffer.getInt(12) != 0x666d7420) {
                return null; // Not a valid WAV file
            }

            int channelCount = headerBuffer.getShort(22);
            int sampleRate = headerBuffer.getInt(24);
            int bitsPerSample = headerBuffer.getShort(34);

            // Find the data chunk
            while (headerBuffer.getInt(36) != 0x64617461) {
                int chunkSize = headerBuffer.getInt(40);
                inputStream.skip(chunkSize);
                inputStream.read(header, 36, 8);
                headerBuffer = ByteBuffer.wrap(header).order(ByteOrder.LITTLE_ENDIAN);
            }

            int dataSize = headerBuffer.getInt(40);
            byte[] data = new byte[dataSize];
            inputStream.read(data);

            // Convert the byte data to float
            float[] floatData = new float[dataSize / (bitsPerSample / 8)];
            ByteBuffer dataBuffer = ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN);
            for (int i = 0; i < floatData.length; i++) {
                if (bitsPerSample == 16) {
                    floatData[i] = dataBuffer.getShort() / 32768.0f;
                } else if (bitsPerSample == 24) {
                    int sample = (dataBuffer.get() & 0xFF) |
                                 ((dataBuffer.get() & 0xFF) << 8) |
                                 ((dataBuffer.get() & 0xFF) << 16);
                    if ((sample & 0x800000) != 0) {
                        sample |= 0xFF000000;
                    }
                    floatData[i] = sample / 8388608.0f;
                } else if (bitsPerSample == 32) {
                    floatData[i] = dataBuffer.getInt() / 2147483648.0f;
                }
            }

            return new AudioBuffer(floatData, sampleRate, channelCount);
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }
}
