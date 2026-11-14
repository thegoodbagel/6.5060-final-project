#!/usr/bin/env python3
"""Run all dataset, seed, and algorithm combinations through the C++ CLI."""

import argparse
import csv
import hashlib
import itertools
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import tempfile


DATASET_FIELDS = {"n", "k", "shape", "exponent", "within", "between", "proxy_seed"}
ALGORITHMS = {"representative_baseline", "black_limited_round", "edge_ordering",
              "node_ordering", "side_information"}


def settings(config):
    if not isinstance(config, dict) or set(config) != {"datasets", "seeds", "algorithms"}:
        raise ValueError("Config requires datasets, seeds, and algorithms")
    for key, values in config.items():
        if not isinstance(values, list) or not values:
            raise ValueError(f"{key} must be a nonempty list")
    seeds = config["seeds"]
    if any(type(seed) is not int or not 0 <= seed < 2**64 for seed in seeds):
        raise ValueError("Seeds must be unsigned 64-bit integers")
    if len(set(seeds)) != len(seeds):
        raise ValueError("Seeds must be distinct")
    dataset_ids = set()
    for dataset in config["datasets"]:
        if not isinstance(dataset, dict) or set(dataset) - DATASET_FIELDS - {"id"}:
            raise ValueError("Invalid dataset fields")
        dataset_id = dataset.get("id")
        if not isinstance(dataset_id, str) or not dataset_id or dataset_id in dataset_ids:
            raise ValueError("Dataset IDs must be distinct nonempty strings")
        dataset_ids.add(dataset_id)
        if any(type(dataset.get(key)) is not int for key in ("n", "k")) or not (
            1 <= dataset["k"] <= dataset["n"]
        ):
            raise ValueError("Datasets require integer n and k with 1 <= k <= n")
    for algorithm in config["algorithms"]:
        if not isinstance(algorithm, dict):
            raise ValueError("Algorithm entries must be objects")
        name = algorithm.get("name")
        if not isinstance(name, str) or name not in ALGORITHMS:
            raise ValueError("Unknown algorithm")
        allowed = {"name"}
        if name == "black_limited_round":
            allowed |= {"rounds", "k_bound"}
        if set(algorithm) - allowed:
            raise ValueError(f"Invalid settings for {algorithm['name']}")
    return itertools.product(config["datasets"], seeds, config["algorithms"])


def cli_flags(options, omit):
    return [
        part for key, value in options.items() if key != omit
        for part in (f"--{key.replace('_', '-')}", str(value))
    ]


def run_suite(binary, config_bytes, output):
    cases = settings(json.loads(config_bytes))
    revision = subprocess.run(
        ["git", "rev-parse", "HEAD"], cwd=Path(__file__).resolve().parents[1],
        text=True, capture_output=True, check=False
    )
    metadata = {
        "config_sha256": hashlib.sha256(config_bytes).hexdigest(),
        "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
        "git_commit": revision.stdout.strip(),
        "platform": platform.platform(),
        "machine": platform.machine(),
        "omp_num_threads": os.environ.get("OMP_NUM_THREADS", ""),
    }
    rows = []
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=output.parent) as temporary:
        temporary = Path(temporary)
        result = temporary / "result.csv"
        for dataset, seed, algorithm in cases:
            args = [
                str(binary), "--algorithm", algorithm["name"], "--seed", str(seed),
                *cli_flags(dataset, "id"), *cli_flags(algorithm, "name"),
                "--output", str(result),
            ]
            result.unlink(missing_ok=True)
            completed = subprocess.run(args, text=True, capture_output=True, check=False)
            if completed.returncode not in (0, 2):
                raise RuntimeError(f"{dataset['id']} seed {seed} {algorithm['name']}: "
                                   f"{completed.stderr.strip() or completed.returncode}")
            with result.open(newline="") as stream:
                records = list(csv.DictReader(stream))
            if len(records) != 1 or records[0].get("exact_recovery") != (
                "1" if completed.returncode == 0 else "0"
            ):
                raise RuntimeError("Unexpected experiment CSV or exit status")
            rows.append({
                "dataset_id": dataset["id"], **records[0],
                "shape": dataset.get("shape", "balanced"),
                "exponent": dataset.get("exponent", 2),
                "within": dataset.get("within", "gaussian,0.8,0.1"),
                "between": dataset.get("between", "gaussian,0.2,0.1"),
                "proxy_seed": dataset.get("proxy_seed", seed),
                **metadata,
            })
        columns = list(dict.fromkeys(key for row in rows for key in row))
        combined = temporary / "combined.csv"
        with combined.open("w", newline="") as stream:
            writer = csv.DictWriter(stream, fieldnames=columns)
            writer.writeheader()
            writer.writerows(rows)
        combined.replace(output)
    failures = sum(row["exact_recovery"] == "0" for row in rows)
    print(f"Wrote {len(rows)} runs to {output} ({failures} recovery misses)")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--binary", type=Path, default=Path("build-release/partition_experiment"))
    args = parser.parse_args()
    try:
        if args.output.resolve() in {args.config.resolve(), args.binary.resolve()}:
            raise ValueError("Output must differ from the configuration and executable")
        run_suite(args.binary.resolve(), args.config.read_bytes(), args.output)
    except (ValueError, OSError, RuntimeError) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
