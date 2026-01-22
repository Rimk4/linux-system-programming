#include "thread_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

// Глобальный обработчик ошибок
static error_handler_t error_handler = NULL;

// Установка обработчика ошибок
void thread_pool_set_error_handler(error_handler_t handler) {
    error_handler = handler;
}

// Вспомогательная функция для вывода ошибок
static void thread_pool_error(const char* msg) {
    if (error_handler) {
        error_handler(msg);
    } else {
        fprintf(stderr, "Thread Pool Error: %s\n", msg);
    }
}

// Функция потока в пуле
static void* thread_pool_worker(void* thread_pool) {
    thread_pool_t* pool = (thread_pool_t*)thread_pool;
    task_t* task;
    
    while (true) {
        pthread_mutex_lock(&pool->lock);
        
        // Ожидание задачи или сигнала завершения
        while (pool->queue_size == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->notify, &pool->lock);
        }
        
        // Проверка флага завершения
        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->lock);
            pthread_exit(NULL);
        }
        
        // Извлечение задачи из очереди
        task = pool->task_queue;
        if (task != NULL) {
            pool->task_queue = task->next;
            pool->queue_size--;
            
            // Обновление хвоста очереди, если нужно
            if (pool->task_queue == NULL) {
                pool->task_queue_tail = NULL;
            }
            
            pool->count++; // Увеличиваем счетчик активных потоков
        }
        
        pthread_mutex_unlock(&pool->lock);
        
        // Выполнение задачи
        if (task != NULL) {
            task->function(task->arg);
            free(task);
            
            pthread_mutex_lock(&pool->lock);
            pool->count--; // Уменьшаем счетчик активных потоков
            pthread_mutex_unlock(&pool->lock);
        }
    }
    
    return NULL;
}

// Создание пула потоков
thread_pool_t* thread_pool_create(int num_threads) {
    if (num_threads <= 0) {
        num_threads = 4; // Значение по умолчанию
    }
    
    thread_pool_t* pool = (thread_pool_t*)malloc(sizeof(thread_pool_t));
    if (!pool) {
        thread_pool_error("Не удалось выделить память для пула потоков");
        return NULL;
    }
    
    // Инициализация полей
    pool->thread_count = num_threads;
    pool->queue_size = 0;
    pool->count = 0;
    pool->shutdown = false;
    pool->dynamic_scaling = false;
    pool->task_queue = NULL;
    pool->task_queue_tail = NULL;
    
    // Инициализация мьютекса и условной переменной
    if (pthread_mutex_init(&pool->lock, NULL) != 0) {
        free(pool);
        thread_pool_error("Не удалось инициализировать мьютекс");
        return NULL;
    }
    
    if (pthread_cond_init(&pool->notify, NULL) != 0) {
        pthread_mutex_destroy(&pool->lock);
        free(pool);
        thread_pool_error("Не удалось инициализировать условную переменную");
        return NULL;
    }
    
    // Выделение памяти для потоков
    pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * num_threads);
    if (!pool->threads) {
        pthread_mutex_destroy(&pool->lock);
        pthread_cond_destroy(&pool->notify);
        free(pool);
        thread_pool_error("Не удалось выделить память для потоков");
        return NULL;
    }
    
    // Создание потоков
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&pool->threads[i], NULL, thread_pool_worker, pool) != 0) {
            // В случае ошибки, завершаем уже созданные потоки
            pool->shutdown = true;
            pthread_cond_broadcast(&pool->notify);
            
            for (int j = 0; j < i; j++) {
                pthread_join(pool->threads[j], NULL);
            }
            
            free(pool->threads);
            pthread_mutex_destroy(&pool->lock);
            pthread_cond_destroy(&pool->notify);
            free(pool);
            thread_pool_error("Не удалось создать поток");
            return NULL;
        }
    }
    
    printf("Пул потоков создан с %d потоками\n", num_threads);
    return pool;
}

// Расширенное создание пула
thread_pool_t* thread_pool_create_advanced(int min_threads, int max_threads, 
                                           bool dynamic_scaling) {
    if (min_threads <= 0) min_threads = 2;
    if (max_threads <= min_threads) max_threads = min_threads * 2;
    if (max_threads > 64) max_threads = 64; // Ограничение для безопасности
    
    thread_pool_t* pool = thread_pool_create(min_threads);
    if (!pool) return NULL;
    
    pool->dynamic_scaling = dynamic_scaling;
    pool->min_threads = min_threads;
    pool->max_threads = max_threads;
    
    printf("Расширенный пул потоков создан: %d-%d потоков, динамическое масштабирование: %s\n",
           min_threads, max_threads, dynamic_scaling ? "вкл" : "выкл");
    
    return pool;
}

// Добавление задачи в пул
int thread_pool_add_task(thread_pool_t* pool, void (*function)(void*), void* arg) {
    if (!pool || !function) {
        return -1;
    }
    
    task_t* task = (task_t*)malloc(sizeof(task_t));
    if (!task) {
        thread_pool_error("Не удалось выделить память для задачи");
        return -1;
    }
    
    task->function = function;
    task->arg = arg;
    task->next = NULL;
    
    pthread_mutex_lock(&pool->lock);
    
    // Добавление задачи в очередь
    if (pool->task_queue_tail) {
        pool->task_queue_tail->next = task;
        pool->task_queue_tail = task;
    } else {
        pool->task_queue = task;
        pool->task_queue_tail = task;
    }
    
    pool->queue_size++;
    
    // Сигнал одному ожидающему потоку
    pthread_cond_signal(&pool->notify);
    pthread_mutex_unlock(&pool->lock);
    
    return 0;
}

// Ожидание завершения всех задач
int thread_pool_wait(thread_pool_t* pool) {
    if (!pool) return -1;
    
    // Ожидаем, пока очередь не опустеет и все потоки не освободятся
    while (true) {
        pthread_mutex_lock(&pool->lock);
        int queue_empty = (pool->queue_size == 0);
        int all_idle = (pool->count == 0);
        pthread_mutex_unlock(&pool->lock);
        
        if (queue_empty && all_idle) {
            break;
        }
        
        usleep(10000); // 10 мс задержка
    }
    
    return 0;
}

// Уничтожение пула потоков
int thread_pool_destroy(thread_pool_t* pool) {
    if (!pool) return -1;
    
    pthread_mutex_lock(&pool->lock);
    pool->shutdown = true;
    pthread_mutex_unlock(&pool->lock);
    
    // Сигнал всем потокам
    pthread_cond_broadcast(&pool->notify);
    
    // Ожидание завершения всех потоков
    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    
    // Освобождение оставшихся задач в очереди
    task_t* task;
    while (pool->task_queue != NULL) {
        task = pool->task_queue;
        pool->task_queue = pool->task_queue->next;
        free(task);
    }
    
    // Освобождение ресурсов
    free(pool->threads);
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->notify);
    free(pool);
    
    printf("Пул потоков уничтожен\n");
    return 0;
}

// Получение статистики пула
thread_pool_stats_t thread_pool_get_stats(thread_pool_t* pool) {
    thread_pool_stats_t stats = {0};
    
    if (!pool) return stats;
    
    pthread_mutex_lock(&pool->lock);
    stats.active_threads = pool->count;
    stats.queued_tasks = pool->queue_size;
    stats.current_queue_size = pool->queue_size;
    pthread_mutex_unlock(&pool->lock);
    
    return stats;
}
