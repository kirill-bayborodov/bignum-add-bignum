# `bignum_add_bignum` full benchmark profile

This companion document defines the extended reproducible matrix in `bignum_add_bignum_full.json`. It exercises addition across zero, nonzero, mixed, short, variable, and near-capacity workloads while preserving the generic `benchmark-framework` schema.

## Matrix scope

Each profile contains `input_kind`, `operation_kind`, `measure_mode`, `size_profile`, and `capacity_profile`. The adapter accepts `add` as the canonical operation token and retains generic framework aliases for transport compatibility. The matrix includes both `kernel-only` and `end-to-end` measurements, with alternate profile IDs for equivalent sizes so that report grouping remains explicit.

The `near-capacity` profiles use representable operands. The adapter constrains the highest generated word and the manifest does not intentionally benchmark an overflow failure. Overflow, NULL, capacity, and overlap behavior are verified in the deterministic and extended test suites.

## Validation and expected output

Workload validation occurs before the timed callback. A valid run must complete every requested iteration, return successful operation statuses, and produce deterministic fingerprints for the same seed and workload. A report with a failed sample, timeout, missing framework tool, or fingerprint mismatch is not suitable for performance comparison.

The full manifest contains twelve profiles. With ST and MT modes and three repetitions, the expected report contains 72 samples. Use the same manifest, seed, repetition count, iteration counts, warmup, data count, and timeout for C11 and ASM.

## Reproducible commands

Run the C11 reference matrix:

```sh
make bench_matrix CONFIG=release USE_ASM=no \
  BENCH_MATRIX_PROFILE=benchmarks/profiles/bignum_add_bignum_full.json \
  BENCH_MATRIX_REPETITIONS=3 BENCH_MATRIX_ITERATIONS=100000 \
  BENCH_MATRIX_MT_TOTAL_ITERATIONS=200000 BENCH_MATRIX_WARMUP=1000 \
  BENCH_MATRIX_DATA_COUNT=4 BENCH_MATRIX_TIMEOUT_SECONDS=60
```

Repeat with `USE_ASM=yes`. Compare median `ns_per_call` values by identical profile ID and measurement mode. Report ST and MT results separately because thread scheduling overhead can dominate very short additions.

## Reproducibility and failure handling

The adapter derives all operand words from the profile seed and item index. The checksum incorporates all result words and the logical result length, preventing dead-code elimination and exposing result changes. Retain both raw JSON reports and the generated statistics summary.

Do not convert failed samples into timing values and do not alter the frozen Makefile to bypass validation. Correct the profile, framework distribution, adapter, or kernel issue first, then rerun the complete matrix.
