#ifndef AUDIOAPP_CIRCULARBUFFER_H
#define AUDIOAPP_CIRCULARBUFFER_H

#include <vector>
#include <atomic>

class CircularBuffer {
public:
    CircularBuffer(int size) : buffer(size), head(0), tail(0) {}

    bool write(float value) {
        int nextHead = (head + 1) % buffer.size();
        if (nextHead == tail) {
            return false; // Buffer is full
        }
        buffer[head] = value;
        head = nextHead;
        return true;
    }

    bool read(float &value) {
        if (head == tail) {
            return false; // Buffer is empty
        }
        value = buffer[tail];
        tail = (tail + 1) % buffer.size();
        return true;
    }

private:
    std::vector<float> buffer;
    std::atomic<int> head;
    std::atomic<int> tail;
};

#endif //AUDIOAPP_CIRCULARBUFFER_H
