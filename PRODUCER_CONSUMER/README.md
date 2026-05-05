## CS471/571 Operating System Concepts - Project, Spring 2026
By: Adam Daif
---

## Overview

This program implements the classic **Producer-Consumer** synchronization problem using **POSIX pthreads** and **POSIX semaphores**.

- **p producers**, each assigned a fixed store ID, randomly generate sales records and place them in a shared circular buffer.
- **c consumers** read items from the buffer and accumulate sales statistics locally.
- Synchronization is enforced via three semaphores: a **mutex** (binary semaphore) for mutual exclusion, an **empty_slots** counting semaphore, and a **full_slots** counting semaphore.
- Simulation ends when **1000 items** have been produced total.
- The program is run for all combinations of p = {2, 5, 10} and c = {2, 5, 10} (9 runs total), and also with different sleep times (5ms, 20ms, 40ms).

---

## Files

| File | Description |
|------|-------------|
| producer_consumer.cpp | Source code (well documented, semaphore usage annotated) |
| producer_consumer | Compiled executable (Linux x86-64) |
| input_config.txt | Input file listing all run configurations (p, c, buffer, sleep, output) |
| run_all.sh | Shell script that reads `input_config.txt` and executes all runs to make it easier
| output_p2_c2_sleep20.txt | Sample output - p=2, c=2, sleep=20ms |
| output_p2_c5_sleep20.txt | Sample output - p=2, c=5, sleep=20ms |
| output_p2_c10_sleep20.txt | Sample output - p=2, c=10, sleep=20ms |
| output_p5_c2_sleep20.txt | Sample output - p=5, c=2, sleep=20ms |
| output_p5_c5_sleep20.txt | Sample output - p=5, c=5, sleep=20ms |
| output_p5_c10_sleep20.txt | Sample output - p=5, c=10, sleep=20ms |
...etc
| README.md | This file |

---

## Requirements

- **OS:** Linux (Ubuntu 20.04+ or equivalent)
- **Compiler:** g++ with C++11 support or later
- **Libraries:** pthreads (`-pthread` flag), POSIX semaphores (included in glibc)

---

## Compilation

```bash
g++ -O2 -pthread -o producer_consumer producer_consumer.cpp
```

---

## Usage

```bash
./producer_consumer <producers> <consumers> <buffer_size> <sleep_ms> <output_file>
```

| Argument | Description |
|----------|-------------|
| producers | Number of producer threads (p) |
| consumers | Number of consumer threads (c) |
| buffer_size | Shared buffer capacity (number of items) |
| sleep_ms | Maximum sleep time per producer between productions (ms) |
| output_file | Path to save results |

---

## Run Commands

**Single run example (p=2, c=2):**
```bash
./producer_consumer 2 2 10 20 output_p2_c2_sleep20.txt
```

**Run all 9 required p×c combinations at once:**
```bash
bash run_all.sh
```

**Compile and run all:**
```bash
g++ -O2 -pthread -o producer_consumer producer_consumer.cpp && bash run_all.sh
```

**Sleep-time comparison runs (p=5, c=5):**
```bash
./producer_consumer 5 5 20 5  output_p5_c5_sleep5.txt
./producer_consumer 5 5 20 20 output_p5_c5_sleep20.txt
./producer_consumer 5 5 20 40 output_p5_c5_sleep40.txt
```

---

## Input File Format (`input_config.txt`)

Each non-comment line specifies one run:
```
producers  consumers  buffer_size  sleep_ms  output_file
```

Example:
```
2  2  20  20  output_p2_c2_sleep20.txt
5  5  20  40  output_p5_c5_sleep40.txt
```

Lines starting with `#` are comments and are ignored.

---

## Shared Variables and Semaphore Usage

The code uses three semaphores to synchronize access to all shared state:

| Semaphore | Type | Initial Value | Purpose |
|-----------|------|---------------|---------|
| `g_mutex` | Binary (sem_t) | 1 | Mutual exclusion - protects buffer, counters, `g_done`, and global stats |
| `g_empty` | Counting (sem_t) | `buffer_size` | Tracks available empty buffer slots; producers wait on this |
| `g_full` | Counting (sem_t) | 0 | Tracks filled buffer slots; consumers wait on this |

**Shared variables (all protected by `g_mutex`):**

| Variable | Description |
|----------|-------------|
| `g_buffer[]` | Circular shared buffer of `SalesRecord` items |
| `g_bufIn`, `g_bufOut` | Buffer insertion and extraction indices |
| `g_totalProduced` | Total items produced so far (by all producers combined) |
| `g_totalConsumed` | Total items consumed so far |
| `g_done` | Boolean flag; set to `true` once 1000 items produced |
| `g_globalSales` | Accumulated total sales (written by consumers at thread end) |
| `g_globalMonthly[]` | Month-wise totals (written by consumers at thread end) |
| `g_globalConsumed` | Global consumed count (written by consumers at thread end) |

**Producer critical section pattern:**
```
sem_wait(&g_empty)    // wait for empty slot
sem_wait(&g_mutex)    // acquire mutual exclusion
  → insert into buffer, increment g_totalProduced
sem_post(&g_mutex)    // release mutual exclusion
sem_post(&g_full)     // signal a new full slot
```

**Consumer critical section pattern:**
```
sem_timedwait(&g_full)  // wait for full slot (with timeout to re-check done)
sem_wait(&g_mutex)      // acquire mutual exclusion
  → extract from buffer, increment g_totalConsumed
sem_post(&g_mutex)      // release mutual exclusion
sem_post(&g_empty)      // signal a new empty slot
```

---

## Each Sales Record Contains

| Field | Range |
|-------|-------|
| Day (DD) | 1–30 |
| Month (MM) | 01–12 |
| Year (YY) | always 16 |
| Store ID | 1 to p (fixed per producer) |
| Register # | 1–6 |
| Sale amount | $0.50 – $999.99 |

---

## Statistics Reported

**Per consumer:**
- Number of items consumed
- Total sales processed
- Month-wise sales breakdown

**Global (aggregated from all consumers):**
- Store-wide total sales
- Month-wise total sales across all stores
- Aggregate sales (all sales combined)
- Total simulation time (wall clock, seconds)

---

## Sleep-Time Comparison Summary (p=5, c=5)

| Sleep Time | Simulation Time |
|------------|----------------|
| 5 ms | ~1.2 seconds |
| 20 ms | ~2.4 seconds |
| 40 ms | ~4.4 seconds |

Longer sleep times cause producers to generate items more slowly, increasing total simulation time proportionally. Buffer contention is reduced with more sleep, while consumers spend more time idle waiting for items.

---

## Notes

- Pseudo-random number generation uses `rand()` seeded with `time(nullptr)`.
- Each run stops exactly when 1000 total items have been produced.
- Each buffer item is consumed by **exactly one** consumer (no duplication).
- Consumer threads use a 25ms timed wait (`sem_timedwait`) to periodically re-check the done flag rather than blocking forever, ensuring clean termination.
