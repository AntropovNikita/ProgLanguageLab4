#ifndef _TESTS_H_
#define _TESTS_H_


/**
 * @defgroup TESTS Тесты для аллокатора
*/
/**
 * @brief Запуск все тестов
*/
void all_test();

/**@{*/
/**
 * @brief Тест на обычное выделение памяти
 * @param[in] heap Указатель на кучу
*/
void simple_alloc_test();

/**
 * @brief Тест на освобождение одного блока из нескольких выделенных
 * @param[in] heap Указатель на кучу
*/
void free_one_block_test();

/**
 * @brief Тест на освобождение двух блоков из нескольких выделенных
 * @param[in] heap Указатель на кучу
*/
void free_two_block_test();

/**
 * @brief Тест на расширение кучи (регионы идут последовательно)
 * @param[in] heap Указатель на кучу
*/
void extend_heap_test();

/**
 * @brief Тест на расширение кучи (регионы идут не последовательно)
 * @param[in] heap Указатель на кучу
*/
void continue_heap_test();
/**@}*/

#endif // !_TESTS_H_