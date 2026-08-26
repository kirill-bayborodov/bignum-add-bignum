/**
 * @file bignum_add_bignum.c
 * @brief C11 reference implementation for fixed-capacity addition.
 * @details The reference validates all preconditions, computes the sum in the
 * caller's destination for successful operations, and never allocates memory.
 * It preserves the public status and aliasing contract used by the assembly
 * implementation. Independent calls are reentrant and thread-safe.
 */
#include "bignum_add_bignum.h"

/**
 * @brief Detects forbidden partial overlap between two bignum objects.
 * @details Exact identity is permitted for in-place addition. Otherwise the
 * byte ranges of both complete fixed-size objects are compared. The helper
 * does not dereference either object and returns only a validation result.
 * @param[in] result Candidate destination object.
 * @param[in] operand Candidate input object.
 * @return Non-zero when the complete object ranges partially overlap.
 */
static inline int check_buffer_overlap(const bignum_t *result,
                                       const bignum_t *operand)
{
    if (result == operand) {
        return 0;
    }

    const unsigned char *result_bytes = (const unsigned char *)result;
    const unsigned char *operand_bytes = (const unsigned char *)operand;
    return (result_bytes < operand_bytes + sizeof(bignum_t)) &&
           (operand_bytes < result_bytes + sizeof(bignum_t));
}

/**
 * @brief Adds two fixed-capacity bignum_t values using portable C11 arithmetic.
 * @details The implementation validates pointers, lengths, and aliasing before
 * writing. It adds the common words with carry, processes the longer tail,
 * reports a final carry as overflow when capacity is exhausted, normalizes the
 * logical length, and publishes the final length only after successful work.
 * @param[out] result Caller-owned destination; exact aliasing is allowed and
 *                    partial overlap is rejected.
 * @param[in] a Caller-owned first operand.
 * @param[in] b Caller-owned second operand.
 * @return A named bignum_add_bignum_status_t value; error paths leave result
 *         unchanged.
 */
bignum_add_bignum_status_t bignum_add_bignum(bignum_t *result,
                                             const bignum_t *a,
                                             const bignum_t *b)
{
    /* Validate before touching result so every failure is transactional. */
    if (!result || !a || !b) {
        return BIGNUM_ADD_BIGNUM_ERROR_NULL_PTR;
    }
    if (a->len > BIGNUM_CAPACITY || b->len > BIGNUM_CAPACITY) {
        return BIGNUM_ADD_BIGNUM_ERROR_CAPACITY_EXCEEDED;
    }
    if (check_buffer_overlap(result, a) || check_buffer_overlap(result, b)) {
        return BIGNUM_ADD_BIGNUM_ERROR_BUFFER_OVERLAP;
    }

    size_t min_len = (a->len < b->len) ? a->len : b->len;
    size_t max_len = (a->len > b->len) ? a->len : b->len;
    const bignum_t *longest = (a->len > b->len) ? a : b;
    uint64_t carry = 0;
    size_t i = 0;

    /* Add the common prefix and explicitly preserve both carry sources. */
    for (; i < min_len; ++i) {
        uint64_t a_word = a->words[i];
        uint64_t b_word = b->words[i];
        uint64_t sum = a_word + b_word;
        uint64_t carry_from_addends = (sum < a_word) ? 1U : 0U;
        uint64_t result_word = sum + carry;
        uint64_t carry_from_incoming = (result_word < sum) ? 1U : 0U;
        result->words[i] = result_word;
        carry = carry_from_addends | carry_from_incoming;
    }

    /* Copy the longer tail while propagating any carry into it. */
    for (; i < max_len; ++i) {
        uint64_t word = longest->words[i];
        uint64_t result_word = word + carry;
        carry = (result_word < word) ? 1U : 0U;
        result->words[i] = result_word;
    }

    /* A carry needs one extra word; do not publish a partial overflow result. */
    if (carry) {
        if (max_len == BIGNUM_CAPACITY) {
            return BIGNUM_ADD_BIGNUM_ERROR_OVERFLOW;
        }
        result->words[i] = 1U;
        ++max_len;
    }

    /* Canonicalize zero and remove any leading zero words from the length. */
    while (max_len > 0 && result->words[max_len - 1] == 0) {
        --max_len;
    }
    result->len = max_len;
    return BIGNUM_ADD_BIGNUM_SUCCESS;
}
