#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>

void daemonize() {
    pid_t pid;
    
    // 1. Создание дочернего процесса
    pid = fork();
    
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    
    if (pid > 0) {
        exit(EXIT_SUCCESS); // Завершение родительского процесса
    }
    
    // 2. Создание новой сессии
    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }
    
    // 3. Установка маски прав доступа к файлам
    umask(0);
    
    // 4. Изменение рабочего каталога
    chdir("/");
    
    // 5. Закрытие стандартных дескрипторов
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    // Перенаправление в /dev/null
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_RDWR);
}

void signal_handler(int sig) {
    if (sig == SIGTERM) {
        // Логирование завершения
        int log_fd = open("/var/log/mydaemon.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
        char buffer[256];
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        strftime(buffer, 256, "[%Y-%m-%d %H:%M:%S] ", tm_info);
        write(log_fd, buffer, strlen(buffer));
        write(log_fd, "Демон завершает работу\n", 23);
        close(log_fd);
        
        exit(EXIT_SUCCESS);
    }
}

int main() {
    daemonize();
    
    // Установка обработчика сигналов
    signal(SIGTERM, signal_handler);
    
    // Основной цикл демона
    while (1) {
        // Открытие лог-файла
        int log_fd = open("/var/log/mydaemon.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
        
        if (log_fd >= 0) {
            char buffer[256];
            time_t now = time(NULL);
            struct tm *tm_info = localtime(&now);
            strftime(buffer, 256, "[%Y-%m-%d %H:%M:%S] ", tm_info);
            write(log_fd, buffer, strlen(buffer));
            write(log_fd, "Демон работает\n", 15);
            close(log_fd);
        }
        
        sleep(60); // Пауза 60 секунд
    }
    
    return 0;
}
