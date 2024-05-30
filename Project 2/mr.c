#include "mr.h"

#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash.h"
#include "kvlist.h"

typedef struct {
  kvlist_t *kvlist_inp;
  kvlist_t *kvlist_out;
  void (*mapper)(kvpair_t *, kvlist_t *);
} ThreadArgsMapper;

typedef struct {
  kvlist_t **kvlist_inp;
  kvlist_t *kvlist_out;
  int length;
  int start;
  void (*reducer)(char *, kvlist_t *, kvlist_t *);
} ThreadArgsReduce;

char *toLowerS(char *s) {
  for (char *p = s; *p; p++) {
    *p = tolower(*p);
  }
  return s;
}

void *mapper_func(void *arg) {
  ThreadArgsMapper *thread_args = (ThreadArgsMapper *)arg;

  kvlist_iterator_t *itor = kvlist_iterator_new(thread_args->kvlist_inp);
  for (;;) {
    kvpair_t *pair = kvlist_iterator_next(itor);
    if (pair == NULL) {
      break;
    }
    thread_args->mapper(pair, thread_args->kvlist_out);
  }
  kvlist_iterator_free(&itor);

  return NULL;
}

void *reducer_func(void *arg) {
  ThreadArgsReduce *thread_args = (ThreadArgsReduce *)arg;

  int start = thread_args->start;
  int length = thread_args->length;

  // Create local output kvlist for this thread
  kvlist_t *local_output = kvlist_new();

  unsigned long temp = hash("azazxxxzzz");
  // Process the input kvlists
  for (int i = start; i < start + length; ++i) {
    kvlist_iterator_t *itor = kvlist_iterator_new(thread_args->kvlist_inp[i]);
    kvpair_t *pair;
    while ((pair = kvlist_iterator_next(itor)) != NULL) {
      // Apply reducer function to each key-value pair
      if (temp != hash(pair->key)) {
        thread_args->reducer(pair->key, thread_args->kvlist_inp[i],
                             local_output);
        temp = hash(pair->key);
      }
    }
    kvlist_iterator_free(&itor);
  }

  // Store the local output kvlist in the thread_args
  thread_args->kvlist_out = local_output;

  return NULL;
}

size_t kvlist_size(kvlist_t *kv_list) {
  size_t count = 0;
  char delim[] = "\t\r\n !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
  char mine[256];
  char *token;
  char *saveptr;

  // use `strtok_r` to split the line into words and add pairs to `output`
  kvlist_iterator_t *iterator = kvlist_iterator_new(kv_list);
  kvpair_t *pair;

  while ((pair = kvlist_iterator_next(iterator)) != NULL) {
    // Assuming pair->value is a null-terminated string
    strncpy(mine, pair->value, sizeof(mine) - 1);
    mine[sizeof(mine) - 1] = '\0';  // Ensure null termination

    token = strtok_r(mine, delim, &saveptr);
    if (token != NULL) {
      ++count;
    }
    while ((token = strtok_r(NULL, delim, &saveptr)) != NULL) {
      ++count;
    }
  }
  kvlist_iterator_free(&iterator);
  return count;
}

