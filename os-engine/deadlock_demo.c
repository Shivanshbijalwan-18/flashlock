#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define GATE_TIMEOUT_SECONDS 5

typedef struct {
    pthread_mutex_t *first;
    pthread_mutex_t *second;
    pthread_mutex_t *gate_mutex;
    pthread_cond_t *gate_condition;
    unsigned int *ready_count;
    atomic_bool *release_workers;
    atomic_bool *abort_demo;
    atomic_uint *blocked_attempts;
} DemoWorker;

static void *demonstrate_lock_order(void *argument)
{
    DemoWorker *worker = argument;
    int result = pthread_mutex_lock(worker->first);
    if (result != 0) {
        fprintf(stderr, "lock first mutex: %s\n", strerror(result));
        return worker;
    }

    result = pthread_mutex_lock(worker->gate_mutex);
    if (result != 0) {
        fprintf(stderr, "lock demo gate: %s\n", strerror(result));
        pthread_mutex_unlock(worker->first);
        return worker;
    }
    (*worker->ready_count)++;
    pthread_cond_broadcast(worker->gate_condition);
    struct timespec deadline;
    if (clock_gettime(CLOCK_REALTIME, &deadline) != 0) {
        perror("clock_gettime");
        atomic_store(worker->abort_demo, true);
        atomic_store(worker->release_workers, true);
        pthread_cond_broadcast(worker->gate_condition);
        pthread_mutex_unlock(worker->gate_mutex);
        pthread_mutex_unlock(worker->first);
        return worker;
    }
    deadline.tv_sec += GATE_TIMEOUT_SECONDS;
    while (!atomic_load(worker->release_workers)) {
        result = pthread_cond_timedwait(worker->gate_condition, worker->gate_mutex,
                                        &deadline);
        if (result == ETIMEDOUT) {
            if (!atomic_load(worker->release_workers)) {
                fprintf(stderr, "timed out waiting for demo gate release\n");
                atomic_store(worker->abort_demo, true);
                atomic_store(worker->release_workers, true);
                pthread_cond_broadcast(worker->gate_condition);
            }
            break;
        }
        if (result != 0) {
            fprintf(stderr, "wait on demo gate: %s\n", strerror(result));
            atomic_store(worker->abort_demo, true);
            atomic_store(worker->release_workers, true);
            pthread_cond_broadcast(worker->gate_condition);
            break;
        }
    }
    bool abort_demo = atomic_load(worker->abort_demo);
    pthread_mutex_unlock(worker->gate_mutex);
    if (abort_demo) {
        pthread_mutex_unlock(worker->first);
        return NULL;
    }

    result = pthread_mutex_trylock(worker->second);
    if (result == EBUSY) {
        (void)atomic_fetch_add(worker->blocked_attempts, 1U);
        pthread_mutex_unlock(worker->first);
        return NULL;
    }
    if (result != 0) {
        fprintf(stderr, "trylock second mutex: %s\n", strerror(result));
        pthread_mutex_unlock(worker->first);
        return worker;
    }

    pthread_mutex_unlock(worker->second);
    pthread_mutex_unlock(worker->first);
    return NULL;
}

int main(void)
{
    pthread_mutex_t mutex_a = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t mutex_b = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t gate_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t gate_condition = PTHREAD_COND_INITIALIZER;
    unsigned int ready_count = 0U;
    atomic_bool release_workers = ATOMIC_VAR_INIT(false);
    atomic_bool abort_demo = ATOMIC_VAR_INIT(false);
    atomic_uint blocked_attempts = ATOMIC_VAR_INIT(0U);

    DemoWorker workers[2] = {
        {&mutex_a, &mutex_b, &gate_mutex, &gate_condition, &ready_count,
         &release_workers, &abort_demo, &blocked_attempts},
        {&mutex_b, &mutex_a, &gate_mutex, &gate_condition, &ready_count,
         &release_workers, &abort_demo, &blocked_attempts}
    };
    pthread_t threads[2];
    unsigned int created = 0U;
    for (; created < 2U; created++) {
        int result = pthread_create(&threads[created], NULL,
                                    demonstrate_lock_order, &workers[created]);
        if (result != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(result));
            break;
        }
    }

    int result = pthread_mutex_lock(&gate_mutex);
    if (result != 0) {
        fprintf(stderr, "lock demo gate: %s\n", strerror(result));
        atomic_store(&abort_demo, true);
        atomic_store(&release_workers, true);
        /* Timed waiters also guarantee eventual release if the gate is unusable. */
        pthread_cond_broadcast(&gate_condition);
    } else {
        struct timespec deadline;
        if (clock_gettime(CLOCK_REALTIME, &deadline) != 0) {
            perror("clock_gettime");
            atomic_store(&abort_demo, true);
        } else {
            deadline.tv_sec += GATE_TIMEOUT_SECONDS;
        }
        while (ready_count < created && !atomic_load(&abort_demo)) {
            result = pthread_cond_timedwait(&gate_condition, &gate_mutex, &deadline);
            if (result == ETIMEDOUT) {
                fprintf(stderr, "timed out waiting for demo workers to become ready\n");
                atomic_store(&abort_demo, true);
                break;
            }
            if (result != 0) {
                fprintf(stderr, "wait for demo workers: %s\n", strerror(result));
                atomic_store(&abort_demo, true);
                break;
            }
        }
        if (created != 2U) {
            atomic_store(&abort_demo, true);
        }
        atomic_store(&release_workers, true);
        pthread_cond_broadcast(&gate_condition);
        result = pthread_mutex_unlock(&gate_mutex);
        if (result != 0) {
            fprintf(stderr, "unlock demo gate: %s\n", strerror(result));
            atomic_store(&abort_demo, true);
            /* The workers' timed waits provide a bounded fallback. */
            pthread_cond_broadcast(&gate_condition);
        }
    }

    bool succeeded = created == 2U && !atomic_load(&abort_demo);
    for (unsigned int i = 0U; i < created; i++) {
        void *thread_result = NULL;
        result = pthread_join(threads[i], &thread_result);
        if (result != 0) {
            fprintf(stderr, "pthread_join demo worker: %s\n", strerror(result));
            fflush(stderr);
            _Exit(EXIT_FAILURE);
        }
        if (thread_result != NULL) {
            fprintf(stderr, "deadlock demo worker did not finish cleanly\n");
            succeeded = false;
        }
    }

    if (succeeded && atomic_load(&blocked_attempts) > 0U) {
        puts("Opposing lock order detected; trylock released safely (no hang). ");
    } else if (succeeded) {
        puts("No lock contention observed; rerun the demonstration.");
        succeeded = false;
    }

    pthread_cond_destroy(&gate_condition);
    pthread_mutex_destroy(&gate_mutex);
    pthread_mutex_destroy(&mutex_b);
    pthread_mutex_destroy(&mutex_a);
    return succeeded ? EXIT_SUCCESS : EXIT_FAILURE;
}
