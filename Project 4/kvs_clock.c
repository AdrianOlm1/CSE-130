#include "kvs_clock.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// All my Node creations
typedef struct NodeObj {
  char key[KVS_KEY_MAX];
  char value[KVS_VALUE_MAX];
  struct NodeObj* next;
  struct NodeObj* prev;
  int bit;
  int flush;
} NodeObj;

Node newNodeClock(const char* key, const char* value, int bitIn) {
  Node N = malloc(sizeof(NodeObj));
  // assert(N!=NULL);

  strcpy(N->key, key);
  strcpy(N->value, value);
  N->next = N->prev = NULL;
  N->bit = bitIn;
  N->flush = 0;

  return N;
}

void freeNodeClock(Node* pN) {
  if (pN != NULL && *pN != NULL) {
    free(*pN);
    *pN = NULL;
  }
}

struct kvs_clock {
  // TODO: add necessary variables
  kvs_base_t* kvs_base;
  int capacity;
  int in;
  Node cursor;
};

kvs_clock_t* kvs_clock_new(kvs_base_t* kvs, int capacity) {
  kvs_clock_t* kvs_clock = malloc(sizeof(kvs_clock_t));
  kvs_clock->kvs_base = kvs;
  kvs_clock->capacity = capacity;

  // TODO: initialize other variables
  kvs_clock->in = 0;

  if (capacity < 1) {
    kvs_clock->cursor = NULL;
  }

  Node head = newNodeClock("", "", 0);
  Node current = head;

  for (int i = 1; i < capacity; ++i) {
    Node newNode = newNodeClock("", "", 0);
    current->next = newNode;
    newNode->prev = current;
    current = newNode;
  }

  current->next = head;
  head->prev = current;

  kvs_clock->cursor = head;

  return kvs_clock;
}

void kvs_clock_free(kvs_clock_t** ptr) {
  // TODO: free dynamically allocated memory
  if ((*ptr) != NULL && ptr != NULL) {
    freeClock(*ptr);
    free((*ptr));
    *ptr = NULL;
  }

  // free(*ptr);
  //*ptr = NULL;
}

void freeClock(kvs_clock_t* kvs) {
  if (kvs == NULL || kvs->cursor == NULL) {
    return;
  }

  Node current = kvs->cursor;
  Node temp;
  int count = 0;

  do {
    temp = current;
    current = current->next;
    freeNodeClock(&temp);
    count++;
  } while (current != kvs->cursor && count < kvs->capacity);

  kvs->cursor = NULL;
}

int searchClock(kvs_clock_t* kvs, const char* key) {
  if (kvs == NULL || kvs->cursor == NULL) {
    return -1;
  }

  Node current = kvs->cursor;
  for (int i = 0; i < kvs->capacity; ++i) {
    if (strcmp((current->key), key) == 0) {
      return i;
    }
    current = current->next;
  }

  return -1;
}

int kvs_clock_set(kvs_clock_t* kvs_clock, const char* key, const char* value) {
  // TODO: implement this function
  int val = searchClock(kvs_clock, key);

  if (val == -1) {
    while (1) {
      if (kvs_clock->cursor->bit == 0) {
        if (kvs_clock->cursor->flush == 1) {
          kvs_base_set(kvs_clock->kvs_base, kvs_clock->cursor->key,
                       kvs_clock->cursor->value);
        }
        strcpy(kvs_clock->cursor->key, key);
        strcpy(kvs_clock->cursor->value, value);
        kvs_clock->cursor->flush = 1;
        kvs_clock->cursor->bit = 1;

        if (kvs_clock->in < kvs_clock->capacity) {
          kvs_clock->in++;
        }

        break;
      }
      kvs_clock->cursor->bit = 0;
      kvs_clock->cursor = kvs_clock->cursor->next;
    }

    return SUCCESS;
  } else if (val >= 0 && val < kvs_clock->capacity) {
    for (int i = 0; i < val; ++i) {
      kvs_clock->cursor = kvs_clock->cursor->next;
    }

    if (strcmp(kvs_clock->cursor->key, key) != 0) {
      printf("Error finding correct key in set\n");
      exit(1);
    }

    kvs_clock->cursor->flush = 1;

    strcpy(kvs_clock->cursor->value, value);
    kvs_clock->cursor->bit = 1;

    kvs_clock->cursor = kvs_clock->cursor->next;

    return SUCCESS;
  }

  return FAILURE;
}

int kvs_clock_get(kvs_clock_t* kvs_clock, const char* key, char* value) {
  // TODO: implement this function
  int val = searchClock(kvs_clock, key);

  if (val >= 0 && val < kvs_clock->capacity) {
    for (int i = 0; i < val; ++i) {
      kvs_clock->cursor = kvs_clock->cursor->next;
    }

    if (strcmp(kvs_clock->cursor->key, key) != 0) {
      printf("Error finding correct key in get\n");
      exit(1);
    }

    strcpy(value, kvs_clock->cursor->value);
    kvs_clock->cursor->bit = 1;

    kvs_clock->cursor = kvs_clock->cursor->next;

    return SUCCESS;
  } else if (val == -1) {
    // Fetch the value from the base KVS
    if (kvs_base_get(kvs_clock->kvs_base, key, value) == SUCCESS) {
      // Insert the fetched value into the FIFO cache
      while (1) {
        if (kvs_clock->cursor->bit == 0) {
          if (kvs_clock->cursor->flush == 1) {
            kvs_base_set(kvs_clock->kvs_base, kvs_clock->cursor->key,
                         kvs_clock->cursor->value);
            kvs_clock->cursor->flush = 0;
          }

          strcpy(kvs_clock->cursor->key, key);
          strcpy(kvs_clock->cursor->value, value);
          kvs_clock->cursor->bit = 1;

          if (kvs_clock->in < kvs_clock->capacity) {
            kvs_clock->in++;
          }

          break;
        }
        kvs_clock->cursor->bit = 0;
        kvs_clock->cursor = kvs_clock->cursor->next;
      }

      return SUCCESS;
    }
    // If the key is not found in the base KVS, return failure
    return FAILURE;
  }

  return FAILURE;
}

int kvs_clock_flush(kvs_clock_t* kvs_clock) {
  // TODO: implement this function

  // Check if head isn't NULL
  Node N = kvs_clock->cursor;
  if (N == NULL) {
    exit(1);
  }
  // Adds each node
  for (int i = 0; i < kvs_clock->in; ++i) {
    if (N->flush == 1) {
      kvs_base_set(kvs_clock->kvs_base, N->key, N->value);
    }
    N = N->next;
  }

  return SUCCESS;
}
