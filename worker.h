#pragma once

#include <atomic>
#include "ready_queue.h"

void worker_loop(int worker_id, ReadyQueue& queue, std::atomic<int>& tasks_completed);
