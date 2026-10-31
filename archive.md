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

## 4. Issues encountered

| # | Symptom | Cause | Resolution |
|---|---------|-------|------------|
| 1 | Apple Clang cannot build with `-fopenmp` | macOS toolchain ships no OpenMP | OpenMP made optional in CMake; `brew install libomp` plus `-DOpenMP_ROOT=$(brew --prefix libomp)` when threading is wanted |
| 2 | Every Engaging login landed in an `Apptainer>` prompt; `module: command not found`; messages about fakeroot and `chdir /home/abrianna`; `scp` of dotfiles hung | `~/.bashrc` lines 9–10 (`module load apptainer/1.4.2` and `singularity shell --writable --fakeroot 6106-image`) auto-launched a container at login. Inside it `/root` is the real home, the host module system is unavailable, and `scp` hangs because the remote shell starts an interactive program | Commented out lines 9–10 (backup `~/.bashrc.bak`). Escape hatch if this recurs: `ssh -t host "bash --noprofile --norc"` skips startup files; the OnDemand file editor can also edit dotfiles. The container can still be started by hand when needed |
| 3 | Setup script failed: `.../spack/.../g++ not found` | `$CXX` was inherited from the loaded gcc module but the path did not exist inside the container | Script now falls back to `g++`, `c++`, or `clang++` on PATH when `$CXX` is missing. Resolved at the root by fixing issue 2 |
| 4 | `pip install` failed: `Could not find a version that satisfies contourpy==1.4.0 (from versions: none)` | System `python3` on Rocky 8 is 3.6, too old for current wheels; `requirements.txt` was a full `pip freeze` from macOS with pinned transitive dependencies | `module load miniforge`, `rm -rf .venv`, re-run setup. Script now requires Python >= 3.10. Recommended: keep `requirements.txt` as unpinned top-level packages |
| 5 | Executable reported 448 OpenMP threads | On a login node nothing caps `OMP_NUM_THREADS`, so OpenMP sees every core | Expected. Never run experiments on login nodes. Inside Slurm, `submit_slurm.sh` sets `OMP_NUM_THREADS` to the allocated cores |
| 6 | Job showed `PENDING` with reason `None` and start time `N/A`, then vanished; `scontrol show job` said "Invalid job id" | The scheduler had not yet evaluated the job; the 1-second job finished between checks, and `scontrol` only knows pending/running jobs | Use `sacct -j <jobid>` for finished jobs |
| 7 | (Preventive) Job output lost if `logs/` is missing | Slurm does not create the output directory | `mkdir -p logs` before `sbatch` |

---

## 5. Verified so far

- Local (macOS): build and smoke test pass.
- Engaging: build with GCC 12.2.0 passes; Catch2 smoke test passes; Slurm job on `mit_quicktest` completes with 4 threads.

## 6. Open items

- Branching workflow, naming conventions, coding style.
- Debug/sanitizer build verified.
- Scratch and persistent storage paths recorded; output transfer between Engaging and local tested.
- `requirements.txt` loosened (if still pinned); partner runs setup and a Slurm test.
- Bibliography mapping of references [2] and [3]; node-ordering direction.
