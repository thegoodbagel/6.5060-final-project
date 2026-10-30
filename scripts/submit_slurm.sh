#!/bin/bash
#
# submit_slurm.sh: run partition_experiment as a Slurm batch job on MIT Engaging.
#
# From the repo root, on the login node (the directory logs/ must exist first):
#   mkdir -p logs
#   sbatch scripts/submit_slurm.sh                       # smoke test: run with no args
#   sbatch scripts/submit_slurm.sh --config configs/smoke_test.json \
#          --output results/raw/smoke.csv                # arguments go to the executable
#
# Override resources at submit time without editing this file, e.g.:
#   sbatch --cpus-per-task=8 --time=01:00:00 -p mit_normal scripts/submit_slurm.sh ...
#
# Build on the login node first (CMake downloads Catch2 and needs internet):
#   module load gcc cmake && cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release \
#     && cmake --build build-release -j4

#SBATCH --job-name=partition_exp
#SBATCH --partition=mit_quicktest
#SBATCH --time=00:05:00
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=4
#SBATCH --mem=4G
#SBATCH --output=logs/%x_%j.out
#SBATCH --error=logs/%x_%j.err

set -euo pipefail

# Slurm copies the job script to a spool directory, so use the directory the job
# was submitted from (the repo root) rather than the script's own location.
cd "${SLURM_SUBMIT_DIR:-$PWD}"

# Same toolchain as the build: the binary links dynamically against that GCC's
# libstdc++/libgomp, so the module must be loaded at run time too.
# (Adjust the module name if `module avail gcc` shows something different.)
module load gcc

export OMP_NUM_THREADS="${SLURM_CPUS_PER_TASK:-1}"

BIN=./build-release/partition_experiment
if [ ! -x "$BIN" ]; then
  echo "ERROR: $BIN not found. Build on the login node first." >&2
  exit 1
fi

echo "=== job environment ==="
echo "job id:        ${SLURM_JOB_ID:-none}"
echo "partition:     ${SLURM_JOB_PARTITION:-none}"
echo "node:          $(hostname)"
echo "cpus:          ${SLURM_CPUS_PER_TASK:-?}  (OMP_NUM_THREADS=$OMP_NUM_THREADS)"
echo "cpu model:     $(lscpu | awk -F: '/Model name/ {gsub(/^ +/, "", $2); print $2; exit}')"
echo "compiler:      $(g++ --version | head -n1)"
echo "git commit:    $(git rev-parse --short HEAD 2>/dev/null || echo unknown)"
echo "args:          $*"
echo "======================="

"$BIN" "$@"
