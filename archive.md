# Archive

Detailed record of setup steps, organization, process, and issues. Currently covers the setup phase (Nov 12, 2026). Append new sections as the project progresses.

---

## 1. Organization

### Repository layout

```
.
├── CMakeLists.txt
├── README.md
├── requirements.txt          # Python analysis dependencies
├── include/{core,algorithms,generators,benchmarks}
├── src/{algorithms,generators,oracle,proxy}, main.cpp
├── tests/                    # Catch2 tests (tests/*.cpp globbed by CMake)
├── configs/                  # JSON experiment configs
├── datasets/{raw,processed}  # gitignored contents
├── results/{raw,processed,plots}  # gitignored contents
├── scripts/                  # setup_linux.sh, submit_slurm.sh
├── analysis/                 # Python analysis and plotting
├── docs/                     # tasks.md, digest.md, archive.md
└── logs/                     # Slurm output (gitignored contents)
```

Tracked via `.gitkeep`: `results/*`, `datasets/*`, `logs/`. Ignored: `build*/`, `.venv/`, `__pycache__/`, `.DS_Store`, and the contents of results, datasets, and logs. To share a specific result file, force-add it (`git add -f`).

### Build system (`CMakeLists.txt`)

- C++20, no compiler extensions, default build type Release.
- `find_package(OpenMP)` is optional. If found, it links `OpenMP::OpenMP_CXX` and defines `PL_HAVE_OPENMP=1`; otherwise the build is single-threaded.
- `plcore` interface library carries the include path and `-Wall -Wextra`.
- Executable: `partition_experiment` (from `src/main.cpp`). Tests: `unit_tests` (Catch2 v3.5.2 via `FetchContent`, so the first configure needs internet).

### Design decisions so far

- Algorithms see only `Oracle` and `Proxy` interfaces, never ground-truth labels.
- Correctness builds (debug + sanitizers) and performance builds (release) are kept separate.
- OpenMP is optional so the same code builds on macOS without libomp.
- The setup script never creates or modifies tracked files.
- Raw results and datasets stay out of git.

---

## 2. How to run things

### macOS (local)

```bash
xcode-select --install
brew install cmake libomp                 # libomp only for threading
python3 -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt

cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
# if libomp is not found:
#   -DOpenMP_ROOT=$(brew --prefix libomp)
cmake --build build-release -j
./build-release/unit_tests
./build-release/partition_experiment
```

Debug/sanitizer build (not yet verified):

```bash
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"
cmake --build build-debug -j
```

### Linux / Engaging

```bash
ssh <kerberos>@orcd-login.mit.edu        # Kerberos password, then Duo
cd <repo> && git pull
module avail gcc cmake miniforge
module load gcc cmake miniforge          # use the names avail shows
bash scripts/setup_linux.sh --build
```

`setup_linux.sh` options: `--build` (configure, build, run tests), `--no-system-install` (never call a package manager), `--skip-python`. `CXX=clang++` switches the compiler. What it does: checks git, compiler, make, cmake (>= 3.16), python3 (>= 3.10) and venv; installs missing system packages only if root/sudo and a supported package manager (apt, dnf, yum, pacman, zypper) exist; verifies C++20 and OpenMP by compiling tiny programs; creates `.venv` and installs `requirements.txt`; falls back to a pip-installed cmake in `.venv` if the system cmake is too old; falls back to `g++`/`c++`/`clang++` if `$CXX` points at a missing path. Safe to re-run.

### Running on Engaging via Slurm

Build on the login node, run through Slurm.

```bash
mkdir -p logs                            # must exist before sbatch
sbatch scripts/submit_slurm.sh           # smoke test
sbatch scripts/submit_slurm.sh --config configs/x.json --output results/raw/x.csv
sbatch --cpus-per-task=8 -p mit_normal scripts/submit_slurm.sh ...   # override resources

squeue --me
sacct -o JobID,Partition,AllocCPUS,NodeList,Elapsed,State,ExitCode,MaxRSS -j <jobid>
scancel <jobid>
```

