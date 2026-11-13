# Efficient Partition Learning with Side Information and Limited Adaptive Rounds

Abrianna Zhang · abrianna@mit.edu Edward Xiong · eyxiong@mit.edu

## Overview

This project investigates the practical performance of theoretical algorithms for learning unknown partitions using pairwise oracle queries.

Given $n$ elements partitioned into $k$ unknown clusters, an oracle answers whether two elements belong to the same cluster. Algorithms may also have access to side information in the form of approximate pairwise similarities.

We study the effects of:

- Query complexity: How many oracle and proxy queries are required?
- Adaptivity: How does the number of sequential query rounds affect performance?
- Side information: How does proxy quality, measured in part by Hellinger divergence, affect query efficiency?
- Parallelism: Can theoretical query savings translate into practical runtime improvements?

## Algorithms

The representative baseline, Black et al.’s limited-round algorithm, and Edge Ordering are implemented. Node Ordering and Side-Information-Assisted Learning are planned.

1. Limited Adaptive Rounds: The deterministic algorithm of Black et al. \[1\], with configurable adaptivity.
2. Edge Ordering: Processes pairs in descending proxy similarity and uses transitive inference to skip deducible queries \[3\].
3. Node Ordering: Orders elements by descending aggregate similarity scores \[3\].
4. Side-Information-Assisted Learning: Uses proxy similarities to improve query efficiency, following \[2\].

The Node Ordering approximation discussed in [3] concerns query cost.

## Repository Structure

| Path | Contents |
|---|---|
| `include/core/` | Partition, oracle, proxy, and evaluation interfaces |
| `include/algorithms/` | Representative baseline and Black limited-round recovery |
| `include/benchmarks/` | Synthetic partition generator, proxy quality report, and experiment runner |
| `src/main.cpp` | Experiment CLI |
| `tests/` | Catch2 tests |
| `scripts/` | Linux setup and Slurm submission |
| `datasets/`, `results/` | Dataset and output directories |

## Requirements

- C++20 compiler (GCC >= 10 or Clang >= 12)
- CMake >= 3.16
- OpenMP (optional; enables multithreaded execution)
- Catch2 (testing; downloaded automatically by CMake at configure time, so internet access is needed for the first configure)
- Python >= 3.10 with pandas, NumPy, SciPy, and Matplotlib (analysis)

## Setup

### Linux (including MIT Engaging)

After cloning the repository, a setup script checks for the required tools, installs anything missing, verifies that the compiler supports C++20 and OpenMP, and sets up a Python virtual environment. It only prepares your environment and does not create or modify any files tracked by the repository:

```bash
bash scripts/setup_linux.sh            # set up the environment
bash scripts/setup_linux.sh --build    # ...and also build + run tests
```

Options:

| Flag | Effect |
|------|--------|
| `--build` | Configure and build the release version, then run the tests |
| `--no-system-install` | Never call the system package manager; only report what is missing |
| `--skip-python` | Skip the `.venv` and Python dependency installation |

Set `CXX=clang++` to check/use Clang instead of the default `g++`.

What the script does:

- Detects `apt`, `dnf`, `yum`, `pacman`, or `zypper` and installs missing packages (git, compiler, make, cmake, Python + venv) when you have root or `sudo`.
- Without root, it does not install system packages. It reports what is missing so you can load modules instead (see below).
- If the system CMake is older than 3.16, installs a recent CMake into `.venv` via pip. Run `source .venv/bin/activate` before building so it is picked up.
- Creates `.venv` and installs `requirements.txt`.
- Is safe to re-run.

**MIT Engaging / other clusters.** You will not have root, and the default system Python is too old (3.6), so load toolchain modules first (check exact names with `module avail`), then run the script on the **login node**:

```bash
module avail gcc cmake miniforge
module load gcc cmake miniforge   # names may differ; miniforge provides a modern Python
bash scripts/setup_linux.sh --build
```

