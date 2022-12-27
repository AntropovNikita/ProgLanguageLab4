#define _DEFAULT_SOURCE

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#include "mem_internals.h"
#include "mem.h"
#include "util.h"

#define NO_ADDITIONAL_FLAG 0 // Заглушка для дополнительного флага  при вызове mmap


extern inline block_size size_from_capacity( block_capacity cap );
extern inline block_capacity capacity_from_size( block_size sz );

/**
 * @brief Проверка вместимости блока памяти
 * @param[in] query Пороговый размер в байтах
 * @param[in] block Указател на структуру блока памяти
 * @return true, если вместимость блока больше порогового размера, иначе false
*/
static bool block_is_big_enough( size_t query, struct block_header* block ) { return block->capacity.bytes >= query; }

/**
 * @brief Расчет кол-ва страниц памяти
 * @param[in] mem Размер памяти в байтах
 * @return Кол-во страниц
*/
static size_t pages_count( size_t mem ) { return mem / getpagesize() + ((mem % getpagesize()) > 0); }

/**
 * @brief Расчет общего кол-ва байт на все страницы
 * @param[in] mem Размер памяти в байтах
 * @return Кол-во байт суммарное 
*/ 
static size_t round_pages( size_t mem ) { return getpagesize() * pages_count( mem ) ; }

/**
 * @brief Инициализация блока памяти по заданному адресу
 * @param[out] addr Указатель на адрес в памяти
 * @param[in] block_sz Размер блока в байтах
 * @param[in] next Указатель на следующий блок в памяти
*/
static void block_init( void* restrict addr, block_size block_sz, void* restrict next ) 
{
  *((struct block_header*)addr) = (struct block_header) {
    .next = next,
    .capacity = capacity_from_size(block_sz),
    .is_free = true
  };
}

/**
 * @brief Расчет действительного размера региона памяти
 * @param[in] query Запрашивая память в байтах
 * @return Действительный размер региона
*/
static size_t region_actual_size( size_t query ) { return size_max( round_pages( query ), REGION_MIN_SIZE ); }

extern inline bool region_is_invalid( const struct region* r );

/**
 * @brief Резервация и разметка памяти через mmap
 * @param[in] addr Указатель на желаемый адреас в памяти
 * @param[in] length Размер запрашиваемой памяти в байтах
 * @param[in] additional_falgs Дополнительные флаги для вызова mmap
 * @return Действительный адрес начала выделенной памяти
*/
static void* map_pages(void const* addr, size_t length, int additional_flags)
{
  return mmap( (void*) addr, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | additional_flags , -1, 0 );
}

/*  аллоцировать регион памяти и инициализировать его блоком */
/**
 * @brief Аллокация региона памяти и инициализация блоком
 * @param[in] addr Указатель на адрес начала региона
 * @param[in] query Запрашиваемый размер в байтах
 * @return Структурп региона
*/
static struct region alloc_region( void const * addr, size_t query ) 
{
  struct region reg;
  query = region_actual_size(query); // Выбор действительного размера региона
  void* next_addr = map_pages(addr, query, MAP_FIXED_NOREPLACE); // Пробуем выделить память строго по текущему адресу

  if (next_addr != MAP_FAILED) // Если удалось выделить память
  {
    reg.addr = next_addr;
    reg.size = query;
    reg.extends = true;
  }
  else // Если не удалось выделить память
  {
    next_addr = map_pages(addr, query, NO_ADDITIONAL_FLAG); // Пробуем выделить память, где получится
    reg.addr = next_addr;
    reg.size = query;
    reg.extends = false;
  }

  block_init(next_addr, (block_size){.bytes = query}, NULL); // Инициализация блока в регионе
  return reg;
}

/**
 * @brief Получение адреса начала следующего блока
 * @param[in] block Указатель на структуру текущего блока
 * @return Указатель на начало следующего блока
*/
static void* block_after( struct block_header const* block );

void* heap_init( size_t initial ) 
{
  const struct region region = alloc_region( HEAP_START, initial );
  if ( region_is_invalid(&region) ) 
    return NULL;
  return region.addr;
}

