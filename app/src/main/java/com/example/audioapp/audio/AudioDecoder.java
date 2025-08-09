package com.example.audioapp.audio;

import android.content.ContentResolver;
import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.net.Uri;
import java.nio.ByteBuffer;

public class AudioDecoder {

    public static AudioBuffer decode(ContentResolver contentResolver, Uri uri) {
        MediaExtractor extractor = null;
        MediaCodec codec = null;
        
        try {
            extractor = new MediaExtractor();
            
            // Set data source with better error handling
            try {
                extractor.setDataSource(contentResolver.openFileDescriptor(uri, "r").getFileDescriptor());
            } catch (Exception e) {
                throw new RuntimeException("Failed to open file: " + e.getMessage(), e);
            }

            MediaFormat format = null;
            int audioTrackIndex = -1;
            
            // Find audio track
            for (int i = 0; i < extractor.getTrackCount(); i++) {
                MediaFormat f = extractor.getTrackFormat(i);
                String mime = f.getString(MediaFormat.KEY_MIME);
                if (mime != null && mime.startsWith("audio/")) {
                    format = f;
                    audioTrackIndex = i;
                    extractor.selectTrack(i);
                    break;
                }
            }

            if (format == null || audioTrackIndex == -1) {
                throw new RuntimeException("No audio track found in file");
            }
            
            // Validate format parameters
            if (!format.containsKey(MediaFormat.KEY_SAMPLE_RATE) || 
                !format.containsKey(MediaFormat.KEY_CHANNEL_COUNT)) {
                throw new RuntimeException("Invalid audio format - missing sample rate or channel count");
            }

            
            // Get codec information
            String mime = format.getString(MediaFormat.KEY_MIME);
            if (mime == null) {
                throw new RuntimeException("No MIME type found for audio track");
            }
            
            // Create and configure codec
            codec = MediaCodec.createDecoderByType(mime);
            codec.configure(format, null, null, 0);
            codec.start();

            // Get buffers
            ByteBuffer[] inputBuffers = codec.getInputBuffers();
            ByteBuffer[] outputBuffers = codec.getOutputBuffers();
            MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();

            // Get audio properties
            int sampleRate = format.getInteger(MediaFormat.KEY_SAMPLE_RATE);
            int channelCount = format.getInteger(MediaFormat.KEY_CHANNEL_COUNT);
            
            // Validate audio properties
            if (sampleRate <= 0 || sampleRate > 192000) {
                throw new RuntimeException("Invalid sample rate: " + sampleRate);
            }
            if (channelCount <= 0 || channelCount > 8) {
                throw new RuntimeException("Invalid channel count: " + channelCount);
            }
            
            int totalSize = 0;
            byte[] allData = new byte[1024 * 1024]; // Start with 1MB buffer

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

            // Clean up resources
            if (codec != null) {
                codec.stop();
                codec.release();
            }
            if (extractor != null) {
                extractor.release();
            }

            // Validate we got some data
            if (totalSize == 0) {
                throw new RuntimeException("No audio data decoded from file");
            }

            // Convert to float with bounds checking
            if (totalSize % 2 != 0) {
                totalSize--; // Make sure we have even number of bytes for 16-bit samples
            }
            
            float[] floatData = new float[totalSize / 2];
            ByteBuffer dataBuffer = ByteBuffer.wrap(allData, 0, totalSize).order(java.nio.ByteOrder.LITTLE_ENDIAN);
            
            for (int i = 0; i < floatData.length; i++) {
                floatData[i] = dataBuffer.getShort() / 32768.0f;
            }

            return new AudioBuffer(floatData, sampleRate, channelCount);

        } catch (Exception e) {
            // Clean up resources on error
            if (codec != null) {
                try {
                    codec.stop();
                    codec.release();
                } catch (Exception ignored) {}
            }
            if (extractor != null) {
                try {
                    extractor.release();
                } catch (Exception ignored) {}
            }
            
            // Re-throw with more context
            throw new RuntimeException("Audio decoding failed: " + e.getMessage(), e);
        }
    }
}
