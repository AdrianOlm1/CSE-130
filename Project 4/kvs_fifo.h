#pragma once

#include "kvs_base.h"

struct NodeObj;
typedef struct NodeObj* Node;

struct kvs_fifo;
typedef struct kvs_fifo kvs_fifo_t;

kvs_fifo_t* kvs_fifo_new(kvs_base_t* kvs, int capacity);
void kvs_fifo_free(kvs_fifo_t** ptr);

int kvs_fifo_set(kvs_fifo_t* kvs_fifo, const char* key, const char* value);
int kvs_fifo_get(kvs_fifo_t* kvs_fifo, const char* key, char* value);
int kvs_fifo_flush(kvs_fifo_t* kvs_fifo);

// Function prototypes
Node newNodeFifo(const char* key, const char* value);
void freeNodeFifo(Node* pN);
void deleteFrontFifo(kvs_fifo_t* kvs);
int searchFifo(kvs_fifo_t* kvs, const char* key);
