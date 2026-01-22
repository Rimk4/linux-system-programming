#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>

// Структура задачи для пула потоков
typedef struct task {
    void (*function)(void*);  // Функция для выполнения
    void* arg;                // Аргумент функции
    struct task* next;        // Следующая задача в очереди
} task_t;

// Структура пула потоков
typedef struct {
    pthread_mutex_t lock;     // Мьютекс для синхронизации
    pthread_cond_t notify;    // Условная переменная для уведомлений
    
    pthread_t* threads;       // Массив потоков
    task_t* task_queue;       // Очередь задач (голова)
    task_t* task_queue_tail;  // Хвост очереди задач
    
    int thread_count;         // Количество потоков
    int queue_size;           // Текущий размер очереди
    int count;                // Количество активных потоков
    bool shutdown;            // Флаг завершения работы
    bool dynamic_scaling;     // Динамическое масштабирование
    int min_threads;          // Минимальное количество потоков
    int max_threads;          // Максимальное количество потоков
} thread_pool_t;

// Создание пула потоков
thread_pool_t* thread_pool_create(int num_threads);

// Расширенное создание пула с динамическим масштабированием
thread_pool_t* thread_pool_create_advanced(int min_threads, int max_threads, 
                                           bool dynamic_scaling);

// Добавление задачи в пул
int thread_pool_add_task(thread_pool_t* pool, void (*function)(void*), void* arg);

// Добавление задачи с приоритетом
int thread_pool_add_task_with_priority(thread_pool_t* pool, 
                                       void (*function)(void*), 
                                       void* arg, 
                                       int priority);

// Ожидание завершения всех задач
int thread_pool_wait(thread_pool_t* pool);

// Уничтожение пула потоков
int thread_pool_destroy(thread_pool_t* pool);

// Получение статистики пула
typedef struct {
    int active_threads;
    int queued_tasks;
    int total_tasks_completed;
    int current_queue_size;
} thread_pool_stats_t;

thread_pool_stats_t thread_pool_get_stats(thread_pool_t* pool);

// Установка обработчика ошибок
typedef void (*error_handler_t)(const char* error_msg);
void thread_pool_set_error_handler(error_handler_t handler);

#endif // THREAD_POOL_H
