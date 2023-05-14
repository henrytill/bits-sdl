#pragma once

#include <stdint.h>

#include <SDL.h>

#include "macro.h"

#define MSGQ_FAILURE_VARIANTS                     \
  X(MALLOC, -1, "malloc failed")                  \
  X(SEM_CREATE, -2, "SDL_CreateSemaphore failed") \
  X(SEM_POST, -3, "SDL_SemPost failed")           \
  X(SEM_TRY_WAIT, -4, "SDL_SemTryWait failed")    \
  X(SEM_WAIT, -5, "SDL_SemWait failed")           \
  X(MUTEX_CREATE, -6, "SDL_CreateMutex failed")   \
  X(MUTEX_LOCK, -7, "SDL_LockMutex failed")       \
  X(MUTEX_UNLOCK, -8, "SDL_UnlockMutex failed")

enum MessageQueueFailure {
#define X(variant, i, str) MSGQ_FAILURE_##variant = i,
  MSGQ_FAILURE_VARIANTS
#undef X
};

#define MSG_TAG_VARIANTS \
  X(NONE, 0, "NONE")     \
  X(SOME, 1, "SOME")     \
  X(QUIT, 2, "QUIT")

enum MessageTag {
#define X(variant, i, str) MSG_TAG_##variant = 1 << i,
  MSG_TAG_VARIANTS
#undef X
};

struct Message {
  enum MessageTag tag;
  intptr_t value;
};

/// A synchronous bounded message queue
struct MessageQueue {
  struct Message *buffer; // Buffer to hold messages
  uint32_t capacity;      // Maximum size of the buffer
  size_t front;           // Index of the front message in the buffer
  size_t rear;            // Index of the rear message in the buffer
  SDL_sem *empty;         // Semaphore to track empty slots in the buffer
  SDL_sem *full;          // Semaphore to track filled slots in the buffer
  SDL_mutex *lock;        // Mutex lock to protect buffer access
};

///
/// Return the tag string associated with a message tag.
///
/// @param tag A message tag.
/// @return The tag string associated with the given tag, or NULL if the tag is invalid.
///
const char *msgq_tag(enum MessageTag tag);

///
/// Returns the error message associated with a return code.
///
/// @param rc A return code.
/// @return The error message associated with the given return code, or NULL if the return code is invalid.
///
const char *msgq_error(int rc);

///
/// Initializes a new bounded queue with the given capacity.
///
/// Does not allocate memory for the queue.  Useful for statically allocated queues.
///
/// @param queue A MessageQueue.
/// @param capacity The maximum number of messages the queue can hold.
/// @return 0 on success, or a negative value on error.
///
int msgq_init(struct MessageQueue *queue, uint32_t capacity);

///
/// Creates a new bounded queue with the given capacity.
///
/// Allocates memory for the queue and initializes it.
///
/// @param capacity The maximum number of messages the queue can hold.
/// @return A pointer to a new MessageQueue, or NULL on error.
///
struct MessageQueue *msgq_create(uint32_t capacity);

///
/// Adds an message to the back of the queue.
///
/// @param queue A MessageQueue.
/// @param in Message to add to the back of the queue.
/// @return 0 if the message was added to the queue, 1 if the queue is full, or a negative value on error.
///
int msgq_put(struct MessageQueue *queue, struct Message *in);

///
/// Removes and returns the message at the front of the queue, blocking if the queue is empty.
///
/// @param queue A MessageQueue.
/// @param out The message at the front of the queue.
/// @return 0 if a message was removed from the queue, or a negative value on error.
///
int msgq_get(struct MessageQueue *queue, struct Message *out);

///
/// Returns the number of messages in the queue.
///
/// @param queue A MessageQueue.
/// @return The number of messages in the queue.
///
uint32_t msgq_size(struct MessageQueue *queue);

///
/// Frees resources associated with the queue.
///
/// Does not free the queue itself.  Useful for statically allocated queues.
///
/// @param queue A MessageQueue.
/// @see msgq_init()
///
void msgq_finish(struct MessageQueue *queue);

///
/// Frees resources associated with the queue.
///
/// Also frees the queue itself.
///
/// Consider using _cleanup_msgq_ for scoped cleanup.
///
/// @param queue A MessageQueue.
/// @see msgq_create()
///
void msgq_destroy(struct MessageQueue *queue);

DEFINE_TRIVIAL_CLEANUP_FUNC(struct MessageQueue *, msgq_destroy)
#define _cleanup_msgq_ _cleanup_(msgq_destroyp)
