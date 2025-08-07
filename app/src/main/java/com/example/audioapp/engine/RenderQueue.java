package com.example.audioapp.engine;

import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

public class RenderQueue {
    private final BlockingQueue<RenderJob> queue = new LinkedBlockingQueue<>();
    private final OfflineProcessingEngine engine;
    private boolean isRunning = false;

    public RenderQueue(OfflineProcessingEngine engine) {
        this.engine = engine;
    }

    public void addJob(RenderJob job) {
        try {
            queue.put(job);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    public void start() {
        if (isRunning) {
            return;
        }
        isRunning = true;
        new Thread(() -> {
            while (isRunning) {
                try {
                    RenderJob job = queue.take();
                    engine.renderOffline(job.getInputBuffer(), job.getListener(), job.getToken());
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }).start();
    }

    public void stop() {
        isRunning = false;
    }
}
