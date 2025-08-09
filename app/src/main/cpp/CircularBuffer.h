#ifndef AUDIOAPP_CIRCULARBUFFER_H
#define AUDIOAPP_CIRCULARBUFFER_H

#include <vector>
#include <atomic>

template<typename T>
class CircularBuffer {
public:
    CircularBuffer(int size) : buffer(size), head(0), tail(0) {}

    bool write(const T& value) {
        int nextHead = (head + 1) % buffer.size();
        if (nextHead == tail) {
            return false; // Buffer is full
        }
        buffer[head] = value;
        head = nextHead;
        return true;
    }

    bool read(T& value) {
        if (head == tail) {
            return false; // Buffer is empty
        }
        value = buffer[tail];
        tail = (tail + 1) % buffer.size();
        return true;
    }

    T read() {
        if (head == tail) {
            return T{}; // Buffer is empty, return default value
        }
        T value = buffer[tail];
        tail = (tail + 1) % buffer.size();
        return value;
    }

    void resize(int newSize) {
        buffer.resize(newSize);
        head = 0;
        tail = 0;
    }

private:
    std::vector<T> buffer;
    std::atomic<int> head;
    std::atomic<int> tail;
};

#endif //AUDIOAPP_CIRCULARBUFFER_H
