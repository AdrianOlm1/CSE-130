#include "kvs_fifo.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// All my Node creations
typedef struct NodeObj {
  char key[KVS_KEY_MAX];
  char value[KVS_VALUE_MAX];
  struct NodeObj* next;
  int flush;
} NodeObj;

Node newNodeFifo(const char* key, const char* value) {
  Node N = malloc(sizeof(NodeObj));
  // assert(N!=NULL);

  strcpy(N->key, key);
  strcpy(N->value, value);
  N->next = NULL;
  N->flush = 0;

  return N;
}

void freeNodeFifo(Node* pN) {
  if (pN != NULL && *pN != NULL) {
    free(*pN);
    *pN = NULL;
  }
}

// KVS creations
struct kvs_fifo {
  // TODO: add necessary variables
  kvs_base_t* kvs_base;
  int capacity;
  int in;
  Node head;
  Node tail;
};

kvs_fifo_t* kvs_fifo_new(kvs_base_t* kvs, int capacity) {
  kvs_fifo_t* kvs_fifo = malloc(sizeof(kvs_fifo_t));
  if (kvs_fifo == NULL) {
    return NULL;
  }

  kvs_fifo->kvs_base = kvs;
  kvs_fifo->capacity = capacity;

  // TODO: initialize other variables
  kvs_fifo->head = kvs_fifo->tail = NULL;
  kvs_fifo->in = 0;

  return kvs_fifo;
}

void kvs_fifo_free(kvs_fifo_t** ptr) {
  // TODO: free dynamically allocated memory
  if (ptr != NULL && *ptr != NULL) {
    kvs_fifo_t* kvs_fifo = *ptr;

    while (kvs_fifo->in > 0) {
      deleteFrontFifo(kvs_fifo);
    }

    free(kvs_fifo);
    *ptr = NULL;
  }
}

// Deletes front of the list
void deleteFrontFifo(kvs_fifo_t* kvs) {
  if (kvs == NULL || kvs->head == NULL) {
    return;
  }

  Node N = kvs->head;
  if (kvs->in == 1) {
    kvs->head = kvs->tail = NULL;
  } else {
    kvs->head = kvs->head->next;
  }
  if (N->flush == 1) {
    kvs_base_set(kvs->kvs_base, N->key, N->value);
  }
  freeNodeFifo(&N);
  kvs->in--;
}

// Searches for a specific key value pair in the list
int searchFifo(kvs_fifo_t* kvs, const char* key) {
  if (kvs->in == 0) {
    return -1;
  }
  // Itereates through each node to find the index of the key
  Node current = kvs->head;
  for (int i = 0; i < kvs->in; ++i) {
    if (strcmp((current->key), key) == 0) {
      return i;
    }
    current = current->next;
  }

  return -1;
}
int kvs_fifo_set(kvs_fifo_t* kvs_fifo, const char* key, const char* value) {
  // TODO: implement this function

  // Searches if the key exists, if so ouputs location
  int val = searchFifo(kvs_fifo, key);

  // If key doesn't exist
  if (val == -1) {
    // Deletes front Node if full
    if (kvs_fifo->in == kvs_fifo->capacity) {
      deleteFrontFifo(kvs_fifo);
    }
    // Creates New Node and puts it on back
    Node N = newNodeFifo(key, value);
    N->flush = 1;
    // if list is empty
    if (kvs_fifo->tail != NULL) {
      kvs_fifo->tail->next = N;
    } else {
      kvs_fifo->head = N;
    }
    kvs_fifo->tail = N;
    kvs_fifo->in++;
    return SUCCESS;
  }

  // If key was found
  else if (val >= 0 && val < kvs_fifo->capacity) {
    Node N = kvs_fifo->head;
    for (int i = 0; i < val; ++i) {
      N = N->next;
    }
    N->flush = 1;
    strcpy(N->value, value);
    return SUCCESS;
  }

  return FAILURE;
}

int kvs_fifo_get(kvs_fifo_t* kvs_fifo, const char* key, char* value) {
  // TODO: implement this function

  int val = searchFifo(kvs_fifo, key);

  // If the key was found
  if (val >= 0 && val < kvs_fifo->capacity) {
    Node N = kvs_fifo->head;
    for (int i = 0; i < val; ++i) {
      N = N->next;
    }
    strcpy(value, N->value);
    return SUCCESS;
  }
  // If key was not found, go in disk
  else if (val == -1) {
    // Fetch the value from the base KVS
    if (kvs_base_get(kvs_fifo->kvs_base, key, value) == SUCCESS) {
      // Insert the fetched value into the FIFO cache

      if (kvs_fifo->in == kvs_fifo->capacity) {
        deleteFrontFifo(kvs_fifo);
      }
      // Creates New Node and puts it on back
      Node N = newNodeFifo(key, value);
      // if list is empty
      if (kvs_fifo->tail != NULL) {
        kvs_fifo->tail->next = N;
      } else {
        kvs_fifo->head = N;
      }
      kvs_fifo->tail = N;
      kvs_fifo->in++;

      return SUCCESS;
    }
    // If the key is not found in the base KVS, return failure
    return FAILURE;
  }

  return FAILURE;
}

int kvs_fifo_flush(kvs_fifo_t* kvs_fifo) {
  // TODO: implement this function
  // If list is empty

  // Check if head isn't NULL
  Node N = kvs_fifo->head;
  if (N == NULL) {
    exit(1);
  }

  // Adds each node
  for (int i = 0; i < kvs_fifo->in; ++i) {
    if (N->flush == 1) {
      kvs_base_set(kvs_fifo->kvs_base, N->key, N->value);
    }
    N = N->next;
  }

  return SUCCESS;
}
