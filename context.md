# Project Tasks

**Efficient Partition Learning with Side Information and Limited Adaptive Rounds**
Timeline: Nov 12 – Dec 3, 2026 · Team: 2 · Language: C++20 · Compute: MIT Engaging · Real-world dataset: WDC Products

Legend: `[x]` done · `[ ]` not done · **Status** is per phase. Last updated: Nov 12, 2026.

| Phase | Dates | Status |
|-------|-------|--------|
| 1. Setup and infrastructure | Nov 12–13 | Nearly done (a few leftovers below) |
| 2. Common experimental framework | Nov 13–15 | Not started |
| 3. Synthetic benchmark generation | Nov 14–17 | Not started |
| 4. Four algorithms (critical path) | Nov 15–20 | Not started |
| 5. Experiment runner and reproducibility | Nov 17–21 | Not started |
| 6. Real-world benchmark (WDC) | Nov 20–24 | Not started |
| 7. Parallelism and performance | Nov 21–26 | Not started |
| 8. Main experimental evaluation | Nov 23–29 | Not started |
| 9. Final analysis and deliverables | Nov 28 – Dec 3 | Not started |

**Coordination rule:** agree on the Phase 2 interfaces before either person implements an algorithm. Each algorithm must be independently testable and mergeable.

---

## Immediate next steps

1. Finish the Phase 1 leftovers (branching/style, scratch paths, file transfer test, debug build).
2. Write and review the core headers (`partition.hpp`, `oracle.hpp`, `proxy.hpp`, `algorithm.hpp`, `metrics.hpp`) together.
3. In parallel: extract exact pseudocode for all four algorithms and verify the bibliography mapping of references [2] and [3].
4. Milestone 1: one synthetic partition, oracle, proxy, trivial algorithm, evaluation, one CSV row.

---

## Phase 1: Project Setup and Infrastructure

**Status: nearly done.** Deliverable: a working C++/Python environment that runs locally and on Engaging. (Met for the build and a smoke-test Slurm job.)

### 1.1 Repository
- [x] Create GitHub repository
- [x] Establish directory structure
- [x] Write initial README (includes setup and reproducibility instructions)
- [ ] Configure Git branches and collaboration workflow
- [ ] Decide naming conventions and coding style

### 1.2 Development environment
- [x] C++20 and CMake (OpenMP optional)
- [x] GCC/Clang configured (Apple Clang locally, GCC 12.2.0 on Engaging)
- [x] Release build verified locally and on Engaging
- [ ] Debug/sanitizer build verified (`-fsanitize=address,undefined`)
- [x] OpenMP verified on Engaging (4 threads inside a 4-core Slurm job)
- [x] Catch2 set up (fetched by CMake at configure time); smoke test passes
- [x] Python environment (`.venv`, `requirements.txt`; Python >= 3.10)
- [x] `scripts/setup_linux.sh` written and tested on Engaging
- [ ] Loosen `requirements.txt` to unpinned top-level packages (numpy, pandas, matplotlib, scipy) if it is still a full `pip freeze`

### 1.3 MIT Engaging
- [x] Account and SSH access
- [x] Repository cloned on Engaging
- [x] Partitions and limits identified (see `archive.md`)
- [x] Basic Slurm job script (`scripts/submit_slurm.sh`)
- [x] Small C++ test job run (job 28900246, `mit_quicktest`, COMPLETED)
- [ ] Record scratch and persistent storage paths (quota report shows HOME 200 GB, SCRATCH 1 TB, POOL 1 TB)
- [ ] Verify output transfer between Engaging and local machine (`scp`/`rsync` of a CSV)
- [ ] Partner runs `setup_linux.sh --build` and a Slurm test from their own account

---

## Phase 2: Common Experimental Framework

**Status: not started.** Complete before any algorithm work. Deliverable: a shared framework into which all four algorithms plug without changing the experiment infrastructure.

### 2.1 Core data structures
- [ ] `ElementId`, `ClusterId`
- [ ] `Partition` (labels, number of clusters, cluster-size metadata)
- [ ] `AlgorithmResult`
- [ ] `Query` and `QueryBatch`

### 2.2 Oracle
- [ ] Common oracle interface
- [ ] Exact ground-truth oracle
- [ ] Thread-safe query counter (thread-local counters aggregated per batch)
- [ ] Duplicate-query tracking
- [ ] Algorithms cannot access ground-truth labels

### 2.3 Proxy
- [ ] Common proxy interface (lazy, with optional precomputed variant)
- [ ] Synthetic proxy with deterministic per-pair scores (hash-derived seed, independent of query order and thread)
- [ ] Proxy query counters
- [ ] Caching

