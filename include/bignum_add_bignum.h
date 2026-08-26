/**
 * @file bignum_add_bignum.h
 * @brief Public API for fixed-capacity big-integer addition.
 * @details This header defines addition over the little-endian bignum_t
 * representation supplied by bignum-core. Callers own all input and output
 * storage; the library performs no allocation. Exact in-place destinations are
 * allowed, partial overlap is rejected, and errors are returned before a
 * result is published. The API is reentrant and thread-safe for independent
 * objects.
 */
#ifndef BIGNUM_ADD_BIGNUM_H
#define BIGNUM_ADD_BIGNUM_H

#include <bignum.h>
#include <stddef.h>
#include <stdint.h>

#ifndef BIGNUM_CAPACITY
#error "bignum.h must define BIGNUM_CAPACITY"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reports the outcome of a big-integer addition.
 * @details Success guarantees a complete normalized result. Every error
 * guarantees that the destination bytes are unchanged; callers may retry
 * after correcting the reported precondition. No status transfers ownership.
 */
typedef enum bignum_add_bignum_status {
    BIGNUM_ADD_BIGNUM_SUCCESS = 0, /**< Sum published and normalized. */
    BIGNUM_ADD_BIGNUM_ERROR_NULL_PTR = -1, /**< Required pointer is NULL; result is unchanged. */
    BIGNUM_ADD_BIGNUM_ERROR_CAPACITY_EXCEEDED = -2, /**< Input len exceeds capacity; result is unchanged. */
    BIGNUM_ADD_BIGNUM_ERROR_BUFFER_OVERLAP = -3, /**< Partial result/input overlap; result is unchanged. */
    BIGNUM_ADD_BIGNUM_ERROR_OVERFLOW = -4 /**< Sum needs more than capacity; result is unchanged. */
} bignum_add_bignum_status_t;

/**
 * @brief Adds two fixed-capacity unsigned big integers.
 * @details The function validates pointers, logical lengths, and memory
 * ranges, then adds corresponding little-endian words with carry propagation.
 * The longer operand tail is copied with carry, a final carry is represented
 * by one additional word when capacity permits, and the result is normalized.
 * Computation uses bounded storage and no dynamic allocation.
 *
 * @param[out] result Caller-allocated destination. Exact aliasing with `a` or
 *                    `b` is allowed; partial overlap is rejected. Unchanged
 *                    on every error status.
 * @param[in] a Borrowed first operand. It remains caller-owned and is not
 *               modified unless it is also `result` for an in-place call.
 * @param[in] b Borrowed second operand. It remains caller-owned and is not
 *               modified unless it is also `result` for an in-place call.
 * @return A named bignum_add_bignum_status_t describing publication or failure.
 * @retval BIGNUM_ADD_BIGNUM_SUCCESS The normalized sum was published.
 * @retval BIGNUM_ADD_BIGNUM_ERROR_NULL_PTR A required pointer was NULL.
 * @retval BIGNUM_ADD_BIGNUM_ERROR_CAPACITY_EXCEEDED An input length was too large.
 * @retval BIGNUM_ADD_BIGNUM_ERROR_BUFFER_OVERLAP A partial overlap was found.
 * @retval BIGNUM_ADD_BIGNUM_ERROR_OVERFLOW The sum exceeded fixed capacity.
 * @pre All non-NULL objects point to valid bignum_t storage for the call and
 *      input lengths are no greater than BIGNUM_CAPACITY.
 * @post On success result is complete and normalized; on error result bytes are
 *       unchanged.
 * @warning Concurrent writers of the same bignum_t require external
 *          synchronization even though independent calls are thread-safe.
 * @note Time complexity is O(max(a->len, b->len)); auxiliary space is O(1).
 */
bignum_add_bignum_status_t bignum_add_bignum(
    bignum_t *result, const bignum_t *a, const bignum_t *b);

#ifdef __cplusplus
}
#endif

#endif /* BIGNUM_ADD_BIGNUM_H */
