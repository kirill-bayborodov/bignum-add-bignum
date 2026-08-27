/**
 * @file bignum_add_bignum_benchmark_adapter.c
 * @brief Benchmark-framework callbacks for bignum addition.
 * @details The adapter creates deterministic fixed-capacity operands, invokes
 * the public addition API, and exposes a result-sensitive checksum. No global
 * mutable state or dynamic allocation is used.
 */
#include "bignum_add_bignum_benchmark_adapter.h"
#include "bignum_add_bignum.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define FNV_OFFSET UINT64_C(1469598103934665603)
#define FNV_PRIME UINT64_C(1099511628211)

/**
 * @brief Holds one immutable-input addition benchmark state.
 * @details The state is owned by benchmark-core for the duration of a run.
 * Inputs are regenerated before each data item and result is overwritten by
 * the operation callback.
 */
typedef struct addition_state {
    bignum_t a; /**< First caller-independent benchmark operand. */
    bignum_t b; /**< Second caller-independent benchmark operand. */
    bignum_t result; /**< Operation output, valid after a successful callback. */
} addition_state_t;

/** @brief Compares two optional profile strings for exact equality. */
static int equal_text(const char *left, const char *right)
{
    return left != NULL && right != NULL && strcmp(left, right) == 0;
}

/**
 * @brief Tests whether a profile value belongs to a terminated vocabulary.
 * @param[in] value Candidate profile string.
 * @param[in] list NULL-terminated accepted string list.
 * @return Non-zero when value is present in list.
 */
static int allowed(const char *value, const char *const *list)
{
    if (value == NULL || list == NULL) return 0;
    for (size_t i = 0U; list[i] != NULL; ++i) {
        if (equal_text(value, list[i])) return 1;
    }
    return 0;
}

/**
 * @brief Advances the deterministic adapter PRNG.
 * @param[in,out] state Caller-owned per-item generator state.
 * @return The next deterministic 64-bit value.
 */
static uint64_t next_value(uint64_t *state)
{
    if (*state == 0U) *state = UINT64_C(0x9e3779b97f4a7c15);
    *state ^= *state << 7U;
    *state ^= *state >> 9U;
    *state ^= *state << 8U;
    return *state;
}

/**
 * @brief Maps a profile name to a representable operand length.
 * @param[in] workload Validated framework workload descriptor.
 * @param[in,out] state Deterministic generator state for variable profiles.
 * @return A length in the fixed bignum capacity range.
 */
static size_t choose_length(const benchmark_workload_t *workload, uint64_t *state)
{
    if (equal_text(workload->size_profile, "one") || equal_text(workload->size_profile, "tiny")) return 1U;
    if (equal_text(workload->size_profile, "quarter") || equal_text(workload->size_profile, "small")) return BIGNUM_CAPACITY / 4U;
    if (equal_text(workload->size_profile, "half") || equal_text(workload->size_profile, "medium")) return BIGNUM_CAPACITY / 2U;
    if (equal_text(workload->size_profile, "near-capacity") || equal_text(workload->size_profile, "large")) return BIGNUM_CAPACITY;
    return 1U + (size_t)(next_value(state) % (BIGNUM_CAPACITY / 2U));
}

/**
 * @brief Creates deterministic operands for one benchmark data item.
 * @details The top word is constrained so the addition workload remains
 * representable and does not measure an accidental overflow failure.
 */
static void fill_operands(addition_state_t *state, size_t length, uint64_t *random_state, int zero)
{
    memset(state, 0, sizeof(*state));
    if (zero) return;
    state->a.len = length == 0U ? 1U : length;
    state->b.len = state->a.len;
    for (size_t index = 0U; index < state->a.len; ++index) {
        state->a.words[index] = next_value(random_state);
        state->b.words[index] = state->a.words[index] >> 1U;
    }
    state->a.words[state->a.len - 1U] &= UINT64_C(0x3fffffffffffffff);
    if (state->a.words[state->a.len - 1U] == 0U) state->a.words[state->a.len - 1U] = 1U;
}

/**
 * @brief Initializes one framework data item.
 * @param[out] opaque Adapter-owned addition_state_t storage.
 * @param[in] index Deterministic data-item index.
 * @param[in] workload Validated framework workload.
 * @param[in] context Optional framework context, unused by this adapter.
 * @return Framework callback status; state is valid only on success.
 */
