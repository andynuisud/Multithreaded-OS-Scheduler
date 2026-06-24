#include "ready_queue.h"

void ReadyQueue::push(Task t) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        heap_.push(t);
    }
    cv_.notify_one();
}

bool ReadyQueue::pop(Task& out) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !heap_.empty() || shutdown_; });

    if (heap_.empty() && shutdown_) {
        return false;
    }

    out = heap_.top();
    heap_.pop();
    return true;
}

void ReadyQueue::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        shutdown_ = true;
    }
    cv_.notify_all();
}
