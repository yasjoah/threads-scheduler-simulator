#!/usr/bin/env python3
"""
analyze.py
Reads outputs/timeline.csv and outputs/jobs.csv and produces:
 - outputs/gantt.png
 - outputs/response_times.png
 - outputs/jitter.png
 - outputs/deadlines.png

Run (from repo root):
  python python/analyze.py
Optional:
  python python/analyze.py --policy edf
"""

import argparse
import os
import re
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

JOB_RE = re.compile(r"^T(?P<task>\d+)J(?P<job>\d+)$")

def repo_root() -> str:
    # this file lives in repo_root/python/analyze.py
    return os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

def parse_job_label(label: str):
    if not isinstance(label, str):
        return None, None
    m = JOB_RE.match(label.strip())
    if not m:
        return None, None
    return int(m.group("task")), int(m.group("job"))

def read_inputs(root: str, policy: str | None):
    out_dir = os.path.join(root, "outputs")

    # Support either default filenames or policy-suffixed filenames, if you add that later.
    timeline_path = os.path.join(out_dir, "timeline.csv") if not policy else os.path.join(out_dir, f"timeline_{policy}.csv")
    jobs_path     = os.path.join(out_dir, "jobs.csv")     if not policy else os.path.join(out_dir, f"jobs_{policy}.csv")

    if not os.path.exists(timeline_path):
        raise FileNotFoundError(f"Missing {timeline_path}. Run your simulator first to generate outputs.")
    if not os.path.exists(jobs_path):
        raise FileNotFoundError(f"Missing {jobs_path}. Run your simulator first to generate outputs.")

    timeline = pd.read_csv(timeline_path)
    jobs = pd.read_csv(jobs_path)

    return out_dir, timeline, jobs

def make_gantt(out_dir: str, timeline: pd.DataFrame):
    # Keep only RUN segments
    tl = timeline.copy()
    tl = tl[tl["cpu_state"].astype(str).str.upper() == "RUN"].copy()
    if tl.empty:
        print("No RUN segments found in timeline.csv (CPU may have been idle). Skipping gantt.")
        return

    # Parse task/job from label
    parsed = tl["job"].apply(parse_job_label)
    tl["task_id"] = [t for t, _ in parsed]
    tl["job_id"] = [j for _, j in parsed]
    tl = tl.dropna(subset=["task_id"]).copy()
    tl["task_id"] = tl["task_id"].astype(int)

    # Sort tasks to assign y-positions
    task_ids = sorted(tl["task_id"].unique().tolist())
    y_map = {tid: i for i, tid in enumerate(task_ids)}

    plt.figure(figsize=(12, max(3, 0.6 * len(task_ids) + 2)))
    ax = plt.gca()

    # draw one horizontal bar per segment on that task's row
    for _, row in tl.iterrows():
        y = y_map[int(row["task_id"])]
        start = float(row["time_start"])
        end = float(row["time_end"])
        ax.barh(y=y, width=end - start, left=start, height=0.6)

    ax.set_yticks(range(len(task_ids)))
    ax.set_yticklabels([f"Task {tid}" for tid in task_ids])
    ax.set_xlabel("Time (ms)")
    ax.set_title("CPU Schedule (Gantt)")
    ax.grid(True, axis="x", linestyle="--", linewidth=0.5)

    path = os.path.join(out_dir, "gantt.png")
    plt.tight_layout()
    plt.savefig(path, dpi=200)
    plt.close()
    print(f"Wrote {path}")

