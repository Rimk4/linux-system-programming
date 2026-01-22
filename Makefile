CC = gcc
CFLAGS = -Wall -Wextra -pthread
LDFLAGS = -lrt -lpthread

# Многопоточные примеры
THREAD_EXAMPLES = multithreading/thread_creation multithreading/mutex_example \
                  multithreading/condition_variables multithreading/producer_consumer

# IPC примеры
IPC_EXAMPLES = ipc/pipes/unnamed_pipe ipc/shared_memory/shm_writer \
               ipc/shared_memory/shm_reader ipc/message_queues/mq_sender \
               ipc/message_queues/mq_receiver

# Демоны
DAEMON_EXAMPLES = daemons/simple_daemon daemons/syslog_daemon

# Все примеры
EXAMPLES = $(THREAD_EXAMPLES) $(IPC_EXAMPLES) $(DAEMON_EXAMPLES)

all: $(EXAMPLES)

# Общее правило для сборки
%: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(EXAMPLES)
	rm -f /dev/shm/my_shared_memory
	rm -f /tmp/my_named_pipe
	rm -f /var/log/mydaemon.log

.PHONY: all clean
