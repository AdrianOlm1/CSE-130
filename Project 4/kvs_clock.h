#pragma once

#include "kvs_base.h"

struct NodeObj;
typedef struct NodeObj* Node;

struct kvs_clock;
typedef struct kvs_clock kvs_clock_t;

kvs_clock_t* kvs_clock_new(kvs_base_t* kvs, int capacity);
void kvs_clock_free(kvs_clock_t** ptr);

int kvs_clock_set(kvs_clock_t* kvs_clock, const char* key, const char* value);
int kvs_clock_get(kvs_clock_t* kvs_clock, const char* key, char* value);
int kvs_clock_flush(kvs_clock_t* kvs_clock);

Node newNodeClock(const char* key, const char* value, int bitIn);
void freeNodeClock(Node* pN);
int searchClock(kvs_clock_t* kvs, const char* key);
void freeClock(kvs_clock_t* kvs);
