#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEFAULT_REQUESTS 100UL
#define DEFAULT_WORKERS 4UL
#define DEFAULT_CAPACITY 16UL
#define DEFAULT_PRODUCERS 4UL
#define MAX_REQUESTS 1000000UL
#define MAX_THREADS 64UL
#define MAX_CAPACITY 100000UL
#define STOP_ID 0UL

typedef struct {
    unsigned long id;
    struct timespec enqueued_at;
} Request;

typedef struct {
    Request *items;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
    size_t max_depth;
    pthread_mutex_t mutex;
    sem_t free_slots;
    sem_t ready_items;
    bool initialized;
} RequestQueue;

typedef struct {
    unsigned long submitted;
    unsigned long processed;
    unsigned long successful;
    unsigned long rejected;
    double total_queue_wait_ms;
    double total_service_ms;
} Metrics;

typedef struct {
    RequestQueue *queue;
    Metrics *metrics;
    unsigned long first_id;
    unsigned long request_count;
} ProducerArgs;

typedef struct {
    RequestQueue *queue;
    Metrics *metrics;
    pthread_mutex_t *metrics_mutex;
} WorkerArgs;

static pthread_mutex_t metrics_mutex = PTHREAD_MUTEX_INITIALIZER;

static void report_pthread_error(const char *operation, int error_code)
{
    fprintf(stderr, "%s: %s\n", operation, strerror(error_code));
}

/*
 * Queue semaphore/mutex failures can leave queue and semaphore state
 * inconsistent.  There is no safe general rollback once another thread may
 * have observed the state, so the engine uses a fail-stop policy: report and
 * terminate the process rather than let producers/workers hang or continue
 * with corrupt synchronization state.  This is an unrecoverable OS-level
 * synchronization failure; normal pthread_create failures are handled by
 * stopping and joining the threads that were actually created.
 */
static _Noreturn void fatal_sync_error(const char *operation, int error_code)
{
    report_pthread_error(operation, error_code);
    fflush(stderr);
    _Exit(EXIT_FAILURE);
}

static struct timespec monotonic_now(void)
{
    struct timespec result;

    if (clock_gettime(CLOCK_MONOTONIC, &result) != 0) {
        perror("clock_gettime");
        fflush(stderr);
        _Exit(EXIT_FAILURE);
    }
    return result;
}

static double elapsed_ms(struct timespec start, struct timespec end)
{
    double seconds = (double)(end.tv_sec - start.tv_sec);
    double nanoseconds = (double)(end.tv_nsec - start.tv_nsec);
    return seconds * 1000.0 + nanoseconds / 1000000.0;
}

static int semaphore_wait(sem_t *semaphore)
{
    while (sem_wait(semaphore) == -1) {
        if (errno != EINTR) {
            return errno;
        }
    }
    return 0;
}

static int queue_init(RequestQueue *queue, size_t capacity)
{
    memset(queue, 0, sizeof(*queue));
    queue->items = calloc(capacity, sizeof(*queue->items));
    if (queue->items == NULL) {
        return ENOMEM;
    }
    queue->capacity = capacity;

    int result = pthread_mutex_init(&queue->mutex, NULL);
    if (result != 0) {
        free(queue->items);
        queue->items = NULL;
        return result;
    }

    if (sem_init(&queue->free_slots, 0, (unsigned int)capacity) != 0) {
        result = errno;
        pthread_mutex_destroy(&queue->mutex);
        free(queue->items);
        queue->items = NULL;
        return result;
    }

    if (sem_init(&queue->ready_items, 0, 0) != 0) {
        result = errno;
        sem_destroy(&queue->free_slots);
        pthread_mutex_destroy(&queue->mutex);
        free(queue->items);
        queue->items = NULL;
        return result;
    }

    queue->initialized = true;
    return 0;
}

