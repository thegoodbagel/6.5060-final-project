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

We implement and compare four algorithms:

1. Limited Adaptive Rounds: The deterministic algorithm of Black et al. \[1\], with configurable adaptivity.
2. Edge Ordering: Orders element pairs by increasing proxy similarity and performs oracle queries accordingly \[3\].
3. Node Ordering: Orders elements by increasing aggregate similarity scores \[3\].
4. Side-Information-Assisted Learning: Uses proxy similarities to improve query efficiency, following \[2\].

The Node Ordering algorithm is approximate, while the other algorithms target exact partition recovery under their respective assumptions.

## Repository Structure

```text
.
├── CMakeLists.txt
├── README.md
├── requirements.txt    # Python analysis dependencies
│
├── include/
│   ├── core/           # Partition, oracle, proxy, metrics, interfaces
│   ├── algorithms/     # Algorithm implementations
│   ├── generators/     # Synthetic data and proxy generators
│   └── benchmarks/     # Experiment runner
│
├── src/
│   ├── algorithms/     # Algorithm source files
│   ├── generators/     # Data generation implementations
│   ├── oracle/         # Oracle implementations
│   ├── proxy/          # Proxy implementations
│   └── main.cpp
│
├── tests/              # Unit and integration tests
├── configs/            # Experiment configurations
├── datasets/
│   ├── raw/            # Original datasets
│   └── processed/      # Preprocessed datasets
│
├── docs/
│   ├── tasks.md        # Task tracker and project plan
│   ├── digest.md       # Short summary of the current system
│   └── archive.md      # Detailed setup history, run steps, issue log
│
├── scripts/
│   ├── setup_linux.sh  # One-shot Linux environment setup
│   └── submit_slurm.sh # Slurm job script (Engaging)
├── results/
│   ├── raw/            # Raw experiment outputs
│   ├── processed/      # Aggregated results
│   └── plots/          # Generated figures
│
└── analysis/           # Python analysis and plotting scripts
```

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

Run an experiment using a configuration file:

```bash
./build-release/partition_experiment \
  --config configs/synthetic_scaling.json \
  --output results/raw/results.csv
```

Experiment configurations specify the dataset, partition parameters, proxy distribution, algorithm, and execution settings.

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

Project documentation lives in `docs/`:

- [`docs/digest.md`](docs/digest.md): short summary of the current system, key commands, and conventions. Start here.
- [`docs/tasks.md`](docs/tasks.md): task tracker organized by phase, with status, priorities, and division of work.
- [`docs/archive.md`](docs/archive.md): detailed record of setup steps, how to run things on macOS and Engaging, the process log, and issues encountered with their fixes.

## Reproducibility

Experiments use configurable random seeds and JSON configuration files. Results should record relevant parameters, compiler settings, and execution environment information (compiler version and flags, CPU model, git commit hash) to facilitate reproducibility.

Large-scale experiments are intended to run on the [MIT Engaging cluster](https://orcd.mit.edu/resources/about-engaging-cluster), using Slurm for job scheduling.

## References

\[1\] Black et al. (2025). Learning with Limited Adaptive Rounds.

\[2\] Mazumdar et al. (2017). Query Complexity of Clustering with Side Information.

\[3\] Mazumdar et al. (2017). Theoretical analysis of Edge Ordering and Node Ordering algorithms.

\[4\] Mazumdar et al. (2017). Clustering with Noisy Oracle Queries.

See the original project bibliography for the full references.

AI assistance was used in organizing the initial project documentation.