### 2.4 Algorithm interface
- [ ] Common interface; proxy optional (`nullptr` allowed)
- [ ] Configurable parameters
- [ ] Standardized output partition
- [ ] Round tracking and runtime measurement

### 2.5 Evaluation utilities
- [ ] Exact recovery
- [ ] Pairwise precision, recall, F1
- [ ] Adjusted Rand Index
- [ ] Oracle/proxy query and runtime accounting

---

## Phase 3: Synthetic Benchmark Generation

**Status: not started.** Deliverable: a generator that controls partition structure and proxy quality independently.

### 3.1 Partition generator
- [ ] Uniform, power-law, Zipf, one-large-cluster, singleton-heavy
- [ ] Configurable n and k, fixed seeds
- [ ] Validity tests (n elements, exactly k nonempty clusters, valid IDs)

### 3.2 Proxy distributions
- [ ] Gaussian, Uniform, Beta for f₊ and f₋
- [ ] Configurable means, variances, shapes, overlap

### 3.3 Proxy quality characterization
- [ ] Squared Hellinger divergence H²(f₊, f₋)
- [ ] AUC, means/variances, overlap coefficient
- [ ] Distribution pairs with similar H² but different shapes
- [ ] Numerical verification of divergence values

### 3.4 Benchmark tests
- [ ] Small partitions with scores for every pair
- [ ] Within-cluster vs between-cluster score distributions as intended
- [ ] Deterministic for fixed seeds
- [ ] Large n without dense O(n²) storage

---

## Phase 4: Four Algorithms (critical path)

**Status: not started.** First extract exact pseudocode, assumptions, and stopping conditions from each paper. Do not implement from asymptotic bounds alone.

### 4.0 Literature prerequisites
- [ ] Verify bibliography mapping of references [2] and [3] (the side-information paper vs the edge/node ordering paper)
- [ ] Confirm the node-ordering direction (the plan says increasing similarity; do not assume highest-first)

### 4.1 Algorithm 1: deterministic limited-round (Black et al. 2025)
- [ ] Pseudocode, parameters, assumptions
- [ ] Deterministic query selection; configurable rounds r
- [ ] Query-batch generation; queries per round tracked
- [ ] Partition reconstruction; exact recovery verified
- [ ] Query counts compared against Θ(n^(1+1/(2^r−1)) k^(1−1/(2^r−1)))

### 4.2 Algorithm 2: edge ordering
- [ ] Exact procedure from the paper
- [ ] Candidate edges, proxy retrieval, sorting
- [ ] Oracle execution, partition updates, stopping condition
- [ ] No duplicate queries; small-case correctness
- [ ] Measure preprocessing cost (sorting O(n²) edges)

### 4.3 Algorithm 3: node ordering (approximate)
- [ ] Exact procedure from the paper
- [ ] Node scores and prescribed ordering
- [ ] Oracle execution, updates, stopping condition
- [ ] Approximation quality (F1, ARI); imbalanced partitions

### 4.4 Algorithm 4: side-information-assisted (Mazumdar and Saha 2017)
- [ ] Exact pseudocode
- [ ] Initial partitioning and initial queries
- [ ] Proxy-based query selection and refinement; stopping conditions
- [ ] Preserve the parameter-free property (algorithm must not be given k, f₊, f₋)
- [ ] Exact recovery; query counts vs O(k² log n / H²(f₊, f₋))

### 4.5 Cross-algorithm correctness
- [ ] All four on identical small partitions; brute-force reference for tiny n
- [ ] k = 1, k = n, singletons, highly imbalanced
- [ ] At least 100 randomized correctness tests
- [ ] Exact recovery where guaranteed; approximation metrics for node ordering
- [ ] Consistent query accounting

---

## Phase 5: Experiment Runner and Reproducibility

**Status: not started.** Can overlap with Phase 4. Deliverable: one command runs a configured experiment and writes a reproducible CSV.

- [ ] JSON config parsing (dataset, algorithm, proxy parameters, n, k, distributions, seeds, threads)
- [ ] Runner: initialize dataset/oracle/proxy, run algorithm, evaluate, collect metrics, save
- [ ] CSV logging: algorithm, dataset, n, k, cluster distribution, proxy and H², seed, rounds, oracle queries, proxy queries, preprocessing/algorithm/total runtime, memory, accuracy, threads, git commit, hardware metadata
- [ ] Repeat runs to verify determinism and fixed-seed reproducibility
- [ ] Save all configs; batch launch scripts; Slurm job arrays