static int queue_push(RequestQueue *queue, Request request)
{
    int result = semaphore_wait(&queue->free_slots);
    if (result != 0) {
        return result;
    }

    /* Start queue-wait measurement only after a producer owns a queue slot. */
    request.enqueued_at = monotonic_now();

    result = pthread_mutex_lock(&queue->mutex);
    if (result != 0) {
        if (sem_post(&queue->free_slots) != 0) {
            fatal_sync_error("restore queue free slot", errno);
        }
        return result;
    }

    queue->items[queue->tail] = request;
    queue->tail = (queue->tail + 1U) % queue->capacity;
    queue->count++;
    /* Shutdown markers occupy slots but are not simulated requests. */
    if (request.id != STOP_ID && queue->count > queue->max_depth) {
        queue->max_depth = queue->count;
    }

    result = pthread_mutex_unlock(&queue->mutex);
    if (result != 0) {
        fatal_sync_error("unlock queue after push", result);
    }
    if (sem_post(&queue->ready_items) != 0) {
        fatal_sync_error("post ready item after push", errno);
    }
    return 0;
}

static int queue_pop(RequestQueue *queue, Request *request)
{
    int result = semaphore_wait(&queue->ready_items);
    if (result != 0) {
        return result;
    }

    result = pthread_mutex_lock(&queue->mutex);
    if (result != 0) {
        if (sem_post(&queue->ready_items) != 0) {
            fatal_sync_error("restore queue ready item", errno);
        }
        return result;
    }

    *request = queue->items[queue->head];
    queue->head = (queue->head + 1U) % queue->capacity;
    queue->count--;

    result = pthread_mutex_unlock(&queue->mutex);
    if (result != 0) {
        fatal_sync_error("unlock queue after pop", result);
    }
    if (sem_post(&queue->free_slots) != 0) {
        fatal_sync_error("post free slot after pop", errno);
    }
    return 0;
}

static void queue_destroy(RequestQueue *queue)
{
    if (!queue->initialized) {
        return;
    }
    sem_destroy(&queue->ready_items);
    sem_destroy(&queue->free_slots);
    pthread_mutex_destroy(&queue->mutex);
    free(queue->items);
    queue->items = NULL;
    queue->initialized = false;
}

static void *producer_main(void *argument)
{
    ProducerArgs *args = argument;
    for (unsigned long i = 0; i < args->request_count; i++) {
        Request request = {.id = args->first_id + i};
        int result = queue_push(args->queue, request);
        if (result != 0) {
            fatal_sync_error("producer queue push", result);
        }

        result = pthread_mutex_lock(&metrics_mutex);
        if (result != 0) {
            fatal_sync_error("metrics mutex lock", result);
        }
        /* Submitted is counted once the request is actually in the queue. */
        args->metrics->submitted++;
        result = pthread_mutex_unlock(&metrics_mutex);
        if (result != 0) {
            fatal_sync_error("metrics mutex unlock", result);
        }
    }
    return NULL;
}

static void *worker_main(void *argument)
{
    WorkerArgs *args = argument;
    for (;;) {
        Request request;
        int result = queue_pop(args->queue, &request);
        if (result != 0) {
            fatal_sync_error("worker queue pop", result);
        }
        if (request.id == STOP_ID) {
            return NULL;
        }

        struct timespec service_started = monotonic_now();
        double queue_wait_ms = elapsed_ms(request.enqueued_at, service_started);

        /* Simulate service work; this is not a database or backend request. */
        unsigned int delay_ms = 10U + (unsigned int)(request.id % 21UL);
        struct timespec remaining = {
            .tv_sec = (time_t)(delay_ms / 1000U),
            .tv_nsec = (long)(delay_ms % 1000U) * 1000000L
        };
        while (nanosleep(&remaining, &remaining) == -1) {
            if (errno != EINTR) {
                fatal_sync_error("nanosleep", errno);
            }
        }

        struct timespec service_finished = monotonic_now();
        double service_ms = elapsed_ms(service_started, service_finished);
        bool success = (request.id % 5UL) != 0UL;

        result = pthread_mutex_lock(args->metrics_mutex);
        if (result != 0) {
            fatal_sync_error("metrics mutex lock", result);
        }
        args->metrics->processed++;
        if (success) {
            args->metrics->successful++;
        } else {
            args->metrics->rejected++;
        }
        args->metrics->total_queue_wait_ms += queue_wait_ms;
        args->metrics->total_service_ms += service_ms;
        result = pthread_mutex_unlock(args->metrics_mutex);
        if (result != 0) {
            fatal_sync_error("metrics mutex unlock", result);
        }
    }
}

