#include "worker.h"
#include <thread>
#include <chrono>

void worker_loop(int worker_id, ReadyQueue& queue, std::atomic<int>& tasks_completed) {
    (void)worker_id;
    Task t;
    while (queue.pop(t)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(t.burst_time_ms));
        tasks_completed.fetch_add(1, std::memory_order_relaxed);
    }
}
