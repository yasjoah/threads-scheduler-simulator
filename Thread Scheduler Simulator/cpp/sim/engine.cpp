#include "engine.hpp"
#include <algorithm>

static std::vector<Job> generate_jobs(const std::vector<Task>& tasks, int sim_ms) {
    std::vector<Job> jobs;
    for (const auto& t : tasks) {
        int job_counter = 1;
        for (int rel = t.phase_ms; rel < sim_ms; rel += t.period_ms) {
            Job j;
            j.task_id = t.id;
            j.job_id = job_counter++;
            j.release_time = rel;
            j.abs_deadline = rel + t.deadline_ms;
            j.remaining_time = t.wcet_ms;
            jobs.push_back(j);
        }
    }

    std::sort(jobs.begin(), jobs.end(), [](const Job& a, const Job& b) {
        if (a.release_time != b.release_time) return a.release_time < b.release_time;
        if (a.task_id != b.task_id) return a.task_id < b.task_id;
        return a.job_id < b.job_id;
    });

    return jobs;
}

SimResult run_simulation_tick_1ms(
    const std::vector<Task>& tasks,
    const SimConfig& cfg,
    const PickerFn& pick_next_job,
    bool is_round_robin
) {
    SimResult res;
    res.jobs = generate_jobs(tasks, cfg.sim_ms);

    int current_job_idx = -1;

    // RR support: track quantum usage for current running job
    int rr_quantum_left = cfg.rr_quantum_ms;

    std::string current_label = "IDLE";
    int seg_start = 0;

    auto label_for = [&](int idx) -> std::string {
        return (idx == -1) ? "IDLE" : res.jobs[idx].label();
    };

    for (int t = 0; t < cfg.sim_ms; ++t) {
        int next_idx = pick_next_job(res.jobs, t);

        // If RR: enforce quantum expiration by forcing a “switch away”
        // The RR picker itself will rotate; this just detects quantum boundaries.
        if (is_round_robin && current_job_idx != -1 && current_job_idx == next_idx) {
            // continue same job unless quantum ends
            if (rr_quantum_left <= 0) {
                // allow picker to choose someone else (it should rotate)
                next_idx = pick_next_job(res.jobs, t); // picker should already handle
            }
        }

        std::string next_label = label_for(next_idx);

        if (t == 0) {
            current_job_idx = next_idx;
            current_label = next_label;
            seg_start = 0;
            rr_quantum_left = cfg.rr_quantum_ms;
        } else if (next_label != current_label) {
            // close old segment
            res.timeline.push_back({seg_start, t, (current_label == "IDLE" ? "IDLE" : "RUN"), current_label});
            seg_start = t;

            // preemption accounting (only if old job not finished)
            if (current_job_idx != -1 && next_idx != current_job_idx) {
                if (res.jobs[current_job_idx].remaining_time > 0) {
                    res.jobs[current_job_idx].preemptions += 1;
                }
            }

            current_job_idx = next_idx;
            current_label = next_label;
            rr_quantum_left = cfg.rr_quantum_ms; // reset quantum on switch
        }

        // Execute 1ms
        if (current_job_idx != -1) {
            Job& j = res.jobs[current_job_idx];
            if (j.release_time <= t && j.remaining_time > 0) {
                if (j.start_time == -1) j.start_time = t;

                j.remaining_time -= 1;
                rr_quantum_left -= 1;

                if (j.remaining_time == 0) {
                    j.finish_time = t + 1;
                    if (j.finish_time > j.abs_deadline) j.missed_deadline = true;
                    rr_quantum_left = cfg.rr_quantum_ms; // next job gets full quantum
                }
            }
        } else {
            rr_quantum_left = cfg.rr_quantum_ms;
        }
    }

    // close final segment
    res.timeline.push_back({seg_start, cfg.sim_ms, (current_label == "IDLE" ? "IDLE" : "RUN"), current_label});
    return res;
}
