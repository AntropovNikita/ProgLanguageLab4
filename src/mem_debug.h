#ifndef _MEM_DEBUG_H_
#define _MEM_DEBUG_H_

#include "mem_internals.h"
#include <stdio.h>

#define DEBUG_FIRST_BYTES 4 // Кол-во байт данных для вывода


/**
 * @defgroup MEM_DEBUG Функции для вывода информации о работе с памятью
*/
/**@{*/
/**
 * @brief Вывод информации об блоке памяти в файл
 * @param[in] f Указатель на открытый файл
 * @param[in] address Указатель на адрес блока в памяти
*/ 
void debug_struct_info( FILE* f, void const* address );

/**
 * @brief Вывод информации о куче в файл
 * @param[in] f Указатель на открытый файл
 * @param[in] ptr Указатель на адрес первый блок
*/
void debug_heap( FILE* f,  void const* ptr );

/**
 * @brief Вывод инфморации об блока памяти с загловком в stderr
 * @param[in] b Указатель структуру блока памяти
 * @param[in] fmt Строка для форматируемого вывода
*/
void debug_block(struct block_header* b, const char* fmt, ...);

/**
 * @brief Форматируемый вывод в stderr
 * @param[in] fmt Строка для форматируемого вывода
*/
void debug(const char* fmt, ...);
/**@}*/

#endif //_MEM_DEBUG_H_