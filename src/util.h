#ifndef _UTIL_H_
#define _UTIL_H_

#include <stddef.h>

/**
 * @defgroup UTIL Дополнительные функции
*/
/**@{*/
/**
 * @brief Выбор максимального размера
 * @param[in] x Первый размер
 * @param[in] y Второй размер
*/
inline size_t size_max( size_t x, size_t y ) { return (x >= y)? x : y ; }

/**
 * @brief Вывод сообщение об ошибке в stderr и прерывание программы
 * @param[in] msg Строка с сообщением
*/
_Noreturn void err( const char* msg, ... );
/**@}*/

#endif
