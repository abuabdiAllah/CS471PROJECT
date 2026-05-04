## CS471/571 Operating System Concepts - Final Project, Spring 2026

---

## Overview

This program simulates CPU scheduling for 500 processes using two algorithms:
1. **FIFO** (First-In First-Out) - processes are scheduled in the order they arrive.
2. **SJF** (Shortest Job First, Non-Preemptive) - among all currently arrived processes, the one with the shortest burst time runs next.

Input is read from a tab-separated text file (datafile.txt) with columns ArrivalTime and CPUBurstlength. The first 500 processes are used. Statistics are printed to the console and saved to a separate output file for each algorithm.

---

## Files in this directory

| File : Description |
cpu_scheduler.cpp : Source code (well documented) 
cpu_scheduler : Compiled executable (Linux x86-64) 
datafile.txt : Input data - 500+ processes with ArrivalTime and CPUBurstlength 
output_fifo.txt : Sample output - FIFO scheduling 
output_sjf.txt : Sample output - SJF scheduling 
README.md : This file 

---

## Requirements

- **OS:** Linux (Ubuntu 20.04+ or equivalent)
- **Compiler:** g++ with C++11 support or later
- No external libraries required (uses C++ STL only)

---

## Compilation

```bash
g++ -O2 -o cpu_scheduler cpu_scheduler.cpp
```

---

## Usage

```bash
./cpu_scheduler <scheduling_type> <input_file> <output_file>
```

| Argument | Values |
|----------|--------|
| `scheduling_type` | 1 = FIFO, 2 = SJF |
| `input_file` | Path to input data file |
| `output_file` | Path where output will be saved |

---

## Run Commands (copy and paste)

**Run FIFO and save output:**
```bash
./cpu_scheduler 1 datafile.txt output_fifo.txt
```

**Run SJF and save output:**
```bash
./cpu_scheduler 2 datafile.txt output_sjf.txt
```

**Compile and run both in one go:**
```bash
g++ -O2 -o cpu_scheduler cpu_scheduler.cpp && \
./cpu_scheduler 1 datafile.txt output_fifo.txt && \
./cpu_scheduler 2 datafile.txt output_sjf.txt
```

---

## Input File Format

The input file must have a header line (`ArrivalTime  CPUBurstlength`) followed by one process per line, with values separated by whitespace, for example:

```
ArrivalTime	CPUBurstlength
10          22
68          12
98          34
...
```

Only the first 500 processes are used.

---

## Output Format

The program prints and saves:

```
============================================================
  CPU SCHEDULING SIMULATION - FIFO (First-In First-Out)
============================================================

Number of processes:                         500
Total elapsed time (CPU burst units):        14734
Throughput (avg burst time per process):     21.1160
CPU utilization (%):                         71.66%
Average waiting time (burst units):          17.88
Average turnaround time (burst units):       39.00
Average response time (burst units):         17.88

PID   Arrival   Burst   Start     Finish    Wait  Turnaround  Response
...
```

---

## Formulas Used

| Metric | Formula |
|--------|---------|
| Total elapsed time | last_finish_time − first_start_time |
| Throughput | total_burst_time / number_of_processes |
| CPU utilization | total_burst_time / total_elapsed_time × 100% |
| Waiting time | start_time − arrival_time |
| Turnaround time | finish_time − arrival_time |
| Response time | start_time − arrival_time (same as waiting for non-preemptive) |
| Averages | total / number_of_processes |

---

## Definiton/differance between FIFO and SJF

### FIFO
Processes are sorted by arrival time (stable sort  ties preserve input order). The CPU runs each process to completion before moving to the next. If the CPU is idle when a process arrives, time advances to that arrival.

### SJF (Non-Preemptive)
At each scheduling decision point, all processes that have arrived by `current_time` are candidates. The one with the shortest burst time is selected and runs to completion. Ties are broken by earlier arrival time. If no process is ready, time advances to the next arrival.

---

## Sample Results Summary

| Metric | FIFO | SJF |
|--------|------|-----|
| Total elapsed time | 14734 | 14734 |
| CPU utilization | 71.66% | 71.66% |
| Avg waiting time | 17.88 | 14.24 |
| Avg turnaround time | 39.00 | 35.36 |
| Avg response time | 17.88 | 14.24 |

SJF achieves lower average waiting and turnaround times by prioritizing short jobs, reducing the overall queue wait.