---

## Phase 6: Real-World Benchmark (WDC Products)

**Status: not started.** Deliverable: a working entity-resolution benchmark in the same framework.

- [ ] Download multi-class benchmark; identify entity IDs, attributes, difficulty levels, malformed records
- [ ] Preprocess to integer-indexed records and partition labels; random and cluster-aware subsets
- [ ] Real-world oracle from entity IDs (hidden from algorithms); correctness tests
- [ ] Proxies: token Jaccard, TF-IDF cosine (then edit similarity, attribute similarity; embeddings optional)
- [ ] Run all algorithms on the same subsets; compare across difficulty levels and against synthetic proxies
- [ ] Do not build a dense similarity matrix for all 11,715 records; start with subsets
- Optional: Cora

---

## Phase 7: Parallelism and Performance

**Status: not started.** Establish sequential correctness first; parallelize only within a round.

- [ ] Identify parallelizable operations (query batches, proxy/score generation, node and edge scores, independent seeds)
- [ ] OpenMP query batches and score computation; thread-safe counters
- [ ] Results identical across thread counts
- [ ] Benchmark 1, 2, 4, 8, 16 threads: speedup S(p) = T(1)/T(p), efficiency E(p) = S(p)/p
- [ ] Memory, preprocessing separated from algorithm time; profile before optimizing
- [ ] Keep algorithm-parallelism experiments separate from job-array throughput

---

## Phase 8: Main Experimental Evaluation

**Status: not started.** Use focused sweeps, not full Cartesian products. 5–10 seeds initially.

| # | Experiment | Varied | Measured |
|---|------------|--------|----------|
| 1 | Scaling | n | queries, runtime, accuracy |
| 2 | Number of clusters | k | queries, runtime |
| 3 | Proxy quality | H²(f₊, f₋) | oracle/proxy queries |
| 4 | Distribution shape | Gaussian, Uniform, Beta | query complexity |
| 5 | Cluster imbalance | Uniform, Zipf, singleton-heavy | queries, accuracy |
| 6 | Adaptivity | rounds r | queries, runtime |
| 7 | Parallelism | threads | speedup, efficiency |
| 8 | Real-world | WDC subsets/difficulty | queries, runtime, accuracy |

Starting ranges: n ∈ {100, 200, 500, 1K, 2K, 5K}; k ∈ {2, 10, 50, 100}; threads ∈ {1, 2, 4, 8, 16}.

Procedure per experiment: define a controlled configuration, pick ranges, run several seeds, save raw CSVs, compute means and standard deviations, plot, compare against theory, document surprises.

---

## Phase 9: Final Analysis and Deliverables

**Status: not started.**

- [ ] Plots: queries vs n, runtime vs n, queries vs H², queries vs rounds, parallel speedup, cluster-size distributions, synthetic vs real (with confidence intervals)
- [ ] Investigate discrepancies with theoretical bounds
- [ ] Final report (intro, problem, theory, algorithms, architecture, methodology, synthetic results, real-world results, parallelism, limitations, conclusion)
- [ ] Presentation slides (Dec 3)
- [ ] Source code and reproducibility instructions finalized

---

## Division of work

| Workstream | Person A | Person B |
|------------|----------|----------|
| Core architecture | Lead | Review |
| Black et al. algorithm | Lead | Review |
| Edge ordering | Lead | Review |
| Synthetic partition generator | Lead | Review |
| Node ordering | Review | Lead |
| Side-information algorithm | Review | Lead |
| Proxy generator | Review | Lead |
| WDC preprocessing and real-world proxy | Review | Lead |
| Experiment runner, parallelism, final experiments, report | Both | Both |

---

## Priorities (if the schedule slips)

1. **Must complete:** common architecture, synthetic generators, four algorithms, correctness tests, runner and CSV logging, query-complexity experiments, proxy-quality experiments, final plots and report.
2. **Strongly desirable:** WDC Products, adaptivity vs runtime, parallel query execution, cluster-size experiments, memory and preprocessing analysis.
3. **Optional:** Cora, embedding proxy, expensive simulated oracle, extra distribution families, deeper profiling.

If implementation runs long, prioritize the synthetic benchmark and shrink the real-world and parallelism experiments.

## Open questions and risks

- Bibliography mapping of [2] and [3] is unverified.
- Node-ordering direction must be checked against the paper.
- Decide whether proxy scores are charged when generated or when accessed (report both access counts and total proxy compute time).
- Algorithm pseudocode extraction is the biggest schedule risk and has no code dependency, so start now.

