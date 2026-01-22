#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

int main() {
    const char* name = "/my_shared_memory";
    const int SIZE = 4096;
    
    // Создание сегмента разделяемой памяти
    int shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, SIZE);
    
    // Отображение в адресное пространство процесса
    char* ptr = mmap(0, SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0);
    
    // Запись данных
    sprintf(ptr, "Привет из процесса-писателя! PID: %d", getpid());
    
    printf("Данные записаны в разделяемую память\n");
    
    // Пауза для чтения другим процессом
    sleep(10);
    
    // Удаление разделяемой памяти
    shm_unlink(name);
    
    return 0;
}