static unsigned long parse_unsigned(const char *text, const char *name,
                                    unsigned long minimum, unsigned long maximum)
{
    if (text == NULL || *text == '\0') {
        fprintf(stderr, "Invalid %s: expected a number from %lu to %lu.\n",
                name, minimum, maximum);
        exit(EXIT_FAILURE);
    }
    for (const char *character = text; *character != '\0'; character++) {
        if (*character < '0' || *character > '9') {
            fprintf(stderr, "Invalid %s: expected a number from %lu to %lu.\n",
                    name, minimum, maximum);
            exit(EXIT_FAILURE);
        }
    }

    errno = 0;
    char *end = NULL;
    unsigned long value = strtoul(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' ||
        value < minimum || value > maximum) {
        fprintf(stderr, "Invalid %s: expected a number from %lu to %lu.\n",
                name, minimum, maximum);
        exit(EXIT_FAILURE);
    }
    return value;
}

static void print_usage(const char *program)
{
    fprintf(stderr,
            "Usage: %s [requests] [workers] [queue_capacity] [producers]\n"
            "Example: %s 1000 4 16 4\n",
            program, program);
}

static bool join_threads(pthread_t *threads, unsigned long count, const char *label)
{
    bool succeeded = true;
    for (unsigned long i = 0; i < count; i++) {
        void *thread_result = NULL;
        int result = pthread_join(threads[i], &thread_result);
        if (result != 0) {
            fatal_sync_error(label, result);
        } else if (thread_result != NULL) {
            fprintf(stderr, "%s exited after a synchronization error.\n", label);
            succeeded = false;
        }
    }
    return succeeded;
}

static bool stop_workers(RequestQueue *queue, pthread_t *workers, unsigned long count)
{
    bool succeeded = true;
    for (unsigned long i = 0; i < count; i++) {
        Request stop = {.id = STOP_ID};
        int result = queue_push(queue, stop);
        if (result != 0) {
            /* Continuing to join would hang workers that missed a marker. */
            fatal_sync_error("enqueue worker stop marker", result);
        }
    }
    return join_threads(workers, count, "pthread_join worker") && succeeded;
}

static void print_metrics(const Metrics *metrics, size_t max_depth, size_t capacity,
                          double elapsed)
{
    printf("\n========== Simulation Metrics ==========\n");
    printf("Requests submitted : %lu\n", metrics->submitted);
    printf("Requests processed : %lu\n", metrics->processed);
    printf("Simulated success  : %lu\n", metrics->successful);
    printf("Simulated rejected : %lu\n", metrics->rejected);
    printf("Maximum queue depth: %zu / %zu\n", max_depth, capacity);
    if (metrics->processed != 0UL) {
        printf("Average queue wait : %.3f ms\n",
               metrics->total_queue_wait_ms / (double)metrics->processed);
        printf("Average service time: %.3f ms\n",
               metrics->total_service_ms / (double)metrics->processed);
    } else {
        printf("Average queue wait : N/A\nAverage service time: N/A\n");
    }
    printf("Elapsed time       : %.3f ms (producer start through worker shutdown)\n",
           elapsed);
    if (elapsed > 0.0) {
        printf("End-to-end simulated throughput: %.2f requests/sec\n",
               (double)metrics->processed * 1000.0 / elapsed);
    } else {
        printf("End-to-end simulated throughput: N/A\n");
    }
    printf("These are simulated OS-engine outcomes and timings, not real purchases,\n");
    printf("PostgreSQL transactions, backend latency, or database throughput.\n");
    printf("=========================================\n");
}