def make_response_times(out_dir: str, jobs: pd.DataFrame):
    df = jobs.copy()
    # keep completed jobs only
    df = df[df["response"] >= 0].copy()
    if df.empty:
        print("No completed jobs found (response < 0). Skipping response_times.")
        return

    # plot response time vs release, one series per task
    task_ids = sorted(df["task_id"].unique().tolist())

    plt.figure(figsize=(12, 6))
    ax = plt.gca()

    for tid in task_ids:
        d = df[df["task_id"] == tid].sort_values("release")
        ax.plot(d["release"], d["response"], marker="o", linestyle="-", label=f"Task {tid}")

    ax.set_xlabel("Release time (ms)")
    ax.set_ylabel("Response time (ms)")
    ax.set_title("Response Time vs Release")
    ax.grid(True, linestyle="--", linewidth=0.5)
    ax.legend()

    path = os.path.join(out_dir, "response_times.png")
    plt.tight_layout()
    plt.savefig(path, dpi=200)
    plt.close()
    print(f"Wrote {path}")

def make_jitter(out_dir: str, jobs: pd.DataFrame):
    df = jobs.copy()
    df = df[df["response"] >= 0].copy()
    if df.empty:
        print("No completed jobs found. Skipping jitter.")
        return

    # jitter per task: max(response) - min(response)
    g = df.groupby("task_id")["response"]
    jitter = (g.max() - g.min()).reset_index()
    jitter.columns = ["task_id", "jitter_ms"]

    plt.figure(figsize=(10, 5))
    ax = plt.gca()
    ax.bar(jitter["task_id"].astype(str), jitter["jitter_ms"])
    ax.set_xlabel("Task")
    ax.set_ylabel("Jitter (ms) = max(response) - min(response)")
    ax.set_title("Jitter per Task")
    ax.grid(True, axis="y", linestyle="--", linewidth=0.5)

    path = os.path.join(out_dir, "jitter.png")
    plt.tight_layout()
    plt.savefig(path, dpi=200)
    plt.close()
    print(f"Wrote {path}")

def make_deadlines(out_dir: str, jobs: pd.DataFrame):
    df = jobs.copy()
    # Only jobs that finished
    df = df[df["finish"] >= 0].copy()
    if df.empty:
        print("No finished jobs found. Skipping deadlines plot.")
        return

    # Scatter: finish vs deadline; highlight misses by plotting finish-deadline
    df["slack"] = df["deadline"] - df["finish"]  # positive = met deadline
    misses = df[df["missed"] == 1]
    meets = df[df["missed"] == 0]

    plt.figure(figsize=(12, 6))
    ax = plt.gca()

    # Plot slack vs release; negative slack = missed
    ax.scatter(meets["release"], meets["slack"], marker="o", label="Met deadline")
    ax.scatter(misses["release"], misses["slack"], marker="x", label="Missed deadline")

    ax.axhline(0, linewidth=1)
    ax.set_xlabel("Release time (ms)")
    ax.set_ylabel("Slack (ms) = deadline - finish (negative means miss)")
    ax.set_title("Deadline Slack over Time")
    ax.grid(True, linestyle="--", linewidth=0.5)
    ax.legend()

    path = os.path.join(out_dir, "deadlines.png")
    plt.tight_layout()
    plt.savefig(path, dpi=200)
    plt.close()
    print(f"Wrote {path}")

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--policy", type=str, default=None,
                    help="If you name outputs like timeline_edf.csv and jobs_edf.csv, pass --policy edf")
    args = ap.parse_args()

    root = repo_root()
    out_dir, timeline, jobs = read_inputs(root, args.policy)

    # Basic sanity
    required_tl = {"time_start", "time_end", "cpu_state", "job"}
    required_jobs = {"task_id", "job_id", "release", "start", "finish", "deadline", "response", "missed", "preemptions"}
    if not required_tl.issubset(set(timeline.columns)):
        raise ValueError(f"timeline.csv missing columns: {sorted(required_tl - set(timeline.columns))}")
    if not required_jobs.issubset(set(jobs.columns)):
        raise ValueError(f"jobs.csv missing columns: {sorted(required_jobs - set(jobs.columns))}")

    make_gantt(out_dir, timeline)
    make_response_times(out_dir, jobs)
    make_jitter(out_dir, jobs)
    make_deadlines(out_dir, jobs)

    print("Done.")

if __name__ == "__main__":
    main()
