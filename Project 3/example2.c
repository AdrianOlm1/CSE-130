#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "dining.h"
#include "utils.h"

int main(void) {
  dining_t* d = dining_init(3);
  cleaning_t c1 = make_cleaning(1, d);
  cleaning_t c2 = make_cleaning(2, d);

  student_t s1 = make_student(1, d);
  student_t s2 = make_student(2, d);
  student_t s3 = make_student(3, d);
  student_t s4 = make_student(4, d);

  cleaning_enter(&c1);

  pthread_create(&s1.thread, NULL, student_enter, &s1);
  msleep(100);

  pthread_create(&s2.thread, NULL, student_enter, &s2);
  msleep(100);

  pthread_create(&s3.thread, NULL, student_enter, &s3);
  msleep(100);

  pthread_create(&s4.thread, NULL, student_enter, &s4);
  msleep(100);

  pthread_create(&c2.thread, NULL, cleaning_enter, &c2);
  msleep(100);

  cleaning_leave(&c1);

  pthread_join(c2.thread, NULL);

  cleaning_leave(&c2);
  pthread_join(s1.thread, NULL);

  student_leave(&s1);
  pthread_join(s2.thread, NULL);

  student_leave(&s2);

  pthread_join(s3.thread, NULL);

  student_leave(&s3);

  pthread_join(s4.thread, NULL);

  student_leave(&s4);

  dining_destroy(&d);
}
