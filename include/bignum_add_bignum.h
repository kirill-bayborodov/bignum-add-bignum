/**
 * @file    bignum_add_bignum.h
 * @author  git@bayborodov.com
 * @version 1.0.0
 * @date    29.07.2026
 *
 * @brief   Модуль для сложения больших беззнаковых целых чисел.
 * @ingroup bignum_arithmetic
 *
 * @details
 *   Определяет API для функции bignum_add_bignum, включая типы данных,
 *   коды состояния и прототипы функций.
 *
 * @history
 *   - rev. 0 (29.07.2026): Первоначальное создание API.
 *
 * @see     bignum.h
 * @since   1.0.2
 */

#ifndef BIGNUM_ADD_BIGNUM_H
#define BIGNUM_ADD_BIGNUM_H

#include <bignum.h>
#include <stddef.h>
#include <stdint.h>

// Проверка на наличие определения BIGNUM_CAPACITY из общего заголовка
#ifndef BIGNUM_CAPACITY
#  error "bignum.h must define BIGNUM_CAPACITY"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Коды состояния для функции bignum_add_bignum.
 */
typedef enum {
    BIGNUM_ADD_BIGNUM_SUCCESS                 =  0, /**< Успешное выполнение. */
    BIGNUM_ADD_BIGNUM_ERROR_NULL_PTR          = -1, /**< Один из входных указателей `NULL`. */
    BIGNUM_ADD_BIGNUM_ERROR_CAPACITY_EXCEEDED = -2, /**< Длина операнда (a или b) превышает `BIGNUM_CAPACITY`. */
    BIGNUM_ADD_BIGNUM_ERROR_BUFFER_OVERLAP    = -3, /**< Обнаружено недопустимое перекрытие буферов. */
    BIGNUM_ADD_BIGNUM_ERROR_OVERFLOW          = -4  /**< Переполнение: результат не помещается в `BIGNUM_CAPACITY`. */
} bignum_add_bignum_status_t;

/**
 * @brief Выполняет сложение двух больших беззнаковых целых чисел.
 *
 * @details
 *   ### Алгоритм
 *   1.  **Валидация:** Проверяются входные указатели `result`, `a`, `b` на `NULL`.
 *   2.  **Проверка длины:** Проверяется, что `a->len` и `b->len` не превышают `BIGNUM_CAPACITY`.
 *   3.  **Проверка перекрытия:** Проверяется, что диапазоны памяти `result`, `a` и `b` 
 *       не пересекаются (при этом in-place операции `result == a` или `result == b` разрешены).
 *   4.  **Сложение:** Выполняется пословное сложение `a + b` с распространением переноса (carry).
 *   5.  **Обработка хвоста:** Если один операнд длиннее другого, оставшиеся слова 
 *       копируются с учетом возможного продолжения переноса.
 *   6.  **Переполнение:** Если после сложения всех слов остается перенос, и текущая длина 
 *       равна `BIGNUM_CAPACITY`, возвращается ошибка `BIGNUM_ADD_BIGNUM_ERROR_OVERFLOW`.
 *       Иначе длина результата увеличивается на 1, и перенос записывается в старшее слово.
 *   7.  **Нормализация:** Длина результата устанавливается корректно (для сложения нулей).
 *
 *   ### Потокобезопасность
 *   Функция является потокобезопасной, так как не использует глобальное или
 *   статическое состояние.
 *
 * @param[out] result Указатель на структуру `bignum_t` для записи суммы.
 * @param[in]  a      Указатель на `bignum_t`, первое слагаемое.
 * @param[in]  b      Указатель на `bignum_t`, второе слагаемое.
 *
 * @return bignum_add_bignum_status_t Код состояния операции.
 * @retval BIGNUM_ADD_BIGNUM_SUCCESS                 Успешное выполнение.
 * @retval BIGNUM_ADD_BIGNUM_ERROR_NULL_PTR          Один из входных указателей `NULL`.
 * @retval BIGNUM_ADD_BIGNUM_ERROR_CAPACITY_EXCEEDED Длина операнда (a или b) превышает `BIGNUM_CAPACITY`.
 * @retval BIGNUM_ADD_BIGNUM_ERROR_BUFFER_OVERLAP    Обнаружено перекрытие буферов.
 * @retval BIGNUM_ADD_BIGNUM_ERROR_OVERFLOW          Результат превышает максимальную вместимость.
 */
bignum_add_bignum_status_t bignum_add_bignum(bignum_t *result, const bignum_t *a, const bignum_t *b);

#ifdef __cplusplus
}
#endif

#endif /* BIGNUM_ADD_BIGNUM_H */
