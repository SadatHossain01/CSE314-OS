#include "ipc.h"

#include <unistd.h>

#include <cassert>
#include <chrono>
#include <iostream>
#include <vector>

extern chrono::high_resolution_clock::time_point start_time;
extern Random rnd;

extern int N_PRINTER, N_STUDENT, SZ_GROUP, N_GROUP, PRINTING_TIME, BINDING_TIME, RW_TIME;

extern vector<Printer> printers;
extern vector<Group> groups;
extern vector<Student> students;
extern vector<Stuff> stuffs;

extern sem_t output_mutex;
extern sem_t printing_mutex;
extern sem_t bs_semaphore;

// Submission & Stuffs
extern int n_submissions, n_readers;
extern sem_t rc_mutex;          // rc_mutex controls access to shared_variable n_readers
extern sem_t submission_mutex;  // submission_mutex controls access to
                                // shared_variable n_submissions

Student::Student(int sid) {
  this->student_id = sid;
  this->group_id = (sid - 1) / SZ_GROUP + 1;
  this->state = IDLE;
  sem_init(&this->semaphore, 0, 0);
}

void Student::start_thread() {
  pthread_create(&this->thread, NULL, student_thread,
                 &this->student_id);  // will handle printing, leaders' ones will also handle
                                      // binding and submission
}

void *student_thread(void *arg) {
  int sid = *(int *)arg;

  int pid = sid % N_PRINTER + 1;
  long wait_time = rnd.next();
  sleep(wait_time);

  // printing (dining philosopher problem)
  print(STUDENT_ARRIVAL, sid, pid);
  obtain_printer(students[sid - 1], printers[pid - 1]);
  print(STUDENT_PRINTING_START, sid);
  sleep(PRINTING_TIME);
  print(STUDENT_PRINTING_FINISH, sid);
  leave_printer(students[sid - 1], printers[pid - 1]);

  Group group = groups[students[sid - 1].group_id - 1];
  int gid = group.group_id;
  if (sid != group.group_leader) return NULL;  // only group leader will do binding and submission

  for (int i = group.from; i <= group.to; i++) {
    if (i == sid) continue;                      // skip group leader
    pthread_join(students[i - 1].thread, NULL);  // wait for all members to
                                                 // finish printing
  }
  print(GROUP_PRINTING_FINISH, gid);

  // binding
  sem_wait(&bs_semaphore);
  print(GROUP_BINDING_START, gid);
  sleep(BINDING_TIME);
  print(GROUP_BINDING_FINISH, gid);
  sem_post(&bs_semaphore);

  // submission (writing)
  sem_wait(&submission_mutex);  // need exclusive access to write
  n_submissions++;
  print(GROUP_SUBMISSION_START, gid);
  sleep(RW_TIME);
  print(GROUP_SUBMISSION_FINISH, gid);
  sem_post(&submission_mutex);  // release exclusive access

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

Stuff::Stuff(int sid) {
  this->stuff_id = sid;
  this->rnd = Random(11);
}

void Stuff::start_thread() {
  pthread_create(&this->thread, NULL, stuff_thread,
                 &this->stuff_id);  // will read n_submissions once in a while
}

void *stuff_thread(void *arg) {
  int sid = *(int *)arg;
  Stuff stuff = stuffs[sid - 1];
  bool cont = true;

  while (cont) {
    sleep(stuff.rnd.next());

    // reading (readers-writers problem)
    sem_wait(&rc_mutex);
    n_readers++;
    if (n_readers == 1)
      sem_wait(&submission_mutex);  // need exclusive access for all readers together
    sem_post(&rc_mutex);
    print(STUFF_READING_START, sid, n_submissions);
    sleep(RW_TIME);
    if (n_submissions == N_GROUP) cont = false;
    print(STUFF_READING_FINISH, sid, n_submissions);
    sem_wait(&rc_mutex);
    n_readers--;
    if (n_readers == 0) sem_post(&submission_mutex);  // release exclusive access
    sem_post(&rc_mutex);
  }
  return NULL;
}

Random::Random(int mean) {
  std::random_device rd;
  this->generator = std::mt19937(rd());
  this->distribution = std::poisson_distribution<int>(mean);
}

long Random::next() { return this->distribution(this->generator); }

// Dining Philosopher Problem
void obtain_printer(Student &st, Printer &pr) {
  sem_wait(&printing_mutex);
  st.state = Student::WAITING;
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
    if (students[sid - 1].group_id == st.group_id && students[sid - 1].state == Student::WAITING)
      test(students[sid - 1], pr);
    sid += N_PRINTER;
  }

  sid = (pr.printer_id + N_PRINTER - 2) % N_PRINTER + 1;
  while (sid <= N_STUDENT) {  // then others
    if (students[sid - 1].state == Student::WAITING) test(students[sid - 1], pr);
    sid += N_PRINTER;
  }

  sem_post(&printing_mutex);
}
void test(Student &student, Printer &pr) {
  if (student.state == Student::WAITING && pr.state == Printer::IDLE) {
    student.state = Student::PRINTING;
    pr.state = Printer::BUSY;
    sem_post(&student.semaphore);
  }
}

int64_t calculate_time() {
  auto end_time = chrono::high_resolution_clock::now();
  auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();
  return duration;
}

void print(const string &msg) {
  sem_wait(&output_mutex);
  cout << msg << endl;
  sem_post(&output_mutex);
}

void print(Output_Type type, int id, int var) {
  int64_t time = calculate_time();

  sem_wait(&output_mutex);
  switch (type) {
    case STUDENT_ARRIVAL:
      assert(var != -1);
      cout << "Student " << id << " has arrived at print station " << var << " at time " << time
           << endl;
      break;
    case STUDENT_PRINTING_START:
      cout << "Student " << id << " has started printing at time " << time << endl;
      break;
    case STUDENT_PRINTING_FINISH:
      cout << "Student " << id << " has finished printing at time " << time << endl;
      break;
    case GROUP_PRINTING_FINISH:
      cout << "Group " << id << " has finished printing at time " << time << endl;
      break;
    case GROUP_BINDING_START:
      cout << "Group " << id << " has started binding at time " << time << endl;
      break;
    case GROUP_BINDING_FINISH:
      cout << "Group " << id << " has finished binding at time " << time << endl;
      break;
    case GROUP_SUBMISSION_START:
      cout << "Group " << id << " has started submitting the report at time " << time << endl;
      break;
    case GROUP_SUBMISSION_FINISH:
      cout << "Group " << id << " has submitted the report at time " << time << endl;
      break;
    case STUFF_READING_START:
      assert(var != -1);
      cout << "Stuff " << id << " has started reading the entry book at time " << time
           << ". No. of submission = " << var << endl;
      break;
    case STUFF_READING_FINISH:
      assert(var != -1);
      cout << "Stuff " << id << " has finished reading the entry book at time " << time
           << ". No. of submission = " << var << endl;
      break;
    default:
      assert(false);
      break;
  }
  sem_post(&output_mutex);
}