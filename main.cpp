#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <random>
#include <atomic>
#include "ready_queue.h"
#include "worker.h"

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

    std::cout << "Tasks completed: " << tasks_completed.load() << "/" << NUM_TASKS << "\n";
    std::cout << "Sum of burst times: " << serial_ms << " ms (serial baseline)\n";
    std::cout << "Actual wall-clock time: " << wall_ms << " ms (" << NUM_WORKERS << " workers)\n";
    std::cout << "Effective speedup: " << speedup << "x\n";

    return 0;
}