Build on the login node (CMake downloads Catch2, and compute nodes may not have internet access), but run experiments only through Slurm. See [Running on Engaging](#running-on-engaging).

### macOS

Apple's Clang does not ship OpenMP, so install it separately. OpenMP is optional; without it the project builds single-threaded.

```bash
xcode-select --install
brew install cmake libomp
python3 -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt
```

If CMake does not find `libomp`:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release \
  -DOpenMP_ROOT=$(brew --prefix libomp)
```

## Building

Configure and build a release version:

```bash
cmake -S . -B build-release \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build-release -j
```

To build with OpenMP support, ensure OpenMP is installed and detected by CMake. Without OpenMP, the project can still run in single-threaded mode.

For development and correctness testing (with sanitizers):

```bash
cmake -S . -B build-debug \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"

cmake --build build-debug -j
```

Keep correctness builds (`build-debug`) and performance builds (`build-release`) separate. Only benchmark the release build.

## Running

Run the unit tests:

```bash
./build-release/unit_tests
```

Run Black et al. on the balanced synthetic benchmark:

```bash
./build-release/partition_experiment --algorithm black_limited_round \
  --n 500 --k 10 --seed 42 --rounds 3 \
  --output results/raw/black-r3.csv
```

The representative baseline is the default algorithm. For Black,
`--k-bound` defaults to `--k`. The CSV records the actual round count
and query counts by batch. Use `--help` for partition shapes and proxy options.

### Running on Engaging

Never run heavy experiments on the login node. Submit them through Slurm:

```bash
mkdir -p logs
sbatch scripts/submit_slurm.sh
```

Module names and partitions should be checked against the [Engaging documentation](https://orcd-docs.mit.edu/getting-started/).

## Experiments

We evaluate the algorithms using both synthetic and real-world datasets.

Synthetic experiments

- Vary the number of elements and clusters.
- Compare different cluster-size distributions.
- Vary proxy quality and Hellinger divergence.
- Study query complexity as a function of adaptivity.
- Measure runtime and multithreaded performance.

Real-world experiments

- Primarily use the [WDC Products dataset](https://webdatacommons.org/largescaleproductcorpus/wdc-products/).
- Consider the Cora dataset as an alternative.
- Construct ground-truth partitions from entity labels.
- Evaluate feature-based proxies, including token similarity and TF-IDF cosine similarity.

We record oracle queries, proxy queries, adaptive rounds, runtime, and clustering accuracy where applicable. Results are exported for analysis and visualization using Python.

## Documentation

| Document | What it covers |
|---|---|
| [digest.md](digest.md) | Start here for implemented features, build and run commands, verified results, and next steps. |
| [archive.md](archive.md) | Setup and experiment history. Section 4 covers the framework and baseline. Section 5 explains synthetic partition shapes, proxy distributions, and quality estimates. Sections 6 and 7 cover Black’s limited-round algorithm and Edge Ordering, including their implementation and tests. |
| [tasks.md](tasks.md) | Project plan, phase checklists, and division of work. |

## Reproducibility

Experiments use configurable random seeds and command-line arguments. Results should record relevant parameters, compiler settings, and execution environment information (compiler version and flags, CPU model, git commit hash) to facilitate reproducibility.

Large-scale experiments are intended to run on the [MIT Engaging cluster](https://orcd.mit.edu/resources/about-engaging-cluster), using Slurm for job scheduling.

## References

\[1\] Black, Mazumdar, and Saha (2025). [Learning Partitions with Optimal Query and Round Complexities](https://arxiv.org/abs/2505.05009).

\[2\] Mazumdar et al. (2017). Query Complexity of Clustering with Side Information.

\[3\] Mazumdar and Saha (2017). [A Theoretical Analysis of First Heuristics of Crowdsourced Entity Resolution](https://arxiv.org/abs/1702.01208).

\[4\] Mazumdar et al. (2017). Clustering with Noisy Oracle Queries.

See the original project bibliography for the full references.

AI assistance was used in organizing the initial project documentation.
