#ifndef AUDIOAPP_LOCKFREEQUEUE_H
#define AUDIOAPP_LOCKFREEQUEUE_H

#include <atomic>
#include <vector>

template<typename T>
class LockFreeQueue {
public:
    LockFreeQueue(int size) : buffer(size), head(0), tail(0) {}

    bool push(const T &value) {
        int current_tail = tail.load(std::memory_order_relaxed);
        int next_tail = (current_tail + 1) % buffer.size();
        if (next_tail == head.load(std::memory_order_acquire)) {
            return false; // full
        }
        buffer[current_tail] = value;
        tail.store(next_tail, std::memory_order_release);
        return true;
    }

    bool pop(T &value) {
        int current_head = head.load(std::memory_order_relaxed);
        if (current_head == tail.load(std::memory_order_acquire)) {
            return false; // empty
        }
        value = buffer[current_head];
        head.store((current_head + 1) % buffer.size(), std::memory_order_release);
        return true;
    }

private:
    std::vector<T> buffer;
    std::atomic<int> head;
    std::atomic<int> tail;
};

#endif //AUDIOAPP_LOCKFREEQUEUE_H
