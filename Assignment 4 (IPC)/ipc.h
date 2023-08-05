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
  enum State { IDLE, WAITING, PRINTING, DONE } state;

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
void print(const string &msg);
enum Output_Type {
  STUDENT_ARRIVAL,
  STUDENT_PRINTING_START,
  STUDENT_PRINTING_FINISH,
  GROUP_PRINTING_FINISH,
  GROUP_BINDING_START,
  GROUP_BINDING_FINISH,
  GROUP_SUBMISSION_START,
  GROUP_SUBMISSION_FINISH,
  STUFF_READING_START,
  STUFF_READING_FINISH
};
// id can be student_id, group_id, or stuff_id whichever relevant
// var is optional, may be printer_id or no_of_submissions
void print(Output_Type type, int id, int var = -1);

#endif