void map_reduce(mapper_t mapper, size_t num_mapper, reducer_t reducer,
                size_t num_reducer, kvlist_t *input, kvlist_t *output) {
  // Count the total number of words in the input kvlist
  size_t total_words = kvlist_size(input);
  // Calculate the number of words per mapper
  size_t words_per_mapper = total_words / num_mapper;
  size_t words_remainder = total_words % num_mapper;

  // Distribute words among mapper kvlists
  kvlist_t **kv_lists = malloc(num_mapper * sizeof(kvlist_t *));
  if (kv_lists == NULL) {
    exit(1);
  }

  // Initialize mapper kvlists
  for (int i = 0; i < (int)num_mapper; i++) {
    kv_lists[i] = kvlist_new();
    if (kv_lists[i] == NULL) {
      exit(1);
    }
  }

  char delim[] = "\t\r\n !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
  char *token;
  char *saveptr;

  // Iterator for input kvlist
  kvlist_iterator_t *iterator = kvlist_iterator_new(input);
  kvpair_t *pair = kvlist_iterator_next(iterator);
  char *mine = pair->value;
  token = strtok_r(mine, delim, &saveptr);
  // Distribute words evenly among mapper kvlists

  for (int i = 0; i < (int)num_mapper; ++i) {
    int words_to_add = (i < (int)words_remainder) ? (int)words_per_mapper + 1
                                                  : (int)words_per_mapper;

    for (int j = 0; j < words_to_add; ++j) {
      if (token == NULL) {
        // If token is NULL, move to the next pair
        pair = kvlist_iterator_next(iterator);
        if (pair == NULL) {
          // If there are no more pairs, break out of the loop
          break;
        }
        // Reset saveptr and get the first token from the new pair's value
        saveptr = NULL;
        mine = pair->value;
        token = strtok_r(mine, delim, &saveptr);
      }
      // Add the token to the current mapper kvlist
      if (token != NULL) {
        kvlist_append(kv_lists[i], kvpair_new("Random", toLowerS(token)));
      } else {
        --j;
      }
      token = strtok_r(NULL, delim, &saveptr);
    }
  }

  // Free iterator
  kvlist_iterator_free(&iterator);

  // Create kv_lists2 for output of mapper
  kvlist_t **kv_lists2 = malloc(num_mapper * sizeof(kvlist_t *));
  if (kv_lists2 == NULL) {
    exit(1);
  }

  for (int i = 0; i < (int)num_mapper; i++) {
    kv_lists2[i] = kvlist_new();
    if (kv_lists2[i] == NULL) {
      exit(1);
    }
  }

  // Threads do their work
  pthread_t thread_ids_map[num_mapper];
  ThreadArgsMapper thread_args_map[num_mapper];

  for (int i = 0; i < (int)num_mapper; i++) {
    thread_args_map[i].kvlist_inp = kv_lists[i];
    thread_args_map[i].kvlist_out = kv_lists2[i];
    thread_args_map[i].mapper = mapper;

    if (pthread_create(&thread_ids_map[i], NULL, mapper_func,
                       &thread_args_map[i]) != 0) {
      fprintf(stderr, "Error creating thread %d\n", i + 1);
      return;
    }
  }
  // Wait for threads to finish
  for (int i = 0; i < (int)num_mapper; i++) {
    pthread_join(thread_ids_map[i], NULL);
  }

  // Creates a combined list of all of the output from mapper and frees both
  // arrays of kvlists
  kvlist_t *combined_list = kvlist_new();
  for (int i = 0; i < (int)num_mapper; ++i) {
    kvlist_extend(combined_list, kv_lists2[i]);
    kvlist_free(&kv_lists[i]);
    kvlist_free(&kv_lists2[i]);
  }
  free(kv_lists);
  free(kv_lists2);

  //**********************************************************************************************************************************************************
  // REDUCER SECTION
  //**********************************************************************************************************************************************************

  // Count how many unique words there are
  kvlist_sort(combined_list);
  int uniqueCount = 0;
  unsigned long tester = hash("zzxxzzax");
  kvlist_iterator_t *itor = kvlist_iterator_new(combined_list);
  while (1) {
    kvpair_t *pair = kvlist_iterator_next(itor);
    if (pair == NULL) {
      break;
    }
    if (hash(pair->key) != tester) {
      uniqueCount++;
      tester = hash(pair->key);
    }
  }
  kvlist_iterator_free(&itor);

  // Creates a kv list for each uniqueword
  kvlist_t **kv_lists3 = malloc(uniqueCount * sizeof(kvlist_t *));
  if (kv_lists3 == NULL) {
    exit(1);
  }
  for (int i = 0; i < uniqueCount; i++) {
    kv_lists3[i] = kvlist_new();
    if (kv_lists3[i] == NULL) {
      exit(1);
    }
  }

  // Sorts all unique words into their own list;
  tester = hash("zzzxxzzax");
  int tempC = -1;
  kvlist_iterator_t *itor2 = kvlist_iterator_new(combined_list);
  for (;;) {
    kvpair_t *pair = kvlist_iterator_next(itor2);
    if (pair == NULL) {
      break;
    }
    if (hash(pair->key) != tester) {
      tester = hash(pair->key);
      tempC++;
    }
    kvlist_append(kv_lists3[tempC], kvpair_new(pair->key, pair->value));
  }

  // Freeing used memory
  kvlist_iterator_free(&itor2);
  kvlist_free(&combined_list);

  // Creates a kv list for each output
  kvlist_t **kv_lists4 = malloc(num_reducer * sizeof(kvlist_t *));
  if (kv_lists4 == NULL) {
    exit(1);
  }
  for (int i = 0; i < (int)num_reducer; i++) {
    kv_lists4[i] = kvlist_new();
    if (kv_lists4[i] == NULL) {
      exit(1);
    }
  }

  // Create threads and arguments
  pthread_t thread_ids_red[num_reducer];
  ThreadArgsReduce thread_args_red[num_reducer];

  // Calculate partitions for reducers
  int partition_size = uniqueCount / num_reducer;
  int remainder = uniqueCount % num_reducer;

  // Start reducers
  for (int i = 0; i < (int)num_reducer; i++) {
    int start = i * partition_size + (i < remainder ? i : remainder);
    int length = partition_size + (i < remainder ? 1 : 0);

    // Initialize thread arguments
    thread_args_red[i].kvlist_inp = kv_lists3;
    thread_args_red[i].kvlist_out = NULL;  // Initialize to NULL
    thread_args_red[i].reducer = reducer;
    thread_args_red[i].start = start;
    thread_args_red[i].length = length;

    // Create threads
    if (pthread_create(&thread_ids_red[i], NULL, reducer_func,
                       &thread_args_red[i]) != 0) {
      fprintf(stderr, "Error creating thread %d\n", i);
      // Handle error
    }
  }

  // Wait for threads to finish
  for (int i = 0; i < (int)num_reducer; i++) {
    pthread_join(thread_ids_red[i], NULL);
  }

  // Combine local outputs into the final output kvlist
  for (int i = 0; i < (int)num_reducer; i++) {
    if (thread_args_red[i].kvlist_out != NULL) {
      kvlist_extend(output, thread_args_red[i].kvlist_out);
      kvlist_free(&thread_args_red[i].kvlist_out);
    }
    kvlist_free(&kv_lists4[i]);
  }
  // free(intArr);
  free(kv_lists4);

  for (int i = 0; i < uniqueCount; ++i) {
    kvlist_free(&kv_lists3[i]);
  }
  free(kv_lists3);
}