`submit_slurm.sh` defaults: `mit_quicktest`, 5 minutes, 4 cores, 4 GB, output to `logs/%x_%j.{out,err}`. It runs from `$SLURM_SUBMIT_DIR`, loads `gcc` (the binary needs the same GCC runtime libraries it was built with), sets `OMP_NUM_THREADS` from the allocated cores, prints job environment metadata (job id, partition, node, CPUs, CPU model, compiler, git commit, args), then runs `./build-release/partition_experiment "$@"`.

### Engaging reference (from the ORCD docs)

| Partition | Purpose | Max time | Base limit |
|-----------|---------|----------|------------|
| `mit_quicktest` | Short test jobs, CPU only | 15 min | 48 cores |
| `mit_normal` | Longer CPU batch/interactive | 12 h | 96 cores |
| `mit_normal_gpu` | GPU jobs | 6 h | 2 GPUs, 32 cores |
| `mit_preemptable` | Low priority, can be preempted (use `--requeue`) | 48 h | 1024 cores, 4 GPUs |

Older partitions (`sched_*`, `newnodes`) run CentOS 7 and a different software stack; avoid them. Access: `ssh <kerberos>@orcd-login.mit.edu`, or the OnDemand portal at `orcd-ood.mit.edu` (shell, file browser). Compute nodes are reachable only through jobs. Quota report at login: HOME 200 GB, SCRATCH 1 TB, POOL 1 TB.

---

## 3. Process log (Nov 12, 2026)

1. Wrote the project execution plan and task plan; created the GitHub repo and cloned it locally and on Engaging.
2. Added the CMake scaffold, `.gitignore`, minimal `main.cpp`, Catch2 smoke test; local build and tests passed.
3. Wrote `README.md` and `scripts/setup_linux.sh`; made the script environment-only (no skeleton or empty-file creation; `requirements.txt` required rather than generated).
4. First Engaging attempt failed (see issues 2 and 3 below); fixed the shell startup and the compiler fallback.
5. Setup script failed on pip (issue 4); loaded `miniforge`, removed `.venv`, added a Python version check to the script.
6. Build on Engaging succeeded: modules `gcc/12.2.0`, `cmake/4.1.2`, `miniforge/26.7.2-0`; unit test passed; the smoke executable reported 448 OpenMP threads on the login node.
7. Wrote `scripts/submit_slurm.sh`; submitted job 28900246 to `mit_quicktest`.
8. Job result: COMPLETED, exit 0:0, 1 s, node1600 (AMD EPYC 9474F, 48 cores), 4 CPUs allocated, `OMP_NUM_THREADS=4`, `OpenMP threads available: 4`, empty `.err`, commit `400df6b`.

---

## 4. Phase 2 — Core Framework and First End-to-End Experiment (Nov 12, 2026)

### 4.1 Core interfaces implemented

Implemented the shared architecture required for the four partition-learning algorithms.

**Partition (`include/core/partition.hpp`)**
- Stores element-to-cluster assignments.
- Supports cluster membership checks and metadata.
- Accepts arbitrary cluster labels; evaluation uses label-invariant comparisons.

**Oracle / ExactOracle**
- Provides a standardized pairwise same-cluster query interface.
- ExactOracle evaluates queries against a ground-truth partition.
- Tracks total queries and unique queried pairs.
- Supports concurrent query accounting.
- Algorithms access the oracle interface rather than ground-truth labels.

**Proxy / SyntheticProxy**
- Provides pairwise similarity scores.
- Synthetic proxy generates deterministic scores for a fixed seed and element pair, independent of query order.
- Supports optional caching.
- Separately tracks score accesses and computations.

**Algorithm interface**
- Standardizes algorithm execution and output.
- Supports optional proxy information.
- Provides query-round accounting needed for limited-adaptivity experiments.

### 4.2 Baseline algorithm

Implemented `RepresentativeBaseline`, a sequential exact-recovery reference algorithm.

Procedure:

1. Process elements in order.
2. Maintain one representative for each discovered cluster.
3. For each new element, query existing representatives sequentially.
4. If a matching representative is found, assign the element to its cluster.
5. Otherwise, create a new cluster with the element as its representative.

Properties:
- Exact partition recovery with a perfect same-cluster oracle.
- No proxy information required.
- Query complexity depends on element ordering and cluster structure.
- Each sequential oracle query is counted as an adaptive round in the current implementation.
- Serves as a correctness reference for later algorithms, not a research-paper implementation.

