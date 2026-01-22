#include "thread_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

// Структура для передачи данных в задачу
typedef struct {
    int task_id;
    int duration_ms;
    char* message;
} task_data_t;

// Пример задачи для пула потоков
void example_task(void* arg) {
    task_data_t* data = (task_data_t*)arg;
    
    printf("Задача %d начата: %s\n", data->task_id, data->message);
    
    // Имитация работы (блокирующая операция)
    usleep(data->duration_ms * 1000);
    
    printf("Задача %d завершена за %d мс\n", data->task_id, data->duration_ms);
    
    // Освобождение памяти
    free(data->message);
    free(data);
}

// Задача вычисления чисел Фибоначчи
void fibonacci_task(void* arg) {
    int n = *(int*)arg;
    long long a = 0, b = 1, c;
    
    if (n <= 0) return;
    
    printf("Вычисление Fibonacci(%d)...\n", n);
    
    for (int i = 2; i <= n; i++) {
        c = a + b;
        a = b;
        b = c;
    }
    
    printf("Fibonacci(%d) = %lld\n", n, b);
    
    free(arg);
}

// Задача поиска простых чисел
void prime_search_task(void* arg) {
    int limit = *(int*)arg;
    int count = 0;
    
    printf("Поиск простых чисел до %d...\n", limit);
    
    for (int i = 2; i <= limit; i++) {
        int is_prime = 1;
        for (int j = 2; j * j <= i; j++) {
            if (i % j == 0) {
                is_prime = 0;
                break;
            }
        }
        if (is_prime) count++;
    }
    
    printf("Найдено %d простых чисел до %d\n", count, limit);
    
    free(arg);
}

// Обработчик ошибок для пула потоков
void custom_error_handler(const char* error_msg) {
    fprintf(stderr, "[CUSTOM ERROR] %s\n", error_msg);
}

// Пример использования пула потоков
int main() {
    srand(time(NULL));
    
    printf("=== Пример использования пула потоков ===\n\n");
    
    // Установка пользовательского обработчика ошибок
    thread_pool_set_error_handler(custom_error_handler);
    
    // Создание пула потоков
    printf("Создаем пул из 4 потоков...\n");
    thread_pool_t* pool = thread_pool_create(4);
    if (!pool) {
        printf("Не удалось создать пул потоков\n");
        return 1;
    }
    
    // Добавление 10 задач
    printf("\nДобавляем 10 задач в пул...\n");
    for (int i = 0; i < 10; i++) {
        task_data_t* data = malloc(sizeof(task_data_t));
        data->task_id = i + 1;
        data->duration_ms = 100 + (rand() % 400); // 100-500 мс
        data->message = malloc(50);
        snprintf(data->message, 50, "Сообщение от задачи %d", i + 1);
        
        if (thread_pool_add_task(pool, example_task, data) != 0) {
            printf("Не удалось добавить задачу %d\n", i + 1);
            free(data->message);
            free(data);
        }
    }
    
    // Добавление задач вычислений
    printf("\nДобавляем вычислительные задачи...\n");
    for (int i = 0; i < 5; i++) {
        int* n = malloc(sizeof(int));
        *n = 30 + rand() % 20; // 30-49
        
        if (thread_pool_add_task(pool, fibonacci_task, n) != 0) {
            free(n);
        }
    }
    
    // Добавление задач поиска простых чисел
    for (int i = 0; i < 3; i++) {
        int* limit = malloc(sizeof(int));
        *limit = 10000 + rand() % 50000; // 10000-59999
        
        if (thread_pool_add_task(pool, prime_search_task, limit) != 0) {
            free(limit);
        }
    }
    
    // Ожидание завершения всех задач
    printf("\nОжидаем завершения всех задач...\n");
    thread_pool_wait(pool);
    
    // Получение статистики
    thread_pool_stats_t stats = thread_pool_get_stats(pool);
    printf("\nСтатистика пула потоков:\n");
    printf("  Активные потоки: %d\n", stats.active_threads);
    printf("  Задач в очереди: %d\n", stats.queued_tasks);
    
    // Уничтожение пула
    printf("\nЗавершаем работу пула потоков...\n");
    thread_pool_destroy(pool);
    
    printf("\n=== Пример с расширенным пулом потоков ===\n\n");
    
    // Создание расширенного пула с динамическим масштабированием
    thread_pool_t* advanced_pool = thread_pool_create_advanced(2, 8, true);
    if (!advanced_pool) {
        printf("Не удалось создать расширенный пул потоков\n");
        return 1;
    }
    
    // Добавление большого количества задач
    printf("Добавляем 20 быстрых задач...\n");
    for (int i = 0; i < 20; i++) {
        task_data_t* data = malloc(sizeof(task_data_t));
        data->task_id = 100 + i;
        data->duration_ms = 50 + (rand() % 100); // 50-150 мс
        data->message = malloc(50);
        snprintf(data->message, 50, "Быстрая задача %d", 100 + i);
        
        thread_pool_add_task(advanced_pool, example_task, data);
    }
    
    // Ожидание
    printf("Ожидаем завершения...\n");
    thread_pool_wait(advanced_pool);
    
    // Уничтожение расширенного пула
    thread_pool_destroy(advanced_pool);
    
    printf("\nВсе примеры пула потоков завершены успешно!\n");
    return 0;
}
