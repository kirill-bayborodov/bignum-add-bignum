/**
 * @file bignum_add_bignum_benchmark_adapter.h
 * @brief Public benchmark-framework adapter contract for bignum addition.
 * @details The adapter binds deterministic bignum workloads to the generic
 * benchmark-core lifecycle. It owns no heap memory and does not transfer
 * ownership of framework or operand storage. Validation occurs before worker
 * execution and independent adapter instances are thread-safe.
 */
#ifndef BIGNUM_ADD_BIGNUM_BENCHMARK_ADAPTER_H
#define BIGNUM_ADD_BIGNUM_BENCHMARK_ADAPTER_H

#include <benchmark_framework.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reports adapter initialization or workload-validation outcome.
 * @details A successful status guarantees that the adapter binding or workload
 * validation completed. Error statuses leave caller-owned objects unchanged.
 */
typedef enum bignum_add_bignum_benchmark_status {
    BIGNUM_ADD_BIGNUM_BENCHMARK_STATUS_SUCCESS = 0, /**< Binding or validation succeeded. */
    BIGNUM_ADD_BIGNUM_BENCHMARK_STATUS_NULL_ARGUMENT = 1, /**< A required pointer was NULL. */
    BIGNUM_ADD_BIGNUM_BENCHMARK_STATUS_INVALID_PROFILE = 2 /**< Workload vocabulary or range was invalid. */
} bignum_add_bignum_benchmark_status_t;

/**
 * @brief Initializes the framework callback binding for addition.
 * @param[out] adapter Caller-allocated framework adapter; fields are written
 *                     on success and are unchanged on error.
 * @return A named bignum_add_bignum_benchmark_status_t value.
 * @retval BIGNUM_ADD_BIGNUM_BENCHMARK_STATUS_SUCCESS Callbacks are installed.
 * @retval BIGNUM_ADD_BIGNUM_BENCHMARK_STATUS_NULL_ARGUMENT adapter was NULL.
 * @pre adapter points to writable benchmark_adapter_t storage.
 * @post The adapter can be passed to benchmark-core on success.
 * @warning The caller retains ownership and must keep adapter storage alive for
 *          the complete benchmark run.
 */
bignum_add_bignum_benchmark_status_t bignum_add_bignum_benchmark_adapter_init(
    benchmark_adapter_t *adapter);

/**
 * @brief Validates addition-specific benchmark workload fields.
 * @param[in] workload Borrowed immutable workload descriptor supplied by the
 *                     framework; it remains caller-owned.
 * @return A named status describing validation. No allocation is performed.
 * @retval BIGNUM_ADD_BIGNUM_BENCHMARK_STATUS_SUCCESS All fields are supported.
 * @retval BIGNUM_ADD_BIGNUM_BENCHMARK_STATUS_NULL_ARGUMENT workload was NULL.
 * @retval BIGNUM_ADD_BIGNUM_BENCHMARK_STATUS_INVALID_PROFILE A field value was
 *         unsupported or outside the representable addition workload range.
 * @pre workload points to a valid descriptor for the duration of the call.
 * @post The descriptor is not modified.
 * @note Validation is O(1) in the descriptor size and is thread-safe.
 */
bignum_add_bignum_benchmark_status_t bignum_add_bignum_benchmark_validate_workload(
    const benchmark_workload_t *workload);

#ifdef __cplusplus
}
#endif

#endif /* BIGNUM_ADD_BIGNUM_BENCHMARK_ADAPTER_H */
