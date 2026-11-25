# FreeRTOS Multi-Core Criticality-Urgency Quadrant Model Demo

Reference implementation of the **Multi-Core Extension of the Criticality-Urgency Quadrant Model** for partitioned scheduling and quadrant-aware core allocation on **FreeRTOS (Linux POSIX port)**.

## Associated Publication

> **Azad Mohammed Shaik**, *"Multi-Core Extension of the Criticality-Urgency Quadrant Model for Automotive Real-Time Systems,"* IEEE International Conference on Power, Control and Computing Technologies (IC2PCT 2026), 2026.

## Overview

Evaluates multi-core partitioned task allocation strategies for mixed-criticality automotive workloads (4-core and 8-core configurations):
- **Quadrant-Based Core Dedication:** Isolates high-criticality Q1/Q2 tasks on dedicated cores from Q3/Q4 best-effort workloads
- **PU-WFD (Priority-Urgency Worst-Fit Decreasing):** Balances per-core utilization while respecting quadrant priority ordering

## Repository Structure

- `src/freertos_quadrant_demo_smp.c` — Multi-core partitioned quadrant scheduler implementation (`NUM_CORES`, `USE_PU_WFD_ALLOCATION`)
- `src/FreeRTOSConfig_linux.h`, `src/FreeRTOSConfig_smp.h`, `src/FreeRTOSConfig.h` — FreeRTOS POSIX & SMP configuration headers
- `run_smp_tests.sh` — Runs both Core Dedication and PU-WFD strategies and collects statistics
- `analyze_results.py` — Analysis and visualization script for load balance and deadline metrics
- `Makefile` — Build automation including FreeRTOS-Kernel fetch

## Quick Start (Linux / WSL)

### Prerequisites
- `gcc`, `make`, `git`, POSIX `pthread` / `librt`
- Optional (for plotting): Python 3 with `matplotlib`, `numpy`

### Build & Run
```bash
# 1. Fetch FreeRTOS-Kernel V11.1.0
make fetch-freertos

# 2. Build 4-core (or 8-core) demo
make smp
# or: make smp-8core

# 3. Run automated comparison (Core Dedication vs. PU-WFD)
chmod +x run_smp_tests.sh
./run_smp_tests.sh

# 4. Generate plots
python3 analyze_results.py
```

## Citation

```bibtex
@inproceedings{shaik2026multicore_cuqm,
  author    = {Shaik, Azad Mohammed},
  title     = {Multi-Core Extension of the Criticality-Urgency Quadrant Model for Automotive Real-Time Systems},
  booktitle = {Proceedings of the IEEE International Conference on Power, Control and Computing Technologies (IC2PCT 2026)},
  year      = {2026}
}
```

## License

This project is released under the [MIT License](LICENSE). FreeRTOS-Kernel is fetched separately under its upstream MIT license.
