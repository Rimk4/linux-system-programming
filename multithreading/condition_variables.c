#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#define BUFFER_SIZE 5
#define NUM_PRODUCERS 2
#define NUM_CONSUMERS 3
#define ITEMS_PER_PRODUCER 10

// Структура кольцевого буфера
typedef struct {
    int buffer[BUFFER_SIZE];
    int count;
    int in;
    int out;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} RingBuffer;

RingBuffer ring_buffer;

// Инициализация буфера
void buffer_init(RingBuffer* rb) {
    rb->count = 0;
    rb->in = 0;
    rb->out = 0;
    pthread_mutex_init(&rb->mutex, NULL);
    pthread_cond_init(&rb->not_empty, NULL);
    pthread_cond_init(&rb->not_full, NULL);
}

// Очистка буфера
void buffer_destroy(RingBuffer* rb) {
    pthread_mutex_destroy(&rb->mutex);
    pthread_cond_destroy(&rb->not_empty);
    pthread_cond_destroy(&rb->not_full);
}

// Добавление элемента в буфер
void buffer_put(RingBuffer* rb, int value) {
    pthread_mutex_lock(&rb->mutex);
    
    // Ожидаем, пока буфер не освободится
    while (rb->count == BUFFER_SIZE) {
        printf("Буфер полон. Производитель ждет...\n");
        pthread_cond_wait(&rb->not_full, &rb->mutex);
    }
    
    // Добавляем элемент
    rb->buffer[rb->in] = value;
    rb->in = (rb->in + 1) % BUFFER_SIZE;
    rb->count++;
    
    printf("Производитель добавил: %d (всего элементов: %d)\n", value, rb->count);
    
    // Сигнализируем потребителям, что буфер не пуст
    pthread_cond_signal(&rb->not_empty);
    pthread_mutex_unlock(&rb->mutex);
}

// Получение элемента из буфера
int buffer_get(RingBuffer* rb) {
    pthread_mutex_lock(&rb->mutex);
    
    // Ожидаем, пока в буфере не появится элемент
    while (rb->count == 0) {
        printf("Буфер пуст. Потребитель ждет...\n");
        pthread_cond_wait(&rb->not_empty, &rb->mutex);
    }
    
    // Извлекаем элемент
    int value = rb->buffer[rb->out];
    rb->out = (rb->out + 1) % BUFFER_SIZE;
    rb->count--;
    
    printf("Потребитель извлек: %d (осталось элементов: %d)\n", value, rb->count);
    
    // Сигнализируем производителям, что есть место
    pthread_cond_signal(&rb->not_full);
    pthread_mutex_unlock(&rb->mutex);
    
    return value;
}

// Функция производителя
void* producer(void* arg) {
    int producer_id = *(int*)arg;
    
    for (int i = 0; i < ITEMS_PER_PRODUCER; i++) {
        int item = producer_id * 100 + i; // Генерируем уникальный элемент
        printf("Производитель %d генерирует элемент: %d\n", producer_id, item);
        
        buffer_put(&ring_buffer, item);
        
        // Имитация работы
        usleep(rand() % 100000); // 0-100 мс
    }
    
    printf("Производитель %d завершил работу\n", producer_id);
    return NULL;
}

// Функция потребителя
void* consumer(void* arg) {
    int consumer_id = *(int*)arg;
    int total_consumed = 0;
    
    while (total_consumed < (NUM_PRODUCERS * ITEMS_PER_PRODUCER) / NUM_CONSUMERS + 1) {
        int item = buffer_get(&ring_buffer);
        printf("Потребитель %d обрабатывает элемент: %d\n", consumer_id, item);
        
        total_consumed++;
        
        // Имитация обработки
        usleep(rand() % 150000); // 0-150 мс
        
        if (total_consumed % 5 == 0) {
            printf("Потребитель %d обработал уже %d элементов\n", 
                   consumer_id, total_consumed);
        }
    }
    
    printf("Потребитель %d завершил работу. Всего обработано: %d элементов\n",
           consumer_id, total_consumed);
    return NULL;
}

