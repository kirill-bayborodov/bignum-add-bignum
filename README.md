# bignum-add-bignum

[![C/ASM CI](https://github.com/kirill-bayborodov/bignum-add-bignum/actions/workflows/ci.yml/badge.svg)](https://github.com/kirill-bayborodov/bignum-add-bignum/actions/workflows/ci.yml)
[![GitHub release (latest by date)](https://img.shields.io/github/v/release/kirill-bayborodov/bignum-add-bignum?label=release)](https://github.com/kirill-bayborodov/bignum-add-bignum/releases/latest)

`bignum-add-bignum` is a high-performance, standalone module for performing addition on two arbitrary-precision integers (`bignum_t`).
A highly optimized x86-64 assembly implementation of a bignum addition operation, designed for performance-critical applications.

## Distribution

Part of the `bignum-lib` project: https://github.com/kirill-bayborodov/bignum-lib
Also available as a standalone distribution.

## Features

-   **High Performance:** Hand-crafted x86-64 yasm assembly — an ultra-optimized, multithreading-ready engine delivering peak execution speed (utilizing hardware `adc`, branchless overlap checks, and SSE vectorization).
-   **Tests and Benchmarks:** Provides a comprehensive test suite (including fuzzing and thread-safety tests) and performance microbenchmarks.
-   **Automated Builds:** A comprehensive `Makefile` for easy compilation, testing, and benchmarking.
-   **Continuous Integration:** All changes are automatically built and tested via GitHub Actions.
-   **Static Analysis:** Code quality is enforced using `cppcheck` for all C-based test files.

## Dependencies

-   **Build-time:** `make`, `gcc`, `yasm`, `cppcheck`.
-   **Component:** This project requires `bignum-common` as a git submodule located at `libs/bignum-common`.

To clone the repository with its submodule, use:
```bash
git clone --recurse-submodules https://github.com/kirill-bayborodov/bignum-add-bignum.git
```

## API

The library provides a single function, declared in `include/bignum_add_bignum.h`.

```c
bignum_add_bignum_status_t bignum_add_bignum(bignum_t *result, const bignum_t *a, const bignum_t *b);
```
-   **`result`**: A pointer to the `bignum_t` structure to store the operation result (sum).
-   **`a`**: A pointer to the `bignum_t` structure representing the first addend.
-   **`b`**: A pointer to the `bignum_t` structure representing the second addend.
-   **Returns**: A `bignum_add_bignum_status_t` enum:
    - `BIGNUM_ADD_BIGNUM_SUCCESS`
    - `BIGNUM_ADD_BIGNUM_ERROR_NULL_PTR`
    - `BIGNUM_ADD_BIGNUM_ERROR_CAPACITY_EXCEEDED`
    - `BIGNUM_ADD_BIGNUM_ERROR_BUFFER_OVERLAP`
    - `BIGNUM_ADD_BIGNUM_ERROR_OVERFLOW` (if the result exceeds `BIGNUM_CAPACITY`).

## How to Build, Test, Install and Use

The project uses a `Makefile` to manage all tasks.

### Build the code
Builds the assembly object file.
```bash
make build CONFIG=release
```

### Run Unit Tests
Compiles and runs fast, essential correctness tests.
```bash
make test CONFIG=release
```

### Run Static Analysis
Checks all C source files (`tests/`, `benchmarks/` and `dist/`) for potential bugs and style issues.
```bash
make lint
```

### Run Performance Benchmarks
Compiles and runs performance tests using `perf`. The txt report is saved to `benchmarks/reports/check_*.txt`.
```bash
make bench CONFIG=debug
```

### Build the distributive
Builds the installation pack of files (with objects .o file) in dist directory.
```bash
make install CONFIG=release
```

### Build the distributive
Builds the distributive pack of files (with single-header and static library .a file).
```bash
make dist CONFIG=release
```

## Clean Up

To remove all generated files (object files, executables, reports):
```bash
make clean
```

## How to Use

This project produces an object file (`bignum_add_bignum.o`) which you can link with your own application.

**1. Clone the repository with submodules:**
```bash
git clone --recurse-submodules https://github.com/kirill-bayborodov/bignum-add-bignum.git
cd bignum-add-bignum
```

**2. Build the object file:**
```bash
make build
```
The output will be located at `build/bignum_add_bignum.o`.

**3. Link with your application:**
When compiling your project, include the object file and specify the include paths for the headers.
```bash
gcc your_app.c build/bignum_add_bignum.o -I./include -I./libs/bignum-common/include -o your_app -no-pie
```

## Contributing

Contributions are welcome! Please follow these steps:
1.  Fork the repository.
2.  Create a new branch (`git checkout -b feature/AmazingFeature`).
3.  Make your changes.
4.  Commit your changes (`git commit -m 'Add some AmazingFeature'`).
5.  Push to the branch (`git push origin feature/AmazingFeature`).
6.  Open a Pull Request.

When creating Issues or Pull Requests, please use the provided templates to ensure all necessary information is included.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