void heap_kill(void* heap, size_t size)
{
  if (heap != NULL)
    munmap(heap, size);
}

#define BLOCK_MIN_CAPACITY 24 // Минимальный размер блока в байтах

/*  --- Разделение блоков (если найденный свободный блок слишком большой )--- */
/**
 * @brief Проверка того, можно ли разделить блока на два меньших
 * @param[in] block Указатель на структуру блока
 * @param[in] query Запрашиваемая память в байтах
 * @return true, если блок свободен и требуемое кол-во памяти меньше вместимости текущего блока, иначе false
*/
static bool block_splittable( struct block_header* restrict block, size_t query)
{
  return block-> is_free && query + offsetof( struct block_header, contents ) + BLOCK_MIN_CAPACITY <= block->capacity.bytes;
}

/**
 * @brief Деление блока, если он большой для запрашиваемого кол-ва памяти
 * @param[out] block Указатель на структуру блока
 * @param[in] query Запрашиваемая память в байтах
 * @return true, если блок удалось поделить, иначе false
*/
static bool split_if_too_big( struct block_header* block, size_t query ) 
{
  query = size_max(query, BLOCK_MIN_CAPACITY); // Выбор действительного размера запрашиваемой памяти
  if (!block_splittable(block, query)) // Если блок нельзя поделить
    return false;

  block_size size = { // Уменьшение размера текущего блока
    .bytes = block->capacity.bytes - query
  };
  struct block_header* new_block = (struct block_header*)(block->contents + query); // Иницализация нового пустого блока
  block_init(new_block, size, block->next);
  block->capacity.bytes = query; 
  block->next = new_block;

  return true;
}

/*  --- Слияние соседних свободных блоков --- */
static void* block_after( struct block_header const* block )       
{
  return  (void*) (block->contents + block->capacity.bytes);
}

/**
 * @brief Проверка последовательности блоков
 * @param[in] fst Указатель на структуру первого блока
 * @param[in] snd Указатель на структуру второго блока
 * @return true, если второй блок идет после первого, иначе false
*/
static bool blocks_continuous ( struct block_header const* fst, struct block_header const* snd ) 
{
  return (void*)snd == block_after(fst);
}

/**
 * @brief Проверка возможности слияние двух блоков
 * @param[in] fst Указатель на структуру первого блока
 * @param[in] snd Указатель на структуру второго блока
 * @return true, если оба блока свободны и идут друг за другом, иначе false 
*/
static bool mergeable(struct block_header const* restrict fst, struct block_header const* restrict snd) 
{
  return fst->is_free && snd->is_free && blocks_continuous( fst, snd ) ;
}

/**
 * @brief Слияние текущего блока со следующим
 * @param[out] block Указатель на структуру текущего блока
 * @return true, если слияние произошло, иначе false
*/
static bool try_merge_with_next( struct block_header* block ) 
{
  if (block->next) // Если следущющий блок существует
  {
    struct block_header* restrict next_block = block->next; 
    if (next_block && mergeable(block, next_block)) // Если блоки можно слить
    {
      block->next = next_block->next;
      block->capacity.bytes += offsetof(struct block_header, contents) + next_block->capacity.bytes;
      return true;
    }
  }
  return false;
}


/*  --- ... ecли размера кучи хватает --- */
/**
 * @brief Структура результата поиска блока
*/
struct block_search_result 
{
  enum {BSR_FOUND_GOOD_BLOCK, BSR_REACHED_END_NOT_FOUND, BSR_CORRUPTED} type; /** Результат поиска */
  struct block_header* block; /** Указатель на структуру найденного блока*/
};

/**
 * @brief Поиск хорошего блока
 * @param[in] block Указатель на структуру текущего блока
 * @param[in] sz Запрашиваемый размер блока в байтах
 * @return Структура с результатами поиска
*/
static struct block_search_result find_good_or_last  ( struct block_header* restrict block, size_t sz )   
{
  struct block_header* cur_block = block;
  struct block_search_result res;

