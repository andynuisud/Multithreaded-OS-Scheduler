#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include "task.h"

class ReadyQueue {
public:
    void push(Task t);
    bool pop(Task& out);
    void shutdown();

private:
    std::priority_queue<Task> heap_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool shutdown_ = false;
};
