# Digest: Current System

*Last updated November 13, 2026.*

The C++20 framework compares partition-learning algorithms using pairwise
same-cluster oracle queries and optional noisy similarity scores.

## Implemented

- Exact oracle with total and unique pair counts, optional lazy proxy caching,
  and label-invariant recovery, precision, recall, F1, and ARI.
- Synthetic partitions with balanced, uniform, Zipf, power-law, one-large,
  and singleton-heavy cluster sizes.
- Gaussian, uniform, and Beta proxy distributions with independent class
  parameters and optional estimates of squared Hellinger divergence, overlap,
  AUC, and score moments.
- Representative baseline and the deterministic pairwise limited-round
  algorithm of Black, Mazumdar, and Saha (Algorithm 1).

## Run

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release -j
ctest --test-dir build-release --output-on-failure

./build-release/partition_experiment --algorithm black_limited_round \
  --n 500 --k 10 --seed 42 --rounds 3 --output results/raw/black-r3.csv
```

Black's algorithm accepts an upper bound on the number of clusters.
`--k-bound` defaults to the synthetic dataset's `--k`, and the runner rejects
a smaller bound. `--rounds` defaults to 2. The CSV records the requested
budget, bound, actual adaptive rounds, and the pipe-delimited batch sizes.
The representative baseline remains the default and retains its CSV schema.
Use distinct output paths because each run overwrites its CSV.

For the balanced 500-element, 10-cluster dataset with seed 42, both algorithms
recovered exactly. Black used 17,605 queries in two batches (12,655 and 4,950).
The baseline used 2,740 queries and 2,740 rounds. The Black run terminated
early because the representative set met the paper's all-pairs base case.

## Next

- Verify the updated CMake build and Slurm run on MIT Engaging.
- Sweep n, k, and round budgets on fixed seeds and report total and unique
  query counts alongside actual rounds.
- Verify the sources and ordering rules for the remaining research algorithms
  before implementing them.

See `archive.md` for implementation history and `tasks.md` for the project plan.
