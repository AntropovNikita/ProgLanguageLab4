#ifndef _MEM_INTERNALS_
#define _MEM_INTERNALS_

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#define REGION_MIN_SIZE (2 * 4096) // Минимальный размер региона

/**
 * @defgroup MEM_INTERNALS Внутренние свойства памяти
*/
/**@{*/
/**
 * @brief Структра региона памяти
*/
struct region 
{ 
  void* addr;   /** Указательна начало региона в памяти */
  size_t size;  /** Размер региона в байтах*/
  bool extends; /** Флаг расширения */
};

/**
 * @brief Неинициализированный регион
*/
static const struct region REGION_INVALID = {0};

/**
 * @brief Проверка региона на невалидность
 * @param[in] r Указатель на структру региона
 * @return true, если регион неинициализированы, иначе false
*/
inline bool region_is_invalid( const struct region* r ) { return r->addr == NULL; }

/**
 * @brief Вместимость блока в байтах
*/
typedef struct { size_t bytes; } block_capacity;

/**
 * @brief Размер блока в байтах
*/
typedef struct { size_t bytes; } block_size;

/**
 * @brief Структура заголовка блока
*/
struct block_header {
  struct block_header* next; /** Указатель на следующий блок памяти */
  block_capacity capacity;   /** Вместимость блока в байтах */
  bool is_free;              /** Флаг занятости блока */
  uint8_t contents[];        /** Данные */
};

/**
 * @brief Расчет размера блока из его вместимости
 * @param[in] cap Вместимость блока в байтах
 * @return Размер блока в байтах
*/
inline block_size size_from_capacity( block_capacity cap ) { return (block_size) {cap.bytes + offsetof( struct block_header, contents ) }; }

/**
 *  @brief Расчет вместимости блока из его размера
 *  @param[in] sz Размер блока в байтах
 *  @return Вместимость блока в байтах
*/
inline block_capacity capacity_from_size( block_size sz ) { return (block_capacity) {sz.bytes - offsetof( struct block_header, contents ) }; }
/**@}*/

#endif
