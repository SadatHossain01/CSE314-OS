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
extern sem_t printing_mutex;
extern vector<Student> students;
extern vector<P_STATE> printers;
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
  // cerr << "Thread has been created for student id " << std.student_id <<
  // endl;

  int pid = sid % N_PRINTER + 1;
  long wait_time = rnd.next();
  sleep(wait_time);
  obtain_printer(students[sid - 1], pid);
  cout << "Student " << sid << " arrives at print station " << pid
       << " at time " << calculate_time() << endl;
  sleep(PRINTING_TIME);
  cout << "Student " << sid << " leaves print station " << pid << " at time "
       << calculate_time() << endl;
  leave_printer(students[sid - 1], pid);

  // cerr << "Exiting thread for student id " << std.student_id << endl;
  return NULL;
}

Group::Group(int from, int to) {
  this->from = from;
  this->to = to;
  this->group_id = (from - 1) / SZ_GROUP + 1;
  this->group_leader = to;
}

Random::Random(int mean) {
  std::random_device rd;
  this->generator = std::mt19937(rd());
  this->distribution = std::poisson_distribution<int>(mean);
}

long Random::next() { return this->distribution(this->generator); }

void obtain_printer(Student &st, int pid) {
  sem_wait(&printing_mutex);
  st.state = Student::IDLE;
  test(st, pid);
  sem_post(&printing_mutex);
  sem_wait(&st.semaphore);
}
void leave_printer(Student &st, int pid) {
  sem_wait(&printing_mutex);
  st.state = Student::DONE;
  printers[pid - 1] = P_STATE::IDLE;

  int sid = (pid + N_PRINTER - 2) % N_PRINTER + 1;
  while (sid <= N_STUDENT) {  // groupmates first
    if (students[sid - 1].group_id == st.group_id &&
        students[sid - 1].state == Student::IDLE)
      test(students[sid - 1], pid);
    sid += N_PRINTER;
  }

  sid = (pid + N_PRINTER - 2) % N_PRINTER + 1;
  while (sid <= N_STUDENT) {
    if (students[sid - 1].state == Student::IDLE) test(students[sid - 1], pid);
    sid += N_PRINTER;
  }

  sem_post(&printing_mutex);
}

void test(Student &student, int pid) {
  // cerr << "Testing for student " << student.student_id << " and printer " <<
  // pid
  //      << " S " << (student.state == Student::IDLE ? "IDLE" : "BUSY") << " P
  //      "
  //      << (printers[pid - 1] == P_STATE::IDLE ? "IDLE" : "BUSY") << endl;
  if (student.state == Student::IDLE && printers[pid - 1] == P_STATE::IDLE) {
    student.state = Student::PRINTING;
    printers[pid - 1] = P_STATE::BUSY;
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