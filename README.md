Real-Time OS Scheduler Simulator

A kernel-level scheduling simulator that models how a single-core CPU executes periodic real-time tasks under different scheduling policies. 
The simulator implements preemptive real-time schedulers in C++ and provides a Python-based analysis pipeline to visualize latency, jitter, and deadline behavior.
This project is designed to mirror how operating systems and embedded kernels reason about timing, preemption, and schedulability in mission-critical systems.

Features:
Preemptive scheduling engine implemented in C++
Supports multiple scheduling policies:
Earliest Deadline First (EDF)
Rate-Monotonic Scheduling (RM)
First-Come, First-Served (FCFS)

Models:
Periodic real-time tasks
Job releases and deadlines
Context switches and preemptions
Deadline misses and slack
Generates full CPU execution traces (Gantt timelines)
Python analytics pipeline for visualization and benchmarking


Scheduling Policies
Earliest Deadline First (EDF):
A dynamic-priority scheduler that always executes the job with the closest deadline. EDF is optimal for single-core real-time systems: if a task set is schedulable, EDF will schedule it successfully.

Rate-Monotonic (RM):
A fixed-priority scheduler where tasks with shorter periods receive higher priority. RM is widely used in safety-critical systems due to its predictability and formal guarantees.

FCFS:
A non-preemptive baseline scheduler used for comparison.

How It Works:
Tasks are defined by:
Period
Worst-case execution time
Deadline
Phase offset
Tasks generate jobs over time.
The simulator advances in 1 ms ticks, selecting which job runs based on the chosen policy.
Execution decisions, preemptions, and completions are logged.
Results are written to CSV files for analysis.


Building the Simulator
Requirements:
C++17 compiler (GCC via MSYS2 or equivalent)
Compile
From the cpp/ directory:
g++ -std=c++17 -O2 main.cpp sim/engine.cpp policies/fcfs.cpp policies/edf.cpp policies/rm.cpp io/csv.cpp -o sim
Run
./sim edf
./sim rm
./sim fcfs

Each run generates timeline.csv and jobs.csv.

Analysis & Visualization
Python Requirements
pip install pandas numpy matplotlib
Generate Plots

From the repository root:

python python/analyze.py

This produces:

gantt.png — CPU execution timeline

response_times.png — Response time per task

jitter.png — Jitter per task

deadlines.png — Deadline slack and misses

Example Outputs

The analysis pipeline visualizes:
CPU scheduling behavior (Gantt chart);
Latency and response times;
Jitter under preemption;
Deadline slack and misses under overload;
These metrics are standard in real-time OS evaluation and embedded systems design.

Motivation:

Modern embedded and autonomous systems—such as robotics, aerospace, and high-frequency trading infrastructure—require software that meets strict timing constraints. This project was built to gain hands-on experience with real-time kernel scheduling, moving beyond application-level programming into systems-level reasoning.

Technologies Used:

C++ (systems programming, simulation)

Python (data analysis and visualization)

Pandas, NumPy, Matplotlib

Real-Time Scheduling Theory (EDF, RM)

Discrete-Time Simulation

Future Extensions

Multi-core scheduling (global EDF vs partitioned RM)

Priority inversion and priority inheritance

Utilization-based schedulability tests

Event-driven (non-tick) simulation

Hardware-in-the-loop task execution

Author

Developed as a personal systems project to explore operating system kernels, real-time scheduling, and embedded software design.
