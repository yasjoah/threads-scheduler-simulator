#pragma once
#include "../scheduler.hpp"
#include <functional>

using PickerFn = std::function<int(const std::vector<Job>& jobs, int current_time)>;

SimResult run_simulation_tick_1ms(
    const std::vector<Task>& tasks,
    const SimConfig& cfg,
    const PickerFn& pick_next_job,
    bool is_round_robin = false
);
