#include "ipc.h"

#include <unistd.h>

#include <chrono>
#include <iostream>
#include <vector>

extern const int N_PRINTER;
extern int N_STUDENT;
extern int SZ_GROUP;
extern int PRINTING_TIME, BINDING_TIME, RW_TIME;
extern Random rnd;
extern sem_t printing_mutex, bs_semaphore;
extern vector<Student> students;
extern vector<Printer> printers;
extern vector<Group> groups;
extern chrono::high_resolution_clock::time_point start_time;

Student::Student(int sid) {
  this->student_id = sid;
  this->group_id = (sid - 1) / SZ_GROUP + 1;
  this->state = IDLE;
  sem_init(&this->semaphore, 0, 0);
}

void Student::start_thread() {
  pthread_create(&this->thread, NULL, student_thread, &this->student_id);
}

void *student_thread(void *arg) {
  int sid = *(int *)arg;

  int pid = sid % N_PRINTER + 1;
  long wait_time = rnd.next();
  sleep(wait_time);
  obtain_printer(students[sid - 1], printers[pid - 1]);
  cout << "Student " << sid << " has arrived at print station " << pid
       << " at time " << calculate_time() << endl;
  sleep(PRINTING_TIME);
  cout << "Student " << sid << " has finished printing at time "
       << calculate_time() << endl;
  leave_printer(students[sid - 1], printers[pid - 1]);

  return NULL;
}

Printer::Printer(int pid) {
  this->printer_id = pid;
  this->state = IDLE;
}

Group::Group(int from, int to) {
  this->from = from;
  this->to = to;
  this->group_id = (from - 1) / SZ_GROUP + 1;
  this->group_leader = to;
}

void Group::start_thread() {
  pthread_create(&this->thread, NULL, group_thread, &this->group_id);
}

void *group_thread(void *arg) {
  int gid = *(int *)arg;
  Group group = groups[gid - 1];

  for (int i = group.from; i <= group.to; i++) {
    pthread_join(students[i - 1].thread, NULL);
  }

  cout << "Group " << gid << " has finished printing at time "
       << calculate_time() << endl;

  sem_wait(&bs_semaphore);
  cout << "Group " << gid << " has started binding at time " << calculate_time()
       << endl;
  sleep(BINDING_TIME);
  cout << "Group " << gid << " has finished binding at time "
       << calculate_time() << endl;
  sem_post(&bs_semaphore);

  return NULL;
}

Random::Random(int mean) {
  std::random_device rd;
  this->generator = std::mt19937(rd());
  this->distribution = std::poisson_distribution<int>(mean);
}

long Random::next() { return this->distribution(this->generator); }

void obtain_printer(Student &st, Printer &pr) {
  sem_wait(&printing_mutex);
  st.state = Student::IDLE;
  test(st, pr);
  sem_post(&printing_mutex);
  sem_wait(&st.semaphore);
}
void leave_printer(Student &st, Printer &pr) {
  sem_wait(&printing_mutex);
  st.state = Student::DONE;
  pr.state = Printer::IDLE;

  int sid = (pr.printer_id + N_PRINTER - 2) % N_PRINTER + 1;
  while (sid <= N_STUDENT) {  // groupmates first
    if (students[sid - 1].group_id == st.group_id &&
        students[sid - 1].state == Student::IDLE)
      test(students[sid - 1], pr);
    sid += N_PRINTER;
  }

  sid = (pr.printer_id + N_PRINTER - 2) % N_PRINTER + 1;
  while (sid <= N_STUDENT) {
    if (students[sid - 1].state == Student::IDLE) test(students[sid - 1], pr);
    sid += N_PRINTER;
  }

  sem_post(&printing_mutex);
}

void test(Student &student, Printer &pr) {
  if (student.state == Student::IDLE && pr.state == Printer::IDLE) {
    student.state = Student::PRINTING;
    pr.state = Printer::BUSY;
    sem_post(&student.semaphore);
  }
}

int64_t calculate_time() {
  auto end_time = chrono::high_resolution_clock::now();
  auto duration =
      chrono::duration_cast<chrono::milliseconds>(end_time - start_time)
          .count();
  return duration;
}