int main(int argc, char **argv)
{
    unsigned long request_count = DEFAULT_REQUESTS;
    unsigned long worker_count = DEFAULT_WORKERS;
    unsigned long capacity = DEFAULT_CAPACITY;
    unsigned long requested_producers = DEFAULT_PRODUCERS;

    if (argc > 5) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    if (argc >= 2) request_count = parse_unsigned(argv[1], "requests", 1UL, MAX_REQUESTS);
    if (argc >= 3) worker_count = parse_unsigned(argv[2], "workers", 1UL, MAX_THREADS);
    if (argc >= 4) capacity = parse_unsigned(argv[3], "queue_capacity", 1UL, MAX_CAPACITY);
    if (argc >= 5) requested_producers = parse_unsigned(argv[4], "producers", 1UL, MAX_THREADS);

    unsigned long producer_count = requested_producers;
    if (producer_count > request_count) {
        producer_count = request_count;
    }

    printf("FlashLock OS Engine — producer/consumer simulation\n");
    printf("Requests: %lu | Producers: %lu | Workers: %lu | Queue capacity: %lu\n",
           request_count, producer_count, worker_count, capacity);
    printf("Simulated outcomes are not PostgreSQL transactions or backend results.\n");

    RequestQueue queue;
    int result = queue_init(&queue, (size_t)capacity);
    if (result != 0) {
        report_pthread_error("initialize bounded queue", result);
        return EXIT_FAILURE;
    }

    pthread_t *workers = calloc(worker_count, sizeof(*workers));
    if (workers == NULL) {
        perror("allocate worker handles");
        queue_destroy(&queue);
        return EXIT_FAILURE;
    }

    Metrics metrics = {0};
    WorkerArgs worker_args = {
        .queue = &queue,
        .metrics = &metrics,
        .metrics_mutex = &metrics_mutex
    };
    unsigned long workers_created = 0UL;
    for (; workers_created < worker_count; workers_created++) {
        result = pthread_create(&workers[workers_created], NULL, worker_main, &worker_args);
        if (result != 0) {
            report_pthread_error("pthread_create worker", result);
            break;
        }
    }
    if (workers_created != worker_count) {
        bool workers_stopped = stop_workers(&queue, workers, workers_created);
        if (!workers_stopped) {
            fprintf(stderr, "failed to shut down the partially created worker pool\n");
        }
        free(workers);
        queue_destroy(&queue);
        pthread_mutex_destroy(&metrics_mutex);
        return EXIT_FAILURE;
    }

    pthread_t *producers = calloc(producer_count, sizeof(*producers));
    ProducerArgs *producer_args = calloc(producer_count, sizeof(*producer_args));
    if (producers == NULL || producer_args == NULL) {
        perror("allocate producer data");
        free(producers);
        free(producer_args);
        bool workers_stopped = stop_workers(&queue, workers, workers_created);
        if (!workers_stopped) {
            fprintf(stderr, "failed to shut down the worker pool after producer allocation failure\n");
        }
        free(workers);
        queue_destroy(&queue);
        pthread_mutex_destroy(&metrics_mutex);
        return EXIT_FAILURE;
    }

    unsigned long base = request_count / producer_count;
    unsigned long remainder = request_count % producer_count;
    unsigned long next_id = 1UL;
    unsigned long producers_created = 0UL;
    struct timespec simulation_started = monotonic_now();
    for (; producers_created < producer_count; producers_created++) {
        unsigned long assigned = base + (producers_created < remainder ? 1UL : 0UL);
        producer_args[producers_created] = (ProducerArgs) {
            .queue = &queue,
            .metrics = &metrics,
            .first_id = next_id,
            .request_count = assigned
        };
        next_id += assigned;

        result = pthread_create(&producers[producers_created], NULL, producer_main,
                                &producer_args[producers_created]);
        if (result != 0) {
            report_pthread_error("pthread_create producer", result);
            break;
        }
    }

    bool succeeded = join_threads(producers, producers_created, "pthread_join producer");
    if (producers_created != producer_count) {
        succeeded = false;
    }
    succeeded = stop_workers(&queue, workers, workers_created) && succeeded;
    struct timespec simulation_finished = monotonic_now();
    double elapsed = elapsed_ms(simulation_started, simulation_finished);

    int lock_result = pthread_mutex_lock(&queue.mutex);
    size_t max_depth = 0U;
    if (lock_result == 0) {
        max_depth = queue.max_depth;
        pthread_mutex_unlock(&queue.mutex);
    } else {
        report_pthread_error("read queue metrics", lock_result);
        succeeded = false;
    }
    print_metrics(&metrics, max_depth, queue.capacity, elapsed);

    succeeded = succeeded && metrics.submitted == request_count &&
                metrics.processed == request_count &&
                metrics.successful + metrics.rejected == metrics.processed;

    free(producer_args);
    free(producers);
    free(workers);
    queue_destroy(&queue);
    pthread_mutex_destroy(&metrics_mutex);
    return succeeded ? EXIT_SUCCESS : EXIT_FAILURE;
}
