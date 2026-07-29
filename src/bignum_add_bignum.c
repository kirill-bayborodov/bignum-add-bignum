/**
 * @file    bignum_add_bignum.c
 * @author  git@bayborodov.com
 * @version 1.0.0
 * @date    29.07.2026
 *
 * @brief   Эталонная реализация сложения больших беззнаковых целых чисел на C11.
 */

#include "bignum_add_bignum.h"

/**
 * @brief Внутренняя функция для проверки недопустимого перекрытия буферов.
 *
 * @param[in] res Указатель на буфер результата.
 * @param[in] op  Указатель на входной буфер (операнд).
 * @return 1 если есть частичное перекрытие, 0 если всё в порядке или это in-place.
 */
static inline int check_buffer_overlap(const bignum_t *res, const bignum_t *op) {
    if (res == op) {
        return 0; // In-place операция (result == a или result == b) разрешена
    }
    
    const unsigned char *p_res = (const unsigned char *)res;
    const unsigned char *p_op  = (const unsigned char *)op;

    // Проверяем пересечение диапазонов памяти
    if ((p_res < p_op + sizeof(bignum_t)) && (p_op < p_res + sizeof(bignum_t))) {
        return 1;
    }
    return 0;
}

bignum_add_bignum_status_t bignum_add_bignum(bignum_t *result, const bignum_t *a, const bignum_t *b) {
    // 1. Валидация указателей
    if (!result || !a || !b) {
        return BIGNUM_ADD_BIGNUM_ERROR_NULL_PTR;
    }

    // 2. Проверка длины
    if (a->len > BIGNUM_CAPACITY || b->len > BIGNUM_CAPACITY) {
        return BIGNUM_ADD_BIGNUM_ERROR_CAPACITY_EXCEEDED;
    }

    // 3. Проверка перекрытия буферов
    if (check_buffer_overlap(result, a) || check_buffer_overlap(result, b)) {
        return BIGNUM_ADD_BIGNUM_ERROR_BUFFER_OVERLAP;
    }

    // 4. Подготовка к сложению
    size_t min_len = (a->len < b->len) ? a->len : b->len;
    size_t max_len = (a->len > b->len) ? a->len : b->len;
    const bignum_t *longest = (a->len > b->len) ? a : b;

    uint64_t carry = 0;
    size_t i = 0;

    // Сложение общей части (до min_len)
    for (; i < min_len; i++) {
        uint64_t w_a = a->words[i];
        uint64_t w_b = b->words[i];
        
        uint64_t sum = w_a + w_b;
        uint64_t carry1 = (sum < w_a) ? 1 : 0; // Переполнение при сложении a + b
        
        uint64_t res = sum + carry;
        uint64_t carry2 = (res < sum) ? 1 : 0; // Переполнение при добавлении carry
        
        result->words[i] = res;
        carry = carry1 | carry2;
    }

    // 5. Обработка хвоста более длинного числа
    for (; i < max_len; i++) {
        uint64_t w = longest->words[i];
        uint64_t res = w + carry;
        
        carry = (res < w) ? 1 : 0;
        result->words[i] = res;
    }

    // 6. Обработка финального переноса (переполнение)
    if (carry) {
        if (max_len == BIGNUM_CAPACITY) {
            return BIGNUM_ADD_BIGNUM_ERROR_OVERFLOW;
        }
        result->words[i] = 1;
        max_len++;
    }

    // 7. Нормализация результата (на случай, если складывали нули)
    while (max_len > 0 && result->words[max_len - 1] == 0) {
        max_len--;
    }
    result->len = max_len;

    return BIGNUM_ADD_BIGNUM_SUCCESS;
}

