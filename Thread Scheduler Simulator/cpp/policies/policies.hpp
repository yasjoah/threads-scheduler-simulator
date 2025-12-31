#pragma once
#include "../scheduler.hpp"

// Each policy returns the index of the next job to run, or -1 for IDLE.
int pick_fcfs(const std::vector<Job>& jobs, int current_time);
int pick_edf(const std::vector<Job>& jobs, int current_time);
int pick_rm(const std::vector<Job>& jobs, int current_time, const std::vector<Task>& tasks);

// Round Robin needs a small state (ready queue rotation), so we wrap it:
struct RRState {
    int quantum_ms{5};
    int last_idx{-1};
    std::vector<int> ready; // indices of jobs in rotation order
};

int pick_rr(std::vector<Job>& jobs, int current_time, RRState& st);
