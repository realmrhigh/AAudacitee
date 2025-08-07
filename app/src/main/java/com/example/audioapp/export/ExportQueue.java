package com.example.audioapp.export;

import java.util.concurrent.PriorityBlockingQueue;

public class ExportQueue {
    private final PriorityBlockingQueue<ExportJob> queue = new PriorityBlockingQueue<>();
    private final FileExporter exporter;
    private boolean isRunning = false;

    public ExportQueue(FileExporter exporter) {
        this.exporter = exporter;
    }

    public void addJob(ExportJob job) {
        queue.put(job);
    }

    public void start() {
        if (isRunning) {
            return;
        }
        isRunning = true;
        new Thread(() -> {
            while (isRunning) {
                try {
                    ExportJob job = queue.take();
                    exporter.export(job.getInputBuffer(), job.getPath(), job.getListener(), job.getToken());
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
