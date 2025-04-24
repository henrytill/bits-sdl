#ifndef SDL_BITS_INCLUDE_MESSAGE_QUEUE_H
#define SDL_BITS_INCLUDE_MESSAGE_QUEUE_H

#include <stddef.h>
#include <stdint.h>

enum message_queue_failure {
  MSGQ_FAILURE_NULL_POINTER = 1,
  MSGQ_FAILURE_MALLOC = 2,
  MSGQ_FAILURE_SEM_CREATE = 3,
  MSGQ_FAILURE_SEM_POST = 4,
  MSGQ_FAILURE_SEM_TRY_WAIT = 5,
  MSGQ_FAILURE_SEM_WAIT = 6,
  MSGQ_FAILURE_MUTEX_CREATE = 7,
  MSGQ_FAILURE_MUTEX_LOCK = 8,
  MSGQ_FAILURE_MUTEX_UNLOCK = 9,
  MSGQ_FAILURE_MIN = 10,
};

static inline char const *message_queue_failure_str(enum message_queue_failure failure) {
  switch (failure) {
  case MSGQ_FAILURE_NULL_POINTER:
    return "NULL pointer";
  case MSGQ_FAILURE_MALLOC:
    return "malloc failed";
  case MSGQ_FAILURE_SEM_CREATE:
    return "create semaphore failed";
  case MSGQ_FAILURE_SEM_POST:
    return "post sempahore failed";
  case MSGQ_FAILURE_SEM_TRY_WAIT:
    return "try-wait sempahore failed";
  case MSGQ_FAILURE_SEM_WAIT:
    return "wait semaphore failed";
  case MSGQ_FAILURE_MUTEX_CREATE:
    return "create mutex failed";
  case MSGQ_FAILURE_MUTEX_LOCK:
    return "lock mutex failed";
  case MSGQ_FAILURE_MUTEX_UNLOCK:
    return "unlock mutex failed";
  case MSGQ_FAILURE_MIN:
  default:
    return NULL;
  }
}

enum message_tag {
  MSG_TAG_NONE = 0,
  MSG_TAG_SOME = 1,
  MSG_TAG_QUIT = 2,
};

static inline char const *message_tag_str(enum message_tag tag) {
  switch (tag) {
  case MSG_TAG_NONE:
    return "NONE";
  case MSG_TAG_SOME:
    return "SOME";
  case MSG_TAG_QUIT:
    return "QUIT";
  default:
    return NULL;
  }
}

struct message {
  enum message_tag tag;
  intptr_t value;
};

/// A thread-safe bounded message queue
struct message_queue;

/// Returns the error message associated with a return code.
///
/// @param rc A return code.
/// @return The error message associated with the given return code, or NULL if the return code is invalid.
char const *message_queue_failure(int rc);

/// Return the tag string associated with a message tag.
///
/// @param tag A message tag.
/// @return The tag string associated with the given tag, or NULL if the tag is invalid.
char const *message_queue_tag(int tag);

/// Creates a new bounded queue with the given capacity.
///
/// Allocates memory for the queue and initializes it.
///
/// @param capacity The maximum number of messages the queue can hold.
/// @return A pointer to a new message_queue, or NULL on error.
/// @see message_queue_destroy()
struct message_queue *message_queue_create(uint32_t capacity);

/// Frees resources associated with the queue.
///
/// Also frees the queue itself.
///
/// @param queue Message queue.
/// @see message_queue_create()
void message_queue_destroy(struct message_queue *queue);

/// Adds an message to the back of the queue.
///
/// @param queue Message queue.
/// @param in The message to add to the back of the queue.
/// @return 0 if the message was added to the queue, 1 if the queue is full, or a negative value on error.
int message_queue_put(struct message_queue *queue, struct message *in);

/// Removes and returns the message at the front of the queue, blocking if the queue is empty.
///
/// @param queue Message queue.
/// @param out The message at the front of the queue.
/// @return 0 if a message was removed from the queue, or a negative value on error.
int message_queue_get(struct message_queue *queue, struct message *out);

/// Returns the number of messages in the queue.
///
/// @param queue Message queue.
/// @return The number of messages in the queue.
uint32_t message_queue_size(struct message_queue *queue);

#endif // SDL_BITS_INCLUDE_MESSAGE_QUEUE_H
