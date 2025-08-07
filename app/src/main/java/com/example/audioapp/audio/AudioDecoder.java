package com.example.audioapp.audio;

import android.content.ContentResolver;
import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.net.Uri;
import java.nio.ByteBuffer;

public class AudioDecoder {

    public static AudioBuffer decode(ContentResolver contentResolver, Uri uri) {
        try {
            MediaExtractor extractor = new MediaExtractor();
            extractor.setDataSource(contentResolver.openFileDescriptor(uri, "r").getFileDescriptor());

            MediaFormat format = null;
            for (int i = 0; i < extractor.getTrackCount(); i++) {
                MediaFormat f = extractor.getTrackFormat(i);
                String mime = f.getString(MediaFormat.KEY_MIME);
                if (mime.startsWith("audio/")) {
                    format = f;
                    extractor.selectTrack(i);
                    break;
                }
            }

            if (format == null) {
                return null; // No audio track found
            }

            String mime = format.getString(MediaFormat.KEY_MIME);
            MediaCodec codec = MediaCodec.createDecoderByType(mime);
            codec.configure(format, null, null, 0);
            codec.start();

            ByteBuffer[] inputBuffers = codec.getInputBuffers();
            ByteBuffer[] outputBuffers = codec.getOutputBuffers();
            MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();

            int sampleRate = format.getInteger(MediaFormat.KEY_SAMPLE_RATE);
            int channelCount = format.getInteger(MediaFormat.KEY_CHANNEL_COUNT);
            int totalSize = 0;

            byte[] allData = new byte[1024 * 1024]; // 1MB buffer

            boolean sawInputEOS = false;
            boolean sawOutputEOS = false;

            while (!sawOutputEOS) {
                if (!sawInputEOS) {
                    int inputBufIndex = codec.dequeueInputBuffer(10000);
                    if (inputBufIndex >= 0) {
                        ByteBuffer dstBuf = inputBuffers[inputBufIndex];
                        int sampleSize = extractor.readSampleData(dstBuf, 0);
                        long presentationTimeUs = 0;
                        if (sampleSize < 0) {
                            sawInputEOS = true;
                            sampleSize = 0;
                        } else {
                            presentationTimeUs = extractor.getSampleTime();
                        }
                        codec.queueInputBuffer(inputBufIndex, 0, sampleSize, presentationTimeUs,
                                sawInputEOS ? MediaCodec.BUFFER_FLAG_END_OF_STREAM : 0);
                        if (!sawInputEOS) {
                            extractor.advance();
                        }
                    }
                }

                int outputBufIndex = codec.dequeueOutputBuffer(info, 10000);
                if (outputBufIndex >= 0) {
                    ByteBuffer buf = outputBuffers[outputBufIndex];
                    final byte[] chunk = new byte[info.size];
                    buf.get(chunk);
                    buf.clear();

                    if (totalSize + chunk.length > allData.length) {
                        // Resize buffer
                        byte[] newAllData = new byte[allData.length * 2];
                        System.arraycopy(allData, 0, newAllData, 0, totalSize);
                        allData = newAllData;
                    }
                    System.arraycopy(chunk, 0, allData, totalSize, chunk.length);
                    totalSize += chunk.length;

                    codec.releaseOutputBuffer(outputBufIndex, false);

                    if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                        sawOutputEOS = true;
                    }
                }
            }

            codec.stop();
            codec.release();
            extractor.release();

            // Convert to float
            float[] floatData = new float[totalSize / 2];
            ByteBuffer dataBuffer = ByteBuffer.wrap(allData, 0, totalSize).order(java.nio.ByteOrder.LITTLE_ENDIAN);
            for (int i = 0; i < floatData.length; i++) {
                floatData[i] = dataBuffer.getShort() / 32768.0f;
            }

            return new AudioBuffer(floatData, sampleRate, channelCount);

        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }
}