### 4.3 Evaluation and experiment runner

Implemented:
- Exact partition recovery.
- Pairwise precision, recall, and F1.
- Adjusted Rand Index (ARI).
- Oracle and proxy query accounting.
- Adaptive-round tracking.
- Algorithm runtime measurement.
- CSV export.

The runner accepts command-line parameters for `n`, `k`, `seed`, and output path.

Current implementation generates a balanced synthetic partition. Generalized synthetic benchmarks are planned for Phase 3.

### 4.4 Test results

Local macOS release build:

```text
All tests passed (86 assertions in 16 test cases)
```

First end-to-end experiment:

```bash
./build-release/partition_experiment \
  --n 12 \
  --k 3 \
  --seed 42 \
  --output results/raw/milestone1.csv
```

Observed output:

```text
Algorithm: representative_baseline
n=12 k=3 seed=42
Exact recovery: yes
Oracle queries: 21
Adaptive rounds: 21
Pairwise F1: 1
ARI: 1
CSV: results/raw/milestone1.csv
```

CSV record:

```csv
algorithm,dataset,n,k,seed,oracle_queries,unique_oracle_queries,proxy_accesses,proxy_computations,adaptive_rounds,runtime_ms,exact_recovery,pairwise_precision,pairwise_recall,pairwise_f1,adjusted_rand_index
representative_baseline,balanced_synthetic,12,3,42,21,21,0,0,21,0.0058329999999999996,1,1,1,1,1
```

The experiment successfully recovered the partition and generated a CSV result.

The recorded runtime is specific to this run and is not a statistically meaningful benchmark.

## 5. Milestone 2 — Synthetic benchmarks

Synthetic benchmarks give us control over two variables that often move
together in real data: the distribution of cluster sizes and the amount
of information in a similarity score. To measure sensitivity to imbalance,
we can fix n, k, and the score distributions while changing cluster
sizes. To measure sensitivity to side information, we can fix the
partition and change the score distributions. The representative baseline
provides an oracle-only reference for those comparisons.

Hellinger divergence describes how much the two score densities overlap
and appears in the project's side-information query bounds. AUC answers
a different question: how often a within-cluster score ranks above a
between-cluster score. Reporting both lets us build examples with similar
Hellinger divergence and different ranking quality, as in the Gaussian
and uniform comparison below.

### 5.1 Partition construction

`include/benchmarks/synthetic_partition.hpp` generates a partition of `n`
elements with exactly `k` nonempty clusters. It rejects inputs outside
`1 <= k <= n` and nonpositive or nonfinite power-law exponents. The generator
assigns cluster labels first, then shuffles them using `std::mt19937_64` seeded
by `--seed`. Shuffling removes the connection between contiguous element IDs
and the order in which labels were generated.

| Shape | Allocation |
|---|---|
| `balanced` | Assign cyclic labels so sizes differ by at most one |
| `uniform` | Seed each cluster with one element, then sample each remaining label with equal probability |
| `zipf` | Seed every cluster, then sample remaining labels with weights proportional to inverse rank |
| `power-law` | Use inverse rank raised to `--exponent`, which defaults to 2 |
| `one-large` | Give all remaining elements to cluster 0 after seeding every cluster |
| `singleton-heavy` | Distribute the remaining elements across the first `max(1, floor(k/4))` clusters |

The initial assignment guarantees nonempty clusters. For the weighted
shapes, it also changes the expected size profile when `n` is close to `k`.
The uniform mode samples assignments after seeding and can yield uneven
cluster sizes. Balanced mode preserves the original benchmark's assignment
and shuffle procedure.

### 5.2 Proxy distributions and accounting

`include/core/score_distribution.hpp` defines Gaussian parameters as mean
and standard deviation, uniform parameters as lower and upper endpoints,
and Beta parameters as two positive shape parameters. `--within` configures
scores for pairs in the same cluster and `--between` configures scores for
pairs in different clusters. The two families can differ. Omitting either
option leaves that class on the original Gaussian path, with mean 0.8 or
0.2 and standard deviation 0.1.

