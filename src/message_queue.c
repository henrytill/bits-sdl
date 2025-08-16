#include "message_queue.h"

#include <SDL.h>

struct message_queue {
    struct message *buffer; // Buffer to hold messages
    uint32_t capacity;      // Maximum size of the buffer
    size_t front;           // Index of the front message in the buffer
    size_t rear;            // Index of the rear message in the buffer
    SDL_sem *empty;         // Semaphore to track empty slots in the buffer
    SDL_sem *full;          // Semaphore to track filled slots in the buffer
    SDL_mutex *lock;        // Mutex lock to protect buffer access
};

static int message_queue_init(struct message_queue *queue, uint32_t capacity) {
    if (queue == NULL) {
        return -MSGQ_FAILURE_NULL_POINTER;
    }
    queue->buffer = calloc((size_t)capacity, sizeof(*queue->buffer));
    if (queue->buffer == NULL) {
        return -MSGQ_FAILURE_MALLOC;
    }
    queue->capacity = capacity;
    queue->front = 0;
    queue->rear = 0;
    queue->empty = SDL_CreateSemaphore(capacity);
    if (queue->empty == NULL) {
        free(queue->buffer);
        return -MSGQ_FAILURE_SEM_CREATE;
    }
    queue->full = SDL_CreateSemaphore(0);
    if (queue->full == NULL) {
        SDL_DestroySemaphore(queue->empty);
        free(queue->buffer);
        return -MSGQ_FAILURE_SEM_CREATE;
    }
    queue->lock = SDL_CreateMutex();
    if (queue->lock == NULL) {
        SDL_DestroySemaphore(queue->full);
        SDL_DestroySemaphore(queue->empty);
        free(queue->buffer);
        return -MSGQ_FAILURE_MUTEX_CREATE;
    }
    return 0;
}

static void message_queue_finish(struct message_queue *queue) {
    if (queue == NULL) {
        return;
    }
    queue->capacity = 0;
    queue->front = 0;
    queue->rear = 0;
    if (queue->buffer != NULL) {
        free(queue->buffer);
        queue->buffer = NULL;
    }
    if (queue->empty != NULL) {
        SDL_DestroySemaphore(queue->empty);
        queue->empty = NULL;
    }
    if (queue->full != NULL) {
        SDL_DestroySemaphore(queue->full);
        queue->full = NULL;
    }
    if (queue->lock != NULL) {
        SDL_DestroyMutex(queue->lock);
        queue->lock = NULL;
    }
}

struct message_queue *message_queue_create(uint32_t capacity) {
    struct message_queue *queue = calloc(1, sizeof(*queue));
    if (queue == NULL) {
        return NULL;
    }
    int rc = message_queue_init(queue, capacity);
    if (rc < 0) {
        free(queue);
        return NULL;
    }
    return queue;
}

void message_queue_destroy(struct message_queue *queue) {
    if (queue == NULL) {
        return;
    }
    message_queue_finish(queue);
    free(queue);
}

int message_queue_put(struct message_queue *queue, struct message *in) {
    int rc = SDL_SemTryWait(queue->empty);
    if (rc == SDL_MUTEX_TIMEDOUT) {
        return 1;
    }
    if (rc < 0) {
        return -MSGQ_FAILURE_SEM_TRY_WAIT;
    }
    rc = SDL_LockMutex(queue->lock);
    if (rc == -1) {
        return -MSGQ_FAILURE_MUTEX_LOCK;
    }
    queue->buffer[queue->rear] = *in;
    queue->rear = (queue->rear + 1) % queue->capacity;
    rc = SDL_UnlockMutex(queue->lock);
    if (rc == -1) {
        return -MSGQ_FAILURE_MUTEX_UNLOCK;
    }
    rc = SDL_SemPost(queue->full);
    if (rc < 0) {
        return -MSGQ_FAILURE_SEM_POST;
    }
    return 0;
}

int message_queue_get(struct message_queue *queue, struct message *out) {
    int rc = SDL_SemWait(queue->full);
    if (rc < 0) {
        return -MSGQ_FAILURE_SEM_WAIT;
    }
    rc = SDL_LockMutex(queue->lock);
    if (rc == -1) {
        return -MSGQ_FAILURE_MUTEX_LOCK;
    }
    *out = queue->buffer[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    rc = SDL_UnlockMutex(queue->lock);
    if (rc == -1) {
        return -MSGQ_FAILURE_MUTEX_UNLOCK;
    }
    rc = SDL_SemPost(queue->empty);
    if (rc < 0) {
        return -MSGQ_FAILURE_SEM_POST;
    }
    return 0;
}

uint32_t message_queue_size(struct message_queue *queue) {
    if (queue == NULL) {
        return 0;
    }
    return SDL_SemValue(queue->full);
}
