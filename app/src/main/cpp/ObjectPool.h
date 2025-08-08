#ifndef AUDIOAPP_OBJECTPOOL_H
#define AUDIOAPP_OBJECTPOOL_H

#include <vector>
#include <memory>
#include <mutex>

template <typename T>
class ObjectPool {
public:
    ObjectPool() = default;
    ~ObjectPool() = default;

    std::unique_ptr<T> get() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pool_.empty()) {
            return std::make_unique<T>();
        }
        std::unique_ptr<T> obj = std::move(pool_.back());
        pool_.pop_back();
        return obj;
    }

    void release(std::unique_ptr<T> obj) {
        std::lock_guard<std::mutex> lock(mutex_);
        pool_.push_back(std::move(obj));
    }

private:
    std::vector<std::unique_ptr<T>> pool_;
    std::mutex mutex_;
};

#endif //AUDIOAPP_OBJECTPOOL_H