`SyntheticProxy` canonicalizes the pair by sorting its IDs and derives a
seed from the unordered pair and the proxy seed. Configured distributions
draw from a pair-local random engine. Thus, within a build, the score is
independent of query order, pair orientation, and cache state. The proxy
seed defaults to the partition seed and can be set separately with
`--proxy-seed`. Standard library random distributions and shuffle behavior
can vary across platforms, so cross-toolchain bit-for-bit reproducibility
has not been established.

The proxy computes scores lazily. A cache hit increments the access
counter without incrementing the computation counter. The cache grows with
the number of pairs accessed when enabled. The representative baseline
uses the exact oracle exclusively, so both proxy counters stay at zero in
these runs. The benchmark generates no dense score matrix.

### 5.3 Quality estimates

`include/benchmarks/proxy_quality.hpp` draws `--quality-samples` scores
per class directly from the configured distributions. Let `p` and `q`
be the within and between densities. Averaging
`2 sqrt(p q) / (p + q)` across the equally weighted mixture estimates
the Hellinger affinity, which is subtracted from 1 to estimate squared
Hellinger divergence. Averaging `2 min(p,q) / (p + q)` estimates the
overlap coefficient. The implementation evaluates these weights from
log-density differences.

For AUC, the code sorts the between-class sample and uses binary searches
to count how many scores fall below or tie each within-class score. Ties
count half. The program also reports sample means and variances. For
`m` samples per class, the report uses O(m) memory and O(m log m) time,
independent of the number of possible dataset pairs.

### 5.4 Verification

| Check | Observed result |
|---|---|
| Original balanced benchmark, n=12, k=3, seed=42 | Exact recovery, 21 oracle queries |
| Zipf benchmark, n=500, k=20, seed=42, Beta(8,2) versus Beta(2,8) | Exact recovery, 3,497 oracle queries, estimated H² 0.88568 and AUC 0.99838 |
| Disjoint uniforms on [0.6, 1] and [0, 0.4] | H² 1, overlap 0, AUC 1 |
| Gaussian and uniform pairs calibrated to theoretical H² ≈ 0.39347 | Estimated H² 0.39348 and 0.39314, with estimated AUC 0.92073 and 0.81618 respectively |
| Power-law benchmark, n=100,000, k=2, seed=42 | Exact recovery, 119,824 oracle queries |
| Catch2 suite | 19 test cases and 127 assertions passed |

The matched-divergence comparison used Gaussian means 2 and 0 with
unit variance and two unit-width uniform intervals offset by 0.39346934.
It used 100,000 samples per class for each pair.

### 5.5 Issues and limitations

- CMake was unavailable in the implementation environment. The executable
  was built directly with GCC. A first attempt to compile tests against
  Catch2's amalgamated source still referenced its modular headers.
  The successful workaround used temporary test copies that included the
  amalgamated header. The repository test includes stayed unchanged.
- Quality estimates are stochastic and currently print to stdout. The
  baseline CSV records recovery and query accounting. Record the command
  line if the proxy configuration and sample count need to be reproduced.
- Extremely concentrated score distributions can reach floating-point
  limits. The quality reporter rejects nonfinite sampled scores or
  undefined density differences.
- The large-n smoke run used only two clusters. It checks the absence of a
  dense score matrix in that configuration. It provides no scaling claim
  for larger k or for algorithms that access many distinct pairs.

## 6. Milestone 3 — Black limited-round recovery

A round is a batch of queries that can be chosen before seeing any of
their answers. The representative baseline can stop comparing an element
to cluster representatives as soon as it finds a match, but that decision
depends on the preceding answer. At the other extreme, querying all pairs
requires one round and spends n(n − 1)/2 queries. Black's algorithm uses
the round budget to trade between these costs: it pays for all-pairs
queries within small blocks, then carries only one element per local
cluster into the next round.

The block size balances the cost of the current batch against the size of
the remaining problem. A block of size t costs at most t(t − 1)/2 queries
and contributes at most k representatives. Larger blocks cost more to
learn locally, but there are fewer of them and therefore fewer
representatives in the next round. For fixed r, the paper gives a
worst-case query bound of order n^(1+ε) k^(1−ε), where
ε = 1/(2^r − 1). The smoke tests below measure the actual cost on
particular partitions. They illustrate how a small round budget can
reduce sequential dependencies even when the baseline makes fewer queries.

