# `bignum_add_bignum` standard benchmark profile

This companion document defines the reproducible standard matrix in `bignum_add_bignum_standard.json`. The manifest is consumed by `benchmark-framework`; it does not alter the public addition API and it measures only representable unsigned additions.

## Schema and workload fields

Every profile uses `schema_version: 1` and supplies `input_kind`, `operation_kind`, `measure_mode`, `size_profile`, and `capacity_profile`. The addition adapter accepts `zero`, `nonzero`, and `mixed` input kinds; `add` is the canonical operation token and framework-compatible aliases remain accepted. Measurement modes are `kernel-only` and `end-to-end`. Size profiles are `one`, `quarter`, `half`, `variable`, and `near-capacity`; capacity profiles are `normal` and `near-capacity`.

The adapter generates deterministic operands from the manifest seed and constrains the highest word of nonzero inputs so that the selected addition completes successfully. `near-capacity` therefore means a representable boundary workload, not an intentional overflow test. Error-path behavior is covered by the unit tests rather than included in timed successful samples.

## Validation and acceptance

The adapter rejects a NULL workload, unsupported vocabulary, or invalid profile before worker execution. Each successful sample reports `returncode: 0`, `successful` equal to the requested iteration count, a deterministic fingerprint, and a result-sensitive checksum. C11 and ASM comparisons are valid only when profile ID, mode, seed, data count, and iteration settings are identical.

The standard manifest contains eight profiles. With the default two measurement modes and three repetitions, the expected report contains 48 samples. Any missing, failed, or fingerprint-mismatched sample invalidates the comparison.

## Reproducible commands

Build and run the C11 baseline:

```sh
make bench_matrix CONFIG=release USE_ASM=no \
  BENCH_MATRIX_PROFILE=benchmarks/profiles/bignum_add_bignum_standard.json \
  BENCH_MATRIX_REPETITIONS=3 BENCH_MATRIX_ITERATIONS=100000 \
  BENCH_MATRIX_MT_TOTAL_ITERATIONS=200000 BENCH_MATRIX_WARMUP=1000 \
  BENCH_MATRIX_DATA_COUNT=4 BENCH_MATRIX_TIMEOUT_SECONDS=60
```

Repeat with `USE_ASM=yes`, then compare `ns_per_call` for identical profile/mode keys. Use medians across repetitions for the primary comparison and retain the raw JSON report for review.

## Failure semantics

A benchmark failure is a correctness or setup issue, not a performance datapoint. Investigate invalid profile fields, missing framework tools, failed operation statuses, mismatched fingerprints, and timeout records before comparing timings. The frozen repository Makefile must not be edited to bypass a failed sample.
