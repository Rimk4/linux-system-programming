#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 5
#define NUM_ITERATIONS 100000

// Глобальная переменная для демонстрации состояния гонки
int counter = 0;

// Мьютекс для синхронизации доступа к counter
pthread_mutex_t mutex_counter = PTHREAD_MUTEX_INITIALIZER;

// Пример БЕЗ мьютекса (состояние гонки)
void* increment_without_mutex(void* arg) {
    int thread_id = *(int*)arg;
    
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        counter++; // Критическая секция БЕЗ защиты
    }
    
    printf("Поток %d завершился (без мьютекса). Локальное значение: %d\n", 
           thread_id, NUM_ITERATIONS);
    return NULL;
}

// Пример С мьютексом (правильная синхронизация)
void* increment_with_mutex(void* arg) {
    int thread_id = *(int*)arg;
    
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        pthread_mutex_lock(&mutex_counter);  // Блокировка
        counter++; // Критическая секция под защитой
        pthread_mutex_unlock(&mutex_counter); // Разблокировка
    }
    
    printf("Поток %d завершился (с мьютексом). Локальное значение: %d\n", 
           thread_id, NUM_ITERATIONS);
    return NULL;
}

// Пример с trylock (попытка блокировки без ожидания)
void* increment_with_trylock(void* arg) {
    int thread_id = *(int*)arg;
    int successful_locks = 0;
    int failed_locks = 0;
    
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        if (pthread_mutex_trylock(&mutex_counter) == 0) {
            // Успешная блокировка
            counter++;
            pthread_mutex_unlock(&mutex_counter);
            successful_locks++;
        } else {
            // Мьютекс уже заблокирован другим потоком
            failed_locks++;
            usleep(10); // Небольшая задержка перед повторной попыткой
        }
    }
    
    printf("Поток %d: успешные блокировки: %d, неудачи: %d\n",
           thread_id, successful_locks, failed_locks);
    return NULL;
}

// Пример с рекурсивным мьютексом
pthread_mutex_t recursive_mutex;
int recursion_depth = 0;

void recursive_function(int thread_id, int depth) {
    if (depth <= 0) return;
    
    pthread_mutex_lock(&recursive_mutex);
    recursion_depth++;
    
    printf("Поток %d: глубина рекурсии %d, счетчик глубины: %d\n",
           thread_id, depth, recursion_depth);
    
    // Рекурсивный вызов под защитой того же мьютекса
    recursive_function(thread_id, depth - 1);
    
    recursion_depth--;
    pthread_mutex_unlock(&recursive_mutex);
}

void* test_recursive_mutex(void* arg) {
    int thread_id = *(int*)arg;
    recursive_function(thread_id, 5);
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    
    printf("=== Пример 1: Состояние гонки (без мьютекса) ===\n");
    counter = 0;
    
    // Запуск потоков БЕЗ мьютекса
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i + 1;
        pthread_create(&threads[i], NULL, increment_without_mutex, &thread_ids[i]);
    }
    
    // Ожидание завершения потоков
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("Ожидаемое значение счетчика: %d\n", NUM_THREADS * NUM_ITERATIONS);
    printf("Фактическое значение счетчика: %d\n", counter);
    printf("Разница (потерянные инкременты): %d\n\n",
           NUM_THREADS * NUM_ITERATIONS - counter);
    
    printf("=== Пример 2: Правильная синхронизация (с мьютексом) ===\n");
    counter = 0;
    
    // Запуск потоков С мьютексом
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i + 1;
        pthread_create(&threads[i], NULL, increment_with_mutex, &thread_ids[i]);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("Ожидаемое значение счетчика: %d\n", NUM_THREADS * NUM_ITERATIONS);
    printf("Фактическое значение счетчика: %d\n", counter);
    printf("Разница: %d\n\n", NUM_THREADS * NUM_ITERATIONS - counter);
    
    printf("=== Пример 3: Использование trylock ===\n");
    counter = 0;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i + 1;
        pthread_create(&threads[i], NULL, increment_with_trylock, &thread_ids[i]);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("Финальное значение счетчика: %d\n\n", counter);
    
    printf("=== Пример 4: Рекурсивный мьютекс ===\n");
    
    // Инициализация рекурсивного мьютекса
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&recursive_mutex, &attr);
    
    for (int i = 0; i < 2; i++) {
        thread_ids[i] = i + 1;
        pthread_create(&threads[i], NULL, test_recursive_mutex, &thread_ids[i]);
    }
    
    for (int i = 0; i < 2; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Очистка ресурсов
    pthread_mutex_destroy(&mutex_counter);
    pthread_mutex_destroy(&recursive_mutex);
    pthread_mutexattr_destroy(&attr);
    
    printf("\nВсе примеры завершены успешно!\n");
    return 0;
}
