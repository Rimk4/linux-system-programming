linux-system-programming/
├── multithreading/
│   ├── thread_creation.c         # Базовое создание потоков (pthread)
│   ├── mutex_example.c           # Синхронизация с мьютексами
│   ├── condition_variables.c     # Условные переменные
│   ├── thread_pool/              # Пул потоков
│   │   ├── thread_pool.c
│   │   └── thread_pool.h
│   └── producer_consumer.c       # Задача производитель-потребитель
├── ipc/
│   ├── pipes/
│   │   ├── unnamed_pipe.c        # Неименованные каналы
│   │   └── named_pipe_client.c   # Именованные каналы (клиент)
│   │   └── named_pipe_server.c   # Именованные каналы (сервер)
│   ├── shared_memory/
│   │   ├── shm_writer.c          # Разделяемая память (запись)
│   │   └── shm_reader.c          # Разделяемая память (чтение)
│   ├── message_queues/
│   │   ├── mq_sender.c           # Очереди сообщений POSIX
│   │   └── mq_receiver.c
│   ├── semaphores/
│   │   ├── sem_producer.c        # Семафоры POSIX
│   │   └── sem_consumer.c
│   └── sockets/
│       ├── unix_socket_server.c  # UNIX сокеты
│       └── unix_socket_client.c
├── daemons/
│   ├── simple_daemon.c           # Простой демон
│   ├── syslog_daemon.c           # Демон с логированием в syslog
│   └── daemon_with_config.c      # Демон с конфигурационным файлом
├── signals/
│   ├── signal_handler.c          # Обработка сигналов
│   ├── sigaction_example.c       # Использование sigaction
│   └── realtime_signals.c        # Сигналы реального времени
├── process_management/
│   ├── fork_exec.c               # fork() и exec()
│   ├── zombie_process.c          # Демонстрация зомби-процессов
│   └── process_groups.c          # Группы процессов и сессии
└── examples/
    ├── webserver_threaded.c      # Многопоточный веб-сервер
    ├── chat_server/              # Сервер чата с IPC
    │   ├── server.c
    │   └── client.c
    └── monitoring_daemon.c       # Демон мониторинга системы
