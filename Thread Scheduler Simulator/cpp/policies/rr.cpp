#include "policies.hpp"
#include <algorithm>

// Helper: add newly released jobs into RR ready list if not present
static void rr_refresh_ready(std::vector<Job>& jobs, int current_time, RRState& st) {
    for (int i = 0; i < (int)jobs.size(); ++i) {
        auto& j = jobs[i];
        if (j.remaining_time <= 0) continue;
        if (j.release_time > current_time) continue;

        if (std::find(st.ready.begin(), st.ready.end(), i) == st.ready.end()) {
            st.ready.push_back(i);
        }
    }

    // remove finished jobs
    st.ready.erase(std::remove_if(st.ready.begin(), st.ready.end(),
                  [&](int idx){ return jobs[idx].remaining_time <= 0; }),
                  st.ready.end());
}

int pick_rr(std::vector<Job>& jobs, int current_time, RRState& st) {
    rr_refresh_ready(jobs, current_time, st);
    if (st.ready.empty()) return -1;

    // If last_idx is not in ready, reset to front
    auto it = std::find(st.ready.begin(), st.ready.end(), st.last_idx);
    int pos = (it == st.ready.end()) ? -1 : (int)std::distance(st.ready.begin(), it);

    // rotate to next
    int next_pos = (pos + 1) % (int)st.ready.size();
    st.last_idx = st.ready[next_pos];
    return st.last_idx;
}
