# Digest: Current System

*Last updated: Nov 12, 2026. Setup phase only; no algorithms implemented yet.*

## What this is

A C++20 experimental framework for comparing partition-learning algorithms that use pairwise same-cluster oracle queries, optionally with noisy similarity ("proxy") side information. Four algorithms are planned: Black et al. limited-round, edge ordering, node ordering, and the side-information algorithm. Synthetic data comes first, then WDC Products.

## Current state

- Repo scaffold, CMake build, Catch2 smoke test, and a minimal `partition_experiment` executable are in place.
- Builds and passes tests locally (macOS) and on MIT Engaging (GCC 12.2.0, CMake 4.1.2, miniforge Python).
- A test Slurm job ran successfully on `mit_quicktest` (4 cores, `OMP_NUM_THREADS=4` confirmed).
- No algorithm, oracle, proxy, or generator code exists yet. Next up: core interface headers (Phase 2).

## Layout

```
include/{core,algorithms,generators,benchmarks}   headers
src/{algorithms,generators,oracle,proxy}, main.cpp
tests/   configs/   datasets/{raw,processed}   results/{raw,processed,plots}
scripts/setup_linux.sh   scripts/submit_slurm.sh
analysis/   docs/   requirements.txt   CMakeLists.txt
```

Datasets, results, logs, build directories, and `.venv` are gitignored.

## How to use it

```bash
# Build and test (macOS or Linux)
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release -j
./build-release/unit_tests
./build-release/partition_experiment

# Linux / Engaging one-shot environment setup
bash scripts/setup_linux.sh --build

# Engaging (login node: build; Slurm: run)
ssh <kerberos>@orcd-login.mit.edu
module load gcc cmake miniforge
mkdir -p logs && sbatch scripts/submit_slurm.sh
squeue --me
```

## Key conventions

- Algorithms interact only with `Oracle` and `Proxy` interfaces; only the oracle sees ground-truth labels.
- OpenMP is optional; the project builds single-threaded without it.
- Separate debug/sanitizer builds (`build-debug`) from performance builds (`build-release`).
- Never run experiments on login nodes; use Slurm. Build on the login node (CMake downloads Catch2).
- `setup_linux.sh` only prepares the environment; it does not create tracked files.

## Gotchas

- Engaging's default `python3` is 3.6; run `module load miniforge` first.
- A `.bashrc` line that auto-launches an Apptainer container will break logins (details in `archive.md`).
- `logs/` must exist before `sbatch`, or the job fails without output.

## See also

`docs/tasks.md` for the task tracker and `docs/archive.md` for the detailed setup history and issue log.
