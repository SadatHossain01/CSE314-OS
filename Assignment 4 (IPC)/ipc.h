#ifndef IPC_H
#define IPC_H

#include <pthread.h>
#include <semaphore.h>

#include <random>
using namespace std;

struct Student {
 public:
  int group_id;
  int student_id;
  pthread_t thread;
  sem_t semaphore;
  enum State { IDLE, PRINTING, DONE } state;

  Student(int sid);
  void start_thread();
};

struct Printer {
 public:
  int printer_id;
  enum State { IDLE, BUSY } state;

  Printer(int pid);
};

struct Group {
 public:
  int group_leader;
  int from, to;
  int group_id;

  Group(int from, int to);
};

struct Random {
 private:
  std::mt19937 generator;
  std::poisson_distribution<int> distribution;

 public:
  Random(int mean = 5);
  long next();
};

struct Stuff {
 public:
  Random rnd;
  int stuff_id;
  pthread_t thread;

  Stuff(int sid);
  void start_thread();
};

int64_t calculate_time();
void *student_thread(void *arg);
void obtain_printer(Student &st, Printer &pr);
void leave_printer(Student &st, Printer &pr);
void test(Student &st, Printer &pr);
void *group_thread(void *arg);
void *stuff_thread(void *arg);

#endif