  if (!cur_block) // Если указатель на блок невалидный
  {
    res.type = BSR_CORRUPTED;
    res.block = cur_block;
    return res;
  }
  while (cur_block->next) // Перебор блоков (Первое приближение)
  {
    if (cur_block->is_free && block_is_big_enough(sz, cur_block)) // Если блок свободен и больше необходимого размера
    {
      res.type = BSR_FOUND_GOOD_BLOCK;
      res.block = cur_block;
      return res;
    }
    if (!try_merge_with_next(cur_block)) // Если не получается слить текущий блок со следующим
      cur_block = cur_block->next;
  }
  if (cur_block->is_free && block_is_big_enough(sz, cur_block)) // Проверка последнего блока
  {
    res.type = BSR_FOUND_GOOD_BLOCK;
    res.block = cur_block;
    return res;
  }
  res.type = BSR_REACHED_END_NOT_FOUND; // Достигли конца региона
  res.block = cur_block;
  return res;
}

/*  Попробовать выделить память в куче начиная с блока `block` не пытаясь расширить кучу
 Можно переиспользовать как только кучу расширили. */
 /**
  * @brief Попытка выделить память в куче с заданного блока
  * @param[in] query Запрашиваемый размер в байтах
  * @param[in] block Указатель на структуру текущего блока
  * @return Структура с результатами поиска
 */  
static struct block_search_result try_memalloc_existing ( size_t query, struct block_header* block )
{
  query = size_max(query, BLOCK_MIN_CAPACITY); // Выбор действительного размера запрашиваемой памяти
  struct block_search_result res = find_good_or_last(block, query);
  if (res.type == BSR_FOUND_GOOD_BLOCK) // Если блок найден
  {
    split_if_too_big(res.block, query); // Пробуем уменьшить
    res.block->is_free = false;
  }
  return res;
}

/**
 * @brief Увеличение размера кучи
 * @param[out] last Указатель на последний блок в куче
 * @param[in] query Запрашиваемая память в байтах
 * @return Указатель на начало нового региона
*/
static struct block_header* grow_heap( struct block_header* restrict last, size_t query ) 
{
  query += offsetof(struct block_header, contents);
  const struct region reg = alloc_region(block_after(last), query);

  if (region_is_invalid(&reg)) // если выделить память не получилось
    return NULL;

  last->next = (struct block_header*) reg.addr;
  if (try_merge_with_next(last)) // Попытка объелинить новый блок с последним из кучи
    return last;
  return last->next;
}

/*  Реализует основную логику malloc и возвращает заголовок выделенного блока */
/**
 * @brief Выделение памяти 
 * @param[in] query Запрашиваемая память в байтах
 * @param[in] heap_start Указатель на начало кучи
 * @return Указатель на заголовок выделенного блока
*/
static struct block_header* memalloc( size_t query, struct block_header* heap_start)
{
  query = size_max(query, BLOCK_MIN_CAPACITY); // Выбор действительного размера запрашиваемой памяти
  struct block_search_result res = try_memalloc_existing(query, heap_start); // Выбор блока без расширения кучи

  if (res.type != BSR_CORRUPTED) // Если адрес кучи был валидным
  {
    if (res.type != BSR_FOUND_GOOD_BLOCK) // Если не удалось найти хороший блок
    {
      struct block_header* head = grow_heap(res.block, query); // Увеличение кучи
      if (!head)
        return NULL;
      res = try_memalloc_existing(query, res.block); // Повторный поиск
      if (res.type == BSR_CORRUPTED) // Если адрес последнего блока кучи не валидный
        return NULL;
    }
    res.block->is_free = false;
    return res.block;
  }
  return NULL;
}

void* _malloc( size_t query ) 
{
  struct block_header* const addr = memalloc( query, (struct block_header*) HEAP_START );
  if (addr) 
    return addr->contents;
  else 
    return NULL;
}

/**
 * @brief Получение заголовка блока
 * @param[in] contents Указатель на адрес данных блока
*/
static struct block_header* block_get_header(void* contents)
{
  return (struct block_header*) (((uint8_t*)contents)-offsetof(struct block_header, contents));
}

void _free( void* mem ) 
{
  if (!mem) 
    return ;
  struct block_header* header = block_get_header( mem );
  header->is_free = true;
  while (header->next && try_merge_with_next(header));
}
