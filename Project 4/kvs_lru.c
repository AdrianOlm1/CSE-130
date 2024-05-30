#include "kvs_lru.h"

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

Node newNodeLru(const char* key, const char* value) {
  Node N = malloc(sizeof(NodeObj));
  // assert(N!=NULL);

  strcpy(N->key, key);
  strcpy(N->value, value);
  N->next = NULL;
  N->flush = 0;

  return N;
}

void freeNodeLru(Node* pN) {
  if (pN != NULL && *pN != NULL) {
    free(*pN);
    *pN = NULL;
  }
}

struct kvs_lru {
  // TODO: add necessary variables
  kvs_base_t* kvs_base;
  int capacity;
  int in;
  Node head;
  Node tail;
};

kvs_lru_t* kvs_lru_new(kvs_base_t* kvs, int capacity) {
  kvs_lru_t* kvs_lru = malloc(sizeof(kvs_lru_t));
  kvs_lru->kvs_base = kvs;
  kvs_lru->capacity = capacity;

  // TODO: initialize other variables
  kvs_lru->head = kvs_lru->tail = NULL;
  kvs_lru->in = 0;

  return kvs_lru;
}

void kvs_lru_free(kvs_lru_t** ptr) {
  // TODO: free dynamically allocated memory
  if (ptr != NULL && *ptr != NULL) {
    while ((*ptr)->in != 0) {
      deleteFrontLru((*ptr));
    }
    free(*ptr);
    *ptr = NULL;
  }
}

// Deletes front of the list
void deleteFrontLru(kvs_lru_t* kvs) {
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
  freeNodeLru(&N);
  kvs->in--;
}

int surgery(kvs_lru_t* kvs, const char* key) {
  if ((kvs->head) == NULL || kvs->in == 0) {
    return -1;
  }

  if (kvs->in == 1) {
    return 1;
  }

  Node Nprev = NULL;
  Node N = kvs->head;

  for (int i = 0; i < kvs->in; ++i) {
    if (strcmp(N->key, key) == 0) {
      // If key is at head
      if (N == kvs->head) {
        kvs->head = N->next;
        N->next = NULL;
        kvs->tail->next = N;
        kvs->tail = N;
        return 1;
      }
      // If key is at tail
      else if (N == kvs->tail) {
        return 1;
      } else {
        Nprev->next = N->next;
        N->next = NULL;
        kvs->tail->next = N;
        kvs->tail = N;
        return 1;
      }
    }
    Nprev = N;
    N = N->next;
  }
  return -1;
}

// Searches for a specific key value pair in the list
int searchLru(kvs_lru_t* kvs, const char* key) {
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

int kvs_lru_set(kvs_lru_t* kvs_lru, const char* key, const char* value) {
  // TODO: implement this function
  int val = searchLru(kvs_lru, key);

  if (val == -1) {
    if (kvs_lru->in == kvs_lru->capacity) {
      deleteFrontLru(kvs_lru);
    }

    Node N = newNodeLru(key, value);
    N->flush = 1;
    if (kvs_lru->tail != NULL) {
      kvs_lru->tail->next = N;
    } else {
      kvs_lru->head = N;
    }
    kvs_lru->tail = N;
    kvs_lru->in++;
    return SUCCESS;
  }

  else if (val >= 0 && val < kvs_lru->capacity) {
    Node N = kvs_lru->head;
    for (int i = 0; i < val; ++i) {
      N = N->next;
    }
    if (strcmp(N->value, value) != 0) {
      strcpy(N->value, value);
      N->flush = 1;  // Mark the node to be flushed
    }

    if (surgery(kvs_lru, key) == -1) {
      printf("Error with surgery on string %s in set\n", key);
      return FAILURE;
    }
    return SUCCESS;
  }

  return FAILURE;
}

int kvs_lru_get(kvs_lru_t* kvs_lru, const char* key, char* value) {
  // TODO: implement this function

  int val = searchLru(kvs_lru, key);

  if (val >= 0 && val < kvs_lru->capacity) {
    Node N = kvs_lru->head;
    for (int i = 0; i < val; ++i) {
      N = N->next;
    }
    strcpy(value, N->value);

    if (surgery(kvs_lru, key) == -1) {
      printf("Error with surgery on string %s in get\n", key);
      return FAILURE;
    }
    return SUCCESS;
  }

  else if (val == -1) {
    // Fetch the value from the base KVS
    if (kvs_base_get(kvs_lru->kvs_base, key, value) == SUCCESS) {
      if (kvs_lru->in == kvs_lru->capacity) {
        deleteFrontLru(kvs_lru);
      }

      Node N = newNodeLru(key, value);
      if (kvs_lru->tail != NULL) {
        kvs_lru->tail->next = N;
      } else {
        kvs_lru->head = N;
      }
      kvs_lru->tail = N;
      kvs_lru->in++;
      return SUCCESS;
    }
    // If the key is not found in the base KVS, return failure
    return FAILURE;
  }
  return FAILURE;
}

int kvs_lru_flush(kvs_lru_t* kvs_lru) {
  // TODO: implement this function

  Node N = kvs_lru->head;
  if (N == NULL) {
    exit(1);
  }

  for (int i = 0; i < kvs_lru->in; ++i) {
    if (N->flush == 1) {
      kvs_base_set(kvs_lru->kvs_base, N->key, N->value);
    }
    N = N->next;
  }

  return SUCCESS;
}