### 6.1 Paper and interface

`include/algorithms/black_limited_round.hpp` implements the deterministic
pairwise Algorithm 1 from Section 3 of Black, Mazumdar, and Saha.
Its constructor takes a positive cluster upper bound and round budget.
The algorithm receives the element count and oracle interface. The
synthetic runner knows the true number of clusters from dataset
configuration and uses it as the default bound. It accepts larger
bounds and rejects smaller ones. The algorithm assumes that the
supplied bound is valid. No proxy accesses are made.

[Learning Partitions with Optimal Query and Round Complexities](https://arxiv.org/html/2505.05009v1#S3)

### 6.2 Query schedule and reconstruction

1. Initialize an active list in element-ID order and a union-find parent
   array over all original IDs.
2. At each level, let m be the number of active representatives and r the
   number of remaining rounds. With one remaining round or m ≤ 16k,
   use a single block and query every unordered active pair.
3. Otherwise, calculate ε = 1 / (2^r − 1) and use consecutive blocks of
   size at most ceil(3k(m/k)^ε). Only the final block can be short.
   Query every unordered pair inside each block in a single logical round.
4. For each positive answer, merge the roots and keep the smaller ID as
   the root. All queries in the round still execute. After the round, keep
   exactly the roots as active representatives and repeat with one fewer
   round. A singleton active set requires zero further queries.
5. Resolve the root of every original element to produce its cluster
   label. When local components merge in later rounds, their earlier
   member elements inherit the final representative through union-find.

Every local component is established by querying every pair within its
block. Each true cluster present in a block contributes one representative,
so subsequent rounds retain evidence from every cluster. Merging
representatives joins their entire local components. The fixed input order,
block boundaries, and root selection make the schedule deterministic.

### 6.3 Implementation decisions

- Pair iteration streams each fixed batch, avoiding a separate vector of
  all scheduled pairs. Answers update union-find during iteration, while
  the active list and loop boundaries stay fixed. The next batch depends
  on those answers only after the current batch finishes.
- The base-case check uses ceiling division by 16 to avoid multiplying k
  by 16 in integer arithmetic. The exponent is evaluated using the
  reciprocal power of two. For budgets above long-double precision its
  contribution is treated as zero, avoiding overflow from forming 2^r.
- The algorithm follows the paper's all-pairs schedule, including
  repeated pairs across rounds. The exact oracle reports total queries
  and distinct pairs separately. Its distinct-pair tracking still uses
  memory proportional to the number of unique pairs.
- `--algorithm black_limited_round` enables the implementation.
  `--rounds` defaults to 2. Black CSVs append the requested budget,
  cluster bound, and pipe-delimited batch sizes to the existing columns.
  The baseline remains the default and retains its original CSV schema.
  Each run overwrites its output path.

### 6.4 Verification

| Configuration | Queries | Unique pairs | Batches | Exact recovery |
|---|---:|---:|---|---|
| Baseline, balanced n=500, k=10, seed=42 | 2,740 | 2,740 | 2,740 single-query rounds | Yes |
| Black, same dataset, bound=10, budget=3 | 17,605 | 17,155 | 12,655 / 4,950 | Yes |
| Black, balanced n=4,096, k=2, seed=42, bound=2, budget=3 | 43,159 | 42,924 | 34,776 / 8,058 / 325 | Yes |

For n=500, the first round uses nine blocks of 53 and one block of 23.
The 100 surviving representatives satisfy the 16k base case, so the
second batch queries all 4,950 representative pairs and the third
allowed round goes unused. Within each first-round block, the second
round repeats the 45 pairs among its ten representatives, for 450 repeats
across the ten blocks.

The GCC/Catch2 suite passed 23 cases and 526 assertions. Added checks
covered the all-pairs base cases, singleton recovery, invalid
constructor parameters, exact batch sizes, repeated-pair accounting,
and a run that uses all three rounds. The randomized test exercised
128 combinations across four partition shapes, eight seeds, and four
budgets, including a budget of 128. It checked exact recovery, the
round cap, and agreement between batch sums and oracle counts.
The CLI rejected a cluster bound smaller than the configured k.

### 6.5 Issues and follow-up

- The tested implementation executes queries serially within each logical
  round. The recorded round count measures adaptivity. Parallel execution
  and speedup still require separate implementation and measurement.
- The representative baseline can use fewer queries on a particular
  dataset even when Black uses far fewer rounds. The smoke runs verify
  behavior and accounting. Comparing with the paper's worst-case query
  bound requires sweeps over n, k, and round budgets.
- Streaming the pair schedule avoids storing an explicit batch. For
  large all-pairs base cases, the oracle's distinct-pair set can still
  grow quadratically. Treat runtime and memory measurements as properties
  of both the algorithm and the instrumentation.
- CMake and MIT Engaging verification remain pending for this milestone.
  The recorded tests used a direct GCC build and the same temporary
  Catch2 amalgamation workflow as Milestone 2.

## 7. Milestone 4 — Edge Ordering

Edge Ordering spends proxy computation up front to reduce exact oracle
queries. The proxy prioritizes pairs likely to match, so useful positive
merges can happen early. Each merge can make many subsequent comparisons
redundant. With an exact oracle, even an inverted similarity ranking
affects query count and leaves correctness intact.

### 7.1 Paper and implementation search

Mazumdar and Saha (2017), Section 2.2, specify descending similarity and a
scan until every edge has been queried or inferred. Wang et al. (2013),
Section 4.1, give the sequential labeling procedure for a supplied pair
order, including inference of both matches and nonmatches. This version
uses the descending score order specified in the theoretical analysis.
The original paper also studies a separate ordering heuristic and parallel
labeling. Searches for the papers and author projects did not identify a
public implementation that could be verified against this procedure.

- [Mazumdar and Saha (2017)](https://arxiv.org/abs/1702.01208)
- [Wang et al. (2013)](https://arxiv.org/abs/1408.6916)

### 7.2 Query schedule and inference

The algorithm generates every unordered pair, requests one proxy score for
each, and sorts scores in descending order. Equal scores preserve
lexicographic pair order. It scans the sorted list using union-find for
known matches and symmetric sets of incompatible component roots for known
nonmatches. A pair can be inferred if its endpoints have the same root or
their roots are known to be incompatible. Otherwise, the oracle supplies
the answer. A positive answer merges the two components and transfers
their negative constraints to the merged root. A negative answer records
the incompatibility in both directions.

Each component only contains confirmed matches. An incompatibility
between two components witnesses that their true clusters differ, so it
continues to hold after either component merges with another confirmed
match. By the end of the scan, the algorithm has enough information to
label every pair. It requires an exact oracle and a proxy with finite
scores. It takes no cluster count or score distribution parameters.

### 7.3 Verification and cost

At n=500, k=10, seed=42 on the balanced benchmark, Edge Ordering recovered
exactly with 535 oracle queries, all unique, and 124,750 proxy accesses
and computations. The corresponding representative baseline used 2,740
oracle queries. Edge Ordering used 535 sequential rounds. The smoke run
spent about 18 ms of its 19 ms algorithm runtime scoring and sorting.
These timings are single observations.

The direct GCC/Catch2 build passed 26 test cases and 1,135 assertions.
The tests check reversed score ranking, positive and negative inference,
transfer of a negative constraint after a merge, and exact recovery
across 120 shape, seed, and cluster-count combinations. The latter include
all-singleton and one-cluster partitions.

The candidate list consumes O(n²) memory. Comparison sorting uses
O(n² log n) time when its temporary buffer is available. The runner
disables proxy memoization since the current algorithms either skip the
proxy or access each pair once, and the scored edges already store the
values. The CSV records preprocessing time separately while retaining
inclusive algorithm runtime.

Local macOS verification reproduced exact recovery, 535 oracle queries
and rounds, F1 = 1, and ARI = 1. CTest reported 0.55 s elapsed and the
smoke run reported 32.1816 ms preprocessing. These are local checks.
Formal timing comparisons are deferred to Engaging.

## 8. Milestone 5 — Node Ordering

Node Ordering uses pair scores twice. The sum of incident scores sets a
global priority, so nodes with many likely neighbors are considered early.
Once a node is selected, the scores to earlier nodes determine which
existing cluster to try first. With calibrated match probabilities the
sum estimates the expected number of neighbors. This benchmark uses
the configured proxy scores directly, so the sum is a priority signal.

### 8.1 Source and implementation search

The implementation follows Algorithm 5 of Vesdapunt, Bellare, and Dalvi
(2014), as also described by Mazumdar and Saha (2017). Both priority
orders are descending. The 2015 erratum corrects an experimental figure.
The original paper mentions a Python implementation. A search for public
code did not find a version that could be verified against Algorithm 5.

- [Original paper](https://www.vldb.org/pvldb/vol7/p1071-vesdapunt.pdf)
- [Erratum](https://www.vldb.org/pvldb/vol8/p641-vesdapunt-errata.pdf)
- [Theoretical analysis](https://arxiv.org/abs/1702.01208)

### 8.2 Query schedule and correctness

The algorithm scores each unordered pair once and stores the scores in a
triangular table. It sorts nodes by descending score sum. For each current
node, it finds the highest-scoring earlier member of each known cluster
and tries these members in descending score order. Equal priorities and
equal pair scores use element ID to break ties. A negative answer rules
out the entire candidate cluster by transitivity. A positive answer
assigns the node immediately. If every candidate fails, the node starts
a new cluster.

This implements the paper's cached inference directly. In a full scan of
earlier nodes, later members of a rejected cluster would be inferred
negative. Keeping only the earliest member from each cluster gives the
same actual oracle queries for the chosen tie order. Exact oracle answers
maintain the correct partition of the processed prefix by induction.
The paper's approximation concerns oracle query cost. Recovery remains
exact under the perfect-oracle assumption regardless of the score quality.

### 8.3 Verification and limitations

At n=500, k=10, seed=42, the balanced run recovered exactly using 535
oracle queries and 124,750 proxy computations. Each oracle query is counted
as one sequential round. A hand-constructed test checks the exact query
sequence and verifies that a cluster can be tested via a member other than
its first node. Additional tests check exact recovery with identical
within- and between-score distributions, including the one-cluster
and all-singleton cases.

The direct GCC/Catch2 build passed 28 test cases and 1,317 assertions.

The score table takes O(n²) memory. Selecting best members scans the
processed prefix, taking O(n²) total. Sorting the candidates costs at
most O(nk log(k+1)) overall for a partition with k clusters. The initial
scoring and global node sort are reported as preprocessing. Per-node
candidate selection and sorting are included in total runtime. Formal
performance comparisons are deferred to Engaging.

Local macOS verification of Node Ordering passed CTest in 0.63 s and
reproduced 535 queries and rounds, exact recovery, F1 = 1, and ARI = 1
at n=500, k=10, seed=42. The preprocessing reading was 9.79871 ms.
Formal timing comparisons remain deferred to Engaging.

## 9. Milestone 6 — Side-Information-Assisted Learning

Edge and Node Ordering use scores to choose which oracle question to
ask next. The Monte Carlo algorithm can assign a point without asking
about it at all. The tradeoff is that a statistical assignment can be
wrong even when every oracle answer is correct.

### 9.1 Source and benchmark adaptation

The implementation follows the initialization, iterative-update, and
grown-cluster phases in Section 3.1 of Mazumdar and Saha (2017).
The paper analyzes unknown within-cluster and between-cluster score
distributions on a finite alphabet. The implementation maps continuous
scores to 16 fixed bins after an arctangent transform. This gives bounded
histogram storage for arbitrary finite scores. Binning can collapse
differences between distributions, and score separation after binning
can differ from the continuous Hellinger estimate in the benchmark.
The finite-run growth constant is fixed at 4 and was chosen for the
smoke-test implementation. This setting has not been validated as a
constant that satisfies the proof. A public reference implementation
could not be verified in the literature and code search.

[Query Complexity of Clustering with Side Information](https://arxiv.org/abs/1706.07719)

### 9.2 Growth and assignment

1. Shuffle the element IDs with the experiment seed. Process unknown
   points in that order, querying one representative per active cluster
   until there is a positive match or the point starts a new cluster.
2. After each oracle assignment, update pooled within-cluster and
   between-cluster histograms using scores to existing confirmed points.
   The growth threshold is 4 log(n) divided by the empirical squared
   Hellinger divergence. Missing histograms or zero separation postpone
   statistical growth and leave oracle processing available.
3. When an active cluster reaches the threshold, compare the score
   histogram from each unassigned point to the cluster with the cluster's
   internal score histogram. Keep the confirmed seed fixed during the
   scan. Apply the paper's two thresholds to admit close points, queue
   ambiguous points for oracle checks, and leave distant points unassigned.
4. Check queued points against active representatives, starting with the
   grown cluster. Retire that cluster afterward and remove its confirmed
   scores from the active pooled histograms. Resume growing the remaining
   clusters until all points are assigned.

The algorithm receives the proxy, oracle, element count, and seed.
It receives no ground-truth group count or distribution parameters.
Per-cluster histograms are kept in fixed-size arrays. Retiring a cluster
can recompute cross-pair proxy scores to remove its contribution from
other clusters' histograms. Every actual oracle query is counted as one
sequential round.

### 9.3 Verification and limits
The direct GCC/Catch2 build passed 30 test cases and 1,330 assertions.

| Balanced dataset, seed 42 | Oracle queries | Exact recovery |
|---|---:|---|
| Representative baseline, n=500, k=3 | 997 | Yes |
| Side Information, n=500, k=3, disjoint uniform proxies | 870 | Yes |
| Side Information, n=500, k=10, same proxies | 2,753 | Yes |

The proxy intervals were [0.8, 0.81] within clusters and [0.1, 0.11]
between clusters. The k=3 run made 83,253 proxy accesses, including
recomputations. The k=10 run illustrates that even separated proxies
may yield little benefit when grown clusters are smaller or reached
late. Tests also cover constant, indistinguishable proxy scores and
the one-cluster and all-singleton boundaries.

The paper's asymptotic bound applies under its distribution and
concentration assumptions. The smoke tests establish behavior on the
tested inputs and give no empirical recovery guarantee across scores,
seeds, or problem sizes. Formal comparisons are deferred to Engaging.

On macOS, CTest passed in 0.13 s. The reported side-information smoke run
at n=500, k=3, seed=42 recovered exactly with 856 queries and rounds,
F1 = 1, and ARI = 1. The direct GCC run previously recorded 870 queries.
The supplied Mac output does not include the proxy flags or compiler
version, so the cause of the difference remains unverified.

## 10. Milestone 7 — Reproducible synthetic comparisons

Comparing algorithms on the same dataset and seed controls the partition
and the pairwise scores within a build. Changing either would mix algorithm
differences with input variation. The suite groups runs by dataset ID and
seed, with the same proxy settings for every method on that input.

### 10.1 Runner and output

`scripts/run_suite.py` reads JSON and invokes the existing single-run CLI
for each dataset, seed, and algorithm combination. Each invocation writes
a temporary CSV. The runner combines the rows, keeps algorithm-specific
fields such as Black's requested rounds and Edge Ordering's preprocessing
time, and records the config and executable hashes, Git revision, OS,
architecture, and OpenMP environment setting. The executable hash
identifies the actual binary even if it was built from a different source
state than the current checkout.

The final CSV replaces the destination only after all runs finish. Exit
status 2 means the experiment completed with a recovery miss, so its
accuracy and query metrics are retained. Other failures stop the suite
with the dataset, seed, and algorithm in the error message.

### 10.2 Verification

The default config runs the baseline and four research algorithms on two
datasets and two seeds, for 20 rows. Repeating the suite with the same GCC
binary reproduced every non-timing column. An invalid cluster shape
returned an error and preserved the existing output.

| Zipf overlap, Side Information | Queries | Pairwise F1 | ARI |
|---|---:|---:|---:|
| Seed 42 | 472 | 0.7183 | 0.5614 |
| Seed 43 | 643 | 0.8287 | 0.7521 |

The other 18 runs recovered exactly. These two misses show why recovery
must be evaluated alongside query counts for the Monte Carlo algorithm.
The smoke config establishes end-to-end behavior. Larger sweeps, measured
hardware and memory metadata, and Slurm arrays remain future work.