static benchmark_adapter_status_t initialize(void *opaque, uint64_t index,
                                               const benchmark_workload_t *workload, void *context)
{
    addition_state_t *state = opaque;
    uint64_t random_state;
    int zero;
    (void)context;
    if (state == NULL || workload == NULL ||
        bignum_add_bignum_benchmark_validate_workload(workload) != BIGNUM_ADD_BIGNUM_BENCHMARK_STATUS_SUCCESS) {
        return BENCHMARK_ADAPTER_STATUS_INPUT_ERROR;
    }
    random_state = workload->seed ^ (index + UINT64_C(0x9e3779b97f4a7c15));
    zero = equal_text(workload->input_kind, "zero") ||
           (equal_text(workload->input_kind, "mixed") && (index & 1U));
    fill_operands(state, choose_length(workload, &random_state), &random_state, zero);
    return BENCHMARK_ADAPTER_STATUS_SUCCESS;
}

/**
 * @brief Executes one addition operation for the current data item.
 * @param[in,out] opaque Initialized addition_state_t storage.
 * @param[in] iteration Framework iteration number.
 * @param[in] workload Current workload descriptor.
 * @param[in] context Optional framework context, unused by this adapter.
 * @return Framework success or operation error status.
 */
static benchmark_adapter_status_t operation(void *opaque, uint64_t iteration,
                                             const benchmark_workload_t *workload, void *context)
{
    addition_state_t *state = opaque;
    (void)iteration;
    (void)workload;
    (void)context;
    if (state == NULL || bignum_add_bignum(&state->result, &state->a, &state->b) != BIGNUM_ADD_BIGNUM_SUCCESS) {
        return BENCHMARK_ADAPTER_STATUS_OPERATION_ERROR;
    }
    return BENCHMARK_ADAPTER_STATUS_SUCCESS;
}

/**
 * @brief Computes a result-sensitive checksum after an operation.
 * @details Every result word and the logical length contribute to the hash, so
 * the compiler cannot discard the measured addition as an unobservable call.
 * @param[in] opaque Completed addition_state_t storage.
 * @param[in] iteration Framework iteration number.
 * @param[in] context Optional framework context, unused by this adapter.
 * @return Deterministic checksum, or zero for a NULL state.
 */
static uint64_t checksum(const void *opaque, uint64_t iteration, void *context)
{
    const addition_state_t *state = opaque;
    uint64_t hash = FNV_OFFSET;
    (void)context;
    if (state == NULL) return 0U;
    for (size_t index = 0U; index < BIGNUM_CAPACITY; ++index) {
        hash ^= state->result.words[index];
        hash *= FNV_PRIME;
    }
    hash ^= state->result.len;
    hash *= FNV_PRIME;
    return hash ^ iteration;
}

/**
 * @brief Validates the generic workload vocabulary accepted by addition.
 * @return Named adapter validation status; the descriptor is never modified.
 */
bignum_add_bignum_benchmark_status_t bignum_add_bignum_benchmark_validate_workload(
    const benchmark_workload_t *workload)
{
    static const char *const input[] = { "zero", "nonzero", "mixed", NULL };
    static const char *const operation_kind[] = { "add", "addition", "default", "mixed", "noop", NULL };
    static const char *const measure[] = { "end-to-end", "kernel-only", NULL };
    static const char *const size[] = { "one", "quarter", "half", "variable", "near-capacity", "tiny", "small", "medium", "large", NULL };
    static const char *const capacity[] = { "normal", "near-capacity", NULL };
    if (workload == NULL) return BIGNUM_ADD_BIGNUM_BENCHMARK_STATUS_NULL_ARGUMENT;
    if (!allowed(workload->input_kind, input) || !allowed(workload->operation_kind, operation_kind) ||
        !allowed(workload->measure_mode, measure) || !allowed(workload->size_profile, size) ||
        !allowed(workload->capacity_profile, capacity)) return BIGNUM_ADD_BIGNUM_BENCHMARK_STATUS_INVALID_PROFILE;
    return BIGNUM_ADD_BIGNUM_BENCHMARK_STATUS_SUCCESS;
}

/**
 * @brief Installs addition callbacks into a framework adapter.
 * @return Named adapter status; no allocation or ownership transfer occurs.
 */
bignum_add_bignum_benchmark_status_t bignum_add_bignum_benchmark_adapter_init(benchmark_adapter_t *adapter)
{
    if (adapter == NULL) return BIGNUM_ADD_BIGNUM_BENCHMARK_STATUS_NULL_ARGUMENT;
    *adapter = (benchmark_adapter_t){
        .benchmark_name = "bignum_add_bignum",
        .state_size = sizeof(addition_state_t),
        .success_code = BENCHMARK_ADAPTER_STATUS_SUCCESS,
        .adapter_context = NULL,
        .initialize = initialize,
        .operation = operation,
        .checksum = checksum
    };
    return BIGNUM_ADD_BIGNUM_BENCHMARK_STATUS_SUCCESS;
}
