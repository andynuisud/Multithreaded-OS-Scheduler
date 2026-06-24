#pragma once

struct Task {
    int task_id;
    int burst_time_ms;
    int priority;

    bool operator<(const Task& other) const {
        return priority > other.priority;
    }
};
