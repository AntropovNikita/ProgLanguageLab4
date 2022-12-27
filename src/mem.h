#ifndef _MEM_H_
#define _MEM_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <sys/mman.h>

#define HEAP_START ((void*)0x04040000) // Адрес начала кучи


/**
 * @defgroup MEM Основные операции с выделением памяти
*/
/**@{*/
/**
 * @brief Выделение памяти из кучи
 * @param[in] query Запрашиваемый размер в байтах
 * @return Указатель на адрес начала данных в памяти или NULL
*/
void* _malloc( size_t query );

/**
 * @brief Освобождение выделенной памяти
 * @param[in] mem Указател на адрес начала данных в памяти
*/
void  _free( void* mem );

/**
 * @brief Инициализация кучи
 * @param[in] initial_size Начальный размер кучи
 * @return Указатель на адрес начала кучи или NULL
*/
void* heap_init( size_t initial_size );

/**
 * @brief Удаление кучи
 * @param[in] heap Указатель на кучу
 * @param[in] size Размер кучи
*/
void heap_kill(void* heap, size_t size);
/**@}*/

#endif
