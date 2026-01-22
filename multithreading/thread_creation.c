#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void* thread_function(void* arg) {
    int thread_num = *(int*)arg;
    printf("Поток %d запущен\n", thread_num);
    
    // Имитация работы
    for (int i = 0; i < 3; i++) {
        printf("Поток %d: шаг %d\n", thread_num, i);
        sleep(1);
    }
    
    printf("Поток %d завершен\n", thread_num);
    pthread_exit(NULL);
}

int main() {
    pthread_t threads[3];
    int thread_args[3];
    
    printf("Основной поток: создаю рабочие потоки\n");
    
    for (int i = 0; i < 3; i++) {
        thread_args[i] = i + 1;
        if (pthread_create(&threads[i], NULL, thread_function, &thread_args[i])) {
            fprintf(stderr, "Ошибка создания потока\n");
            return 1;
        }
    }
    
    // Ожидание завершения потоков
    for (int i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("Все потоки завершены\n");
    return 0;
}
