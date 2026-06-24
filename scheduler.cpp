#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <chrono>
#include <random>
#include <atomic>


struct Task {
    int task_id;
    int burst_time_ms;
    int priority; 

    bool operator<(const Task& other) const {
        return priority > other.priority;
    }
};

class ReadyQueue {
public:
    void push(Task t) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            heap_.push(t);
        }
        cv_.notify_one(); 
    }

    bool pop(Task& out) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !heap_.empty() || shutdown_; });

        if (heap_.empty() && shutdown_) {
            return false;
        }

        out = heap_.top();
        heap_.pop();
        return true;
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            shutdown_ = true;
        }
        cv_.notify_all(); 
    }

private:
    std::priority_queue<Task> heap_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool shutdown_ = false;
};

void worker_loop(int worker_id, ReadyQueue& queue, std::atomic<int>& tasks_completed) {
    (void)worker_id; 
    Task t;
    while (queue.pop(t)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(t.burst_time_ms));
        tasks_completed.fetch_add(1, std::memory_order_relaxed);
    }
}

constexpr int NUM_WORKERS = 16;
constexpr int NUM_TASKS = 200;

int main() {
    ReadyQueue queue;
    std::atomic<int> tasks_completed{0};
    std::atomic<long long> total_burst_ms{0};
    std::vector<std::thread> workers;
    workers.reserve(NUM_WORKERS);

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> burst_dist(1, 50);
    std::uniform_int_distribution<int> priority_dist(0, 4);

    std::vector<Task> tasks;
    tasks.reserve(NUM_TASKS);
    for (int i = 0; i < NUM_TASKS; ++i) {
        Task t{i, burst_dist(rng), priority_dist(rng)};
        tasks.push_back(t);
        total_burst_ms += t.burst_time_ms;
    }

    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < NUM_WORKERS; ++i) {
        workers.emplace_back(worker_loop, i, std::ref(queue), std::ref(tasks_completed));
    }

    for (const auto& t : tasks) {
        queue.push(t);
    }

    queue.shutdown(); 

    for (auto& w : workers) {
        w.join();
    }

    auto end = std::chrono::steady_clock::now();
    auto wall_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    long long serial_ms = total_burst_ms.load();
    double speedup = static_cast<double>(serial_ms) / static_cast<double>(wall_ms);

    std::cout << "Tasks completed:        " << tasks_completed.load() << "/" << NUM_TASKS << "\n";
    std::cout << "Sum of burst times:     " << serial_ms << " ms (serial baseline)\n";
    std::cout << "Actual wall-clock time: " << wall_ms << " ms (" << NUM_WORKERS << " workers)\n";
    std::cout << "Effective speedup:      " << speedup << "x\n";

    return 0;
}
