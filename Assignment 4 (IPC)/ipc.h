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

struct Group {
 public:
  int group_leader;
  int from, to;
  int group_id;

  Group(int from, int to);
};

enum P_STATE { IDLE, BUSY };

struct Random {
 private:
  std::mt19937 generator;
  std::poisson_distribution<int> distribution;

 public:
  Random(int mean);
  long next();
};

int64_t calculate_time();
void *student_thread(void *student);
void obtain_printer(Student& st, int pid);
void leave_printer(Student& st, int pid);
void test(Student &st, int pid);

#endif