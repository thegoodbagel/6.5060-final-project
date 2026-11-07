# Digest: Current System

*Last updated: Nov 12, 2026. Phase 2 core framework and first end-to-end experiment completed.*

## What this is

A C++20 experimental framework for comparing partition-learning algorithms using pairwise same-cluster oracle queries, optionally assisted by noisy similarity proxies.

Four research algorithms are planned: Black et al. limited-round, Edge Ordering, Node Ordering, and Side-Information-Assisted Learning.

Synthetic benchmarks come first, followed by WDC Products.

## Current state

- Repository scaffold, CMake, Catch2, and optional OpenMP are configured.
- Builds and tests pass locally (macOS) and on MIT Engaging.
- A Slurm smoke-test job completed successfully on `mit_quicktest` with 4 allocated cores.
- Core components implemented:
  - `Partition`: cluster assignments and membership operations.
  - `Oracle` / `ExactOracle`: pairwise membership queries and query accounting.
  - `Proxy` / `SyntheticProxy`: deterministic noisy similarities, optional caching, and access/computation counters.
  - `Algorithm`: common interface and standardized results.
  - `RepresentativeBaseline`: sequential exact-recovery reference algorithm.
  - `Metrics`: exact recovery, pairwise precision/recall/F1, and Adjusted Rand Index.
  - Milestone experiment runner: synthetic partition generation, algorithm execution, evaluation, and CSV export.
- All 16 Catch2 test cases pass (86 assertions).
- First end-to-end experiment successfully completed.
- No research-paper algorithms implemented yet.
- **Next: Phase 3 — configurable synthetic benchmark generation.**

## Verified Experiment

Configuration:

- Algorithm: `representative_baseline`
- Dataset: `balanced_synthetic`
- n = 12, k = 3, seed = 42

Results:

| Metric | Value |
|---|---|
| Exact recovery | Yes |
| Oracle queries | 21 |
| Unique oracle queries | 21 |
| Proxy accesses / computations | 0 / 0 |
| Adaptive rounds | 21 |
| Pairwise precision / recall / F1 | 1 / 1 / 1 |
| Adjusted Rand Index | 1 |
| Runtime | 0.005833 ms |

CSV successfully written to `results/raw/milestone1.csv`.

## How to use it

```bash
# Build and test
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release -j
./build-release/unit_tests

# Run baseline experiment
mkdir -p results/raw
./build-release/partition_experiment \
  --n 12 --k 3 --seed 42 \
  --output results/raw/milestone1.csv

# Inspect output
cat results/raw/milestone1.csv

# Engaging: build on login node, run via Slurm
module load gcc cmake miniforge
bash scripts/setup_linux.sh --build
mkdir -p logs
sbatch scripts/submit_slurm.sh \
  --n 12 --k 3 --seed 42 \
  --output results/raw/milestone1.csv
```

## Key conventions

- Algorithms interact with oracle/proxy interfaces, never ground-truth labels.
- Cluster labels are arbitrary; recovery accuracy is label-invariant.
- Oracle invocations and unique queried pairs are tracked separately.
- Proxy accesses and actual score computations are tracked separately.
- Synthetic proxy scores are deterministic for a fixed seed and pair.
- The representative baseline uses sequential oracle queries and does not access the proxy.
- Adaptive rounds are tracked explicitly.
- OpenMP is optional.
- Use separate debug and release builds; benchmark only release builds.
- Build on Engaging login nodes; run experiments through Slurm.
- Current CSV output overwrites the specified file.

## Known issues and notes

- The initial baseline unit test expected 10 oracle queries but the implementation produced 9. Corrected the test assertion; all tests now pass.
- Interactive zsh initially interpreted pasted `#` comment lines as commands. Fixed using `setopt interactivecomments`.
- The representative baseline is a correctness reference, not one of the four research algorithms.
- The current experiment runner uses a balanced synthetic partition. General synthetic generators are not implemented yet.
- The current end-to-end experiment has been verified locally; the updated runner has not yet been independently verified on Engaging.

## Next Steps

1. Implement configurable partition distributions: balanced, Zipf, power-law, one-large-cluster, singleton-heavy.
2. Implement Gaussian, uniform, and beta proxy distributions.
3. Compute squared Hellinger divergence and proxy-quality metrics.
4. Test determinism, distribution validity, and scalability without dense O(n²) storage.
5. Extract and verify the exact pseudocode for the four research algorithms.

## See Also

- `docs/tasks.md` — detailed project plan and checklist.
- `docs/archive.md` — implementation history, technical decisions, test results, and issue log.

