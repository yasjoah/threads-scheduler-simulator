#include "scheduler.hpp"
#include "sim/engine.hpp"
#include "policies/policies.hpp"
#include "io/csv.hpp"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    std::string policy = (argc >= 2) ? argv[1] : "edf";

    // Example tasks (edit these)
    std::vector<Task> tasks = {
        {1, 10, 3, 10, 0, 0},
        {2, 25, 8, 25, 0, 0},
        {3, 40, 6, 40, 5, 0}
    };

    SimConfig cfg;
    cfg.sim_ms = 200;
    cfg.rr_quantum_ms = 5;

    SimResult result;

    if (policy == "edf") {
        result = run_simulation_tick_1ms(tasks, cfg,
            [&](const std::vector<Job>& jobs, int t){ return pick_edf(jobs, t); }
        );
    } else if (policy == "fcfs") {
        result = run_simulation_tick_1ms(tasks, cfg,
            [&](const std::vector<Job>& jobs, int t){ return pick_fcfs(jobs, t); }
        );
    } else if (policy == "rm") {
        result = run_simulation_tick_1ms(tasks, cfg,
            [&](const std::vector<Job>& jobs, int t){ return pick_rm(jobs, t, tasks); }
        );
    } else if (policy == "rr") {
        RRState st;
        st.quantum_ms = cfg.rr_quantum_ms;

        // RR needs mutable state + mutable jobs; we do a tiny wrapper:
        // We'll run the engine with a picker that ignores the "const" jobs by keeping a shadow copy inside.
        // Simpler approach: run RR via a custom simulation (but we’ll keep it minimal here).
        // Easiest: just re-run engine but store jobs in result first, then simulate in-place.
        // For now: we’ll do RR by directly calling the engine in a slightly different way.

        // Hack-free approach: re-run simulation using a local mutable copy inside the picker via static pointer.
        // (Works for single-run CLI tool.)
        static RRState* pst = nullptr;
        pst = &st;

        result = run_simulation_tick_1ms(tasks, cfg,
            [&](const std::vector<Job>& /*jobs_const*/, int /*t*/) {
                return -1; // placeholder; not used
            }
        );

        // RR simulation in-place using engine but with simple loop? (Keep it straightforward.)
        // For simplicity: tell you to use edf/fcfs/rm now; I’ll give you a clean RR-engine variant next if needed.
        std::cerr << "RR in this modular version needs a dedicated RR engine (stateful picker).\n";
        std::cerr << "Use: edf, fcfs, rm for now. If you want RR, say 'make RR clean' and I'll patch it properly.\n";
        return 1;
    } else {
        std::cerr << "Unknown policy: " << policy << "\n";
        std::cerr << "Use one of: edf | fcfs | rm\n";
        return 1;
    }

    // Write outputs (relative paths)
    write_timeline_csv("../outputs/timeline.csv", result.timeline);
    write_jobs_csv("../outputs/jobs.csv", result.jobs);

    // Console summary
    int finished = 0, missed = 0, unfinished = 0;
    for (const auto& j : result.jobs) {
        if (j.finish_time == -1) unfinished++;
        else {
            finished++;
            if (j.missed_deadline) missed++;
        }
    }

    std::cout << "Policy: " << policy << "\n";
    std::cout << "Sim: " << cfg.sim_ms << " ms\n";
    std::cout << "Jobs total: " << result.jobs.size()
              << " | finished: " << finished
              << " | unfinished: " << unfinished
              << " | missed: " << missed << "\n";
    std::cout << "Wrote: outputs/timeline.csv and outputs/jobs.csv\n";
    return 0;
}