// Пример с барьером (точка синхронизации потоков)
pthread_barrier_t barrier;
void* barrier_example(void* arg) {
    int thread_id = *(int*)arg;
    
    printf("Поток %d: фаза 1\n", thread_id);
    sleep(1);
    
    // Ожидание всех потоков
    printf("Поток %d: ожидание у барьера...\n", thread_id);
    pthread_barrier_wait(&barrier);
    
    printf("Поток %d: фаза 2 (все потоки достигли барьера)\n", thread_id);
    sleep(1);
    
    return NULL;
}

// Пример с условной переменной и предикатом
typedef struct {
    int value;
    bool ready;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} SharedData;

SharedData shared_data;

void* setter_thread(void* arg) {
    sleep(2); // Имитация работы
    
    pthread_mutex_lock(&shared_data.mutex);
    shared_data.value = 42;
    shared_data.ready = true;
    printf("Setter: значение установлено в %d\n", shared_data.value);
    
    // Сигнал всем ожидающим потокам
    pthread_cond_broadcast(&shared_data.cond);
    pthread_mutex_unlock(&shared_data.mutex);
    
    return NULL;
}

void* getter_thread(void* arg) {
    int thread_id = *(int*)arg;
    
    pthread_mutex_lock(&shared_data.mutex);
    
    // Ожидание с проверкой условия (избегаем ложных пробуждений)
    while (!shared_data.ready) {
        printf("Getter %d: ожидание данных...\n", thread_id);
        pthread_cond_wait(&shared_data.cond, &shared_data.mutex);
    }
    
    printf("Getter %d: получил значение %d\n", thread_id, shared_data.value);
    pthread_mutex_unlock(&shared_data.mutex);
    
    return NULL;
}

int main() {
    pthread_t producers[NUM_PRODUCERS];
    pthread_t consumers[NUM_CONSUMERS];
    int producer_ids[NUM_PRODUCERS];
    int consumer_ids[NUM_CONSUMERS];
    
    srand(time(NULL));
    
    printf("=== Пример 1: Производитель-Потребитель с условными переменными ===\n");
    
    // Инициализация буфера
    buffer_init(&ring_buffer);
    
    // Создание производителей
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        producer_ids[i] = i + 1;
        pthread_create(&producers[i], NULL, producer, &producer_ids[i]);
    }
    
    // Создание потребителей
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        consumer_ids[i] = i + 1;
        pthread_create(&consumers[i], NULL, consumer, &consumer_ids[i]);
    }
    
    // Ожидание завершения производителей
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        pthread_join(producers[i], NULL);
    }
    
    // Ожидание завершения потребителей
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        pthread_join(consumers[i], NULL);
    }
    
    // Очистка буфера
    buffer_destroy(&ring_buffer);
    
    printf("\n=== Пример 2: Барьер для синхронизации потоков ===\n");
    
    // Инициализация барьера на 3 потока
    pthread_barrier_init(&barrier, NULL, 3);
    
    pthread_t barrier_threads[3];
    int barrier_ids[3];
    
    for (int i = 0; i < 3; i++) {
        barrier_ids[i] = i + 1;
        pthread_create(&barrier_threads[i], NULL, barrier_example, &barrier_ids[i]);
    }
    
    for (int i = 0; i < 3; i++) {
        pthread_join(barrier_threads[i], NULL);
    }
    
    pthread_barrier_destroy(&barrier);
    
    printf("\n=== Пример 3: Условные переменные с предикатом ===\n");
    
    // Инициализация разделяемых данных
    shared_data.value = 0;
    shared_data.ready = false;
    pthread_mutex_init(&shared_data.mutex, NULL);
    pthread_cond_init(&shared_data.cond, NULL);
    
    pthread_t setter;
    pthread_t getters[3];
    int getter_ids[3];
    
    // Запуск setter потока
    pthread_create(&setter, NULL, setter_thread, NULL);
    
    // Запуск getter потоков
    for (int i = 0; i < 3; i++) {
        getter_ids[i] = i + 1;
        pthread_create(&getters[i], NULL, getter_thread, &getter_ids[i]);
    }
    
    // Ожидание завершения
    pthread_join(setter, NULL);
    for (int i = 0; i < 3; i++) {
        pthread_join(getters[i], NULL);
    }
    
    // Очистка ресурсов
    pthread_mutex_destroy(&shared_data.mutex);
    pthread_cond_destroy(&shared_data.cond);
    
    printf("\nВсе примеры с условными переменными завершены!\n");
    return 0;
}
