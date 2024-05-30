#include "dining.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

typedef struct dining {
  // TODO: Add your variables here
  int capacity;

  sem_t semaphore;
  pthread_mutex_t mutex;
  pthread_mutex_t mutexStudent;
  pthread_cond_t condStudent;
  pthread_cond_t condClean;
  int studentIn;
  bool clean;
  bool needClean;
} dining_t;

dining_t *dining_init(int capacity) {
  // TODO: Initialize necessary variables
  dining_t *dining = malloc(sizeof(dining_t));
  dining->capacity = capacity;

  sem_init(&(dining->semaphore), 0, capacity);
  dining->studentIn = 0;
  pthread_mutex_init(&(dining->mutex), NULL);
  pthread_mutex_init(&(dining->mutexStudent), NULL);

  pthread_cond_init(&(dining->condStudent), NULL);
  pthread_cond_init(&(dining->condClean), NULL);
  dining->clean = dining->needClean = false;

  return dining;
}

void dining_destroy(dining_t **dining) {
  // TODO: Free dynamically allocated memory
  sem_destroy(&((*dining)->semaphore));
  pthread_mutex_destroy(&((*dining)->mutex));
  pthread_mutex_destroy(&((*dining)->mutexStudent));
  pthread_cond_destroy(&((*dining)->condStudent));
  pthread_cond_destroy(&((*dining)->condClean));

  free(*dining);
  *dining = NULL;
}

void dining_student_enter(dining_t *dining) {
  // TODO: Your code goes here
  pthread_mutex_lock(&(dining->mutexStudent));
  while (dining->needClean) {
    pthread_cond_wait(&(dining->condStudent), &(dining->mutexStudent));
  }
  pthread_mutex_unlock(&(dining->mutexStudent));

  sem_wait(&(dining->semaphore));

  pthread_mutex_lock(&(dining->mutex));

  while (dining->clean || dining->needClean) {
    pthread_cond_wait(&(dining->condStudent), &(dining->mutex));
  }

  ++(dining->studentIn);
  pthread_mutex_unlock(&(dining->mutex));
}

void dining_student_leave(dining_t *dining) {
  // TODO: Your code goes here
  pthread_mutex_lock(&(dining->mutex));
  sem_post(&(dining->semaphore));

  --(dining->studentIn);

  pthread_cond_signal(&(dining->condClean));

  pthread_mutex_unlock(&(dining->mutex));
}

void dining_cleaning_enter(dining_t *dining) {
  // TODO: Your code goes here

  pthread_mutex_lock(&(dining->mutex));
  dining->needClean = true;
  while ((dining->studentIn != 0) || dining->clean) {
    pthread_cond_wait(&(dining->condClean), &(dining->mutex));
  }
  dining->clean = true;
  dining->needClean = false;

  pthread_mutex_unlock(&(dining->mutex));
}

void dining_cleaning_leave(dining_t *dining) {
  // TODO: Your code goes here
  pthread_mutex_lock(&(dining->mutex));
  dining->clean = false;

  pthread_cond_signal(&(dining->condClean));

  pthread_cond_broadcast(&(dining->condStudent));

  pthread_mutex_unlock(&(dining->mutex));
}
