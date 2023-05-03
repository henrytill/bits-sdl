#include <SDL.h>

#include "msgq.h"

static const char *const tagString[] = {
  [NONE >> 1] = "NONE",
  [SOME >> 1] = "SOME",
  [QUIT >> 1] = "QUIT",
};

const char *msgq_tag(enum MessageTag tag) {
  extern const char *const tagString[];
  if (tag > QUIT || tag < NONE) {
    return NULL;
  }
  return tagString[tag >> 1];
}

static const char *const errorString[] = {
  [-MSGQ_FAILURE_MALLOC] = "malloc failed",
  [-MSGQ_FAILURE_SEM_CREATE] = "SDL_CreateSemaphore failed",
  [-MSGQ_FAILURE_SEM_POST] = "SDL_SemPost failed",
  [-MSGQ_FAILURE_SEM_TRY_WAIT] = "SDL_SemTryWait failed",
  [-MSGQ_FAILURE_SEM_WAIT] = "SDL_SemWait failed",
  [-MSGQ_FAILURE_MUTEX_CREATE] = "SDL_CreateMutex failed",
  [-MSGQ_FAILURE_MUTEX_LOCK] = "SDL_LockMutex failed",
  [-MSGQ_FAILURE_MUTEX_UNLOCK] = "SDL_UnlockMutex failed",
};

const char *msgq_error(int rc) {
  extern const char *const errorString[];
  if (rc > MSGQ_FAILURE_MALLOC || rc < MSGQ_FAILURE_MUTEX_UNLOCK) {
    return NULL;
  }
  return errorString[-rc];
}

int msgq_init(struct MessageQueue *queue, uint32_t capacity) {
  queue->buffer = malloc((size_t)capacity * sizeof(*queue->buffer));
  if (queue->buffer == NULL) {
    return MSGQ_FAILURE_MALLOC;
  }
  queue->capacity = capacity;
  queue->front = 0;
  queue->rear = 0;
  queue->empty = SDL_CreateSemaphore(capacity);
  if (queue->empty == NULL) {
    free(queue->buffer);
    return MSGQ_FAILURE_SEM_CREATE;
  }
  queue->full = SDL_CreateSemaphore(0);
  if (queue->empty == NULL) {
    SDL_DestroySemaphore(queue->empty);
    free(queue->buffer);
    return MSGQ_FAILURE_SEM_CREATE;
  }
  queue->lock = SDL_CreateMutex();
  if (queue->lock == NULL) {
    SDL_DestroySemaphore(queue->full);
    SDL_DestroySemaphore(queue->empty);
    free(queue->buffer);
    return MSGQ_FAILURE_MUTEX_CREATE;
  }
  return 0;
}

struct MessageQueue *msgq_create(uint32_t capacity) {
  struct MessageQueue *queue = malloc(sizeof(*queue));
  if (queue == NULL) {
    return NULL;
  }
  int rc = msgq_init(queue, capacity);
  if (rc < 0) {
    free(queue);
    return NULL;
  }
  return queue;
}

int msgq_put(struct MessageQueue *queue, struct Message *in) {
  int rc;

  rc = SDL_SemTryWait(queue->empty);
  if (rc == SDL_MUTEX_TIMEDOUT) {
    return 1;
  } else if (rc < 0) {
    return MSGQ_FAILURE_SEM_TRY_WAIT;
  }
  rc = SDL_LockMutex(queue->lock);
  if (rc == -1) {
    return MSGQ_FAILURE_MUTEX_LOCK;
  }
  queue->buffer[queue->rear] = *in;
  queue->rear = (queue->rear + 1) % queue->capacity;
  rc = SDL_UnlockMutex(queue->lock);
  if (rc == -1) {
    return MSGQ_FAILURE_MUTEX_UNLOCK;
  }
  rc = SDL_SemPost(queue->full);
  if (rc < 0) {
    return MSGQ_FAILURE_SEM_POST;
  }
  return 0;
}

int msgq_get(struct MessageQueue *queue, struct Message *out) {
  int rc;

  rc = SDL_SemWait(queue->full);
  if (rc < 0) {
    return MSGQ_FAILURE_SEM_WAIT;
  }
  rc = SDL_LockMutex(queue->lock);
  if (rc == -1) {
    return MSGQ_FAILURE_MUTEX_LOCK;
  }
  *out = queue->buffer[queue->front];
  queue->front = (queue->front + 1) % queue->capacity;
  rc = SDL_UnlockMutex(queue->lock);
  if (rc == -1) {
    return MSGQ_FAILURE_MUTEX_UNLOCK;
  }
  rc = SDL_SemPost(queue->empty);
  if (rc < 0) {
    return MSGQ_FAILURE_SEM_POST;
  }
  return 0;
}

uint32_t msgq_size(struct MessageQueue *queue) {
  if (queue == NULL) return 0;
  return SDL_SemValue(queue->full);
}

void msgq_finish(struct MessageQueue *queue) {
  if (queue == NULL) return;
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
  };
  if (queue->lock != NULL) {
    SDL_DestroyMutex(queue->lock);
    queue->lock = NULL;
  };
}

void msgq_destroy(struct MessageQueue *queue) {
  if (queue == NULL) return;
  msgq_finish(queue);
  free(queue);
}
