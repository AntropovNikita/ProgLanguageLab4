#include "tests.h"

#ifndef DEBUG
 #define DEBUG
#endif // !DEBUG

#define _DEFAULT_SOURCE

#include "util.h"
#include "mem.h"
#include "mem_debug.h"

#include <errno.h>
#include <string.h>

#define HEAP_INIT_SIZE 10000


/**
 * @brief Тест на выделение кучи
 * @param[in] size Размер кучи
 * @param[in] test_num Номер теста
 * @return Указатель на кучу
*/
static void* heap_init_test(size_t size, const uint16_t test_num);

/**
 * @brief Тест на выделение памяти в куче
 * @param[in] size Запращиваемый размер
 * @param[in] test_num Номер теста
 * @param[in] heap Указатель на кучу
 * @param[in] data_type Тип данных для выделения памяти
 * @return Указатель на блок в памяти
*/
static void* malloc_test(size_t size, const uint16_t test_num, const void* heap, const char* data_type);

/**
 * @brief Тест на освобождение памяти
 * @param[in] data Указатель на блок в памяти
 * @param[in] heap Указатель на кучу
 * @param[in] data_type Тип данных для выделения памяти
*/
static void free_test(void* data, const void* heap, const char* data_type);

/**
 * @brief Р
*/
static void* make_mmap(void* addr, const uint16_t test_num);

void all_test()
{
    simple_alloc_test();
    free_one_block_test();
    free_two_block_test();
    extend_heap_test();
    continue_heap_test();
}

void simple_alloc_test()
{
    static const uint16_t test_num = 1;
    debug("Тест %d. Обычное выделение памяти\n", test_num);

    void* heap = heap_init_test(HEAP_INIT_SIZE, test_num);

    uint16_t* data = (uint16_t*) malloc_test(sizeof(uint16_t), test_num, heap, "uint16_t");

    uint32_t* arr = malloc_test(sizeof(uint32_t)*100, test_num, heap, "массив uint32_t размера 100");

    debug("Тест %d пройден\n\n", test_num);

    _free(arr);
    _free(data);

    heap_kill(heap, HEAP_INIT_SIZE);
}

void free_one_block_test()
{
    static const uint16_t test_num = 2;
    debug("Тест %d. Освобождение одного блока из нескольких выделенных\n", test_num);

    void* heap = heap_init_test(HEAP_INIT_SIZE, test_num);

    uint16_t* data = malloc_test(sizeof(uint16_t), test_num, heap, "uint16_t");
    uint32_t* arr = malloc_test(sizeof(uint32_t)*100, test_num, heap, "массив uint32_t размера 100");
    uint8_t* arr2 = malloc_test(sizeof(uint8_t)*100, test_num, heap, "массив uint8_t размера 100");

    free_test(arr, heap, "массив uint32_t");

    debug("Тест %d пройден\n\n", test_num);

    _free(arr2);
    _free(data);

    heap_kill(heap, HEAP_INIT_SIZE);
}

void free_two_block_test()
{
    static const uint16_t test_num = 3;
    debug("Тест %d. Освобождение двух блоков из нескольких выделенных\n", test_num);

    void* heap = heap_init_test(HEAP_INIT_SIZE, test_num);

    uint16_t* data = malloc_test(sizeof(uint16_t), test_num, heap, "uint16_t");
    uint32_t* arr = malloc_test(sizeof(uint32_t)*100, test_num, heap, "массив uint32_t размера 100");
    uint8_t* arr2 = malloc_test(sizeof(uint8_t)*100, test_num, heap, "массив uint8_t размера 100");

    free_test(arr, heap, "массив uint32_t");
    free_test(arr2, heap, "массив uint8_t");

    debug("Тест %d пройден\n\n", test_num);
    _free(data);

    heap_kill(heap, HEAP_INIT_SIZE);
}

void extend_heap_test()
{
    static const uint16_t test_num = 4;
    debug("Тест %d. Расширение кучи, регионы идут последовательно\n", test_num);

    void* heap = heap_init_test(HEAP_INIT_SIZE, test_num);

    debug("Куча в начале:\n");
    debug_heap(stderr, heap);
    uint32_t* arr = malloc_test(sizeof(uint32_t)*3500, test_num, heap, "массив uint32_t размера 3500");
    uint8_t* arr2 = malloc_test(sizeof(uint8_t)*100, test_num, heap, "массив uint8_t размера 100");

    _free(arr2);
    _free(arr);
    debug("Куча после освобождения памяти:\n");
    debug_heap(stderr, heap);

    debug("Тест %d пройден\n\n", test_num);

    heap_kill(heap, 28655);
}

void continue_heap_test()
{
    static const uint16_t test_num = 5;
    debug("Тест %d. Расширение кучи, регионы идут не последовательно\n", test_num);

    void* heap = heap_init_test(HEAP_INIT_SIZE, test_num);

    struct block_header* header = (struct block_header*) HEAP_START;
    void* split_mem = make_mmap((void*) (header->contents + header->capacity.bytes), test_num);
    
    int8_t* arr = malloc_test(sizeof(uint8_t)*1000000, test_num, heap, "массив uint8_t размера 100");
    _free(arr);

    debug("Куча после освобождения памяти:\n");
    debug_heap(stderr, heap);

    debug("Тест %d пройден\n\n", test_num);

    heap_kill(heap, 10000);
    heap_kill(split_mem, 200000);
}

static void* heap_init_test(size_t size, const uint16_t test_num)
{
    debug("Инициализация кучи с размером %d. Результат:\n", HEAP_INIT_SIZE);
    void* heap = heap_init(size);
    if (heap == NULL)
        err("Ошибка: Не удалось инициализировать кучу. Тест %d не пройден\n", test_num);
    debug_heap(stderr, heap);

    return heap;
}

static void* malloc_test(size_t size, const uint16_t test_num, const void* heap, const char* data_type)
{
    debug("Выделение памяти под %s. Результат:\n", data_type);
    void* data = _malloc(size);
    if (data == NULL)
        err("Ошибка: Не удалось выделить память. Тест %d не пройден\n", test_num);
    debug_heap(stderr, heap);

    return data;
}

static void free_test(void* data, const void* heap, const char* data_type)
{
    debug("Освобождение памяти под %s. Результат:\n", data_type);
    _free(data);
    debug_heap(stderr, heap);
}

static void* make_mmap(void* addr, const uint16_t test_num)
{
    void* split_page =  mmap( addr, REGION_MIN_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0 );
    debug("%s\n", strerror(errno));
    if (split_page == MAP_FAILED)
        err("Ошибка: не удалось выделить память под заглушку-разделитель. Тест %d не пройден\n", test_num);

    addr = split_page;
    *((struct block_header*)split_page) = (struct block_header) {
        .next = NULL,
        .capacity = capacity_from_size((block_size){REGION_MIN_SIZE}),
        .is_free = false
    };

    return addr;
}