#include "ipc.h"

#include <unistd.h>

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
  obtain_printer(students[sid - 1], printers[pid - 1]);
  cout << "Student " << sid << " has arrived at print station " << pid << " at time "
       << calculate_time() << endl;
  sleep(PRINTING_TIME);
  cout << "Student " << sid << " has finished printing at time " << calculate_time() << endl;
  leave_printer(students[sid - 1], printers[pid - 1]);

  Group group = groups[students[sid - 1].group_id - 1];
  int gid = group.group_id;
  if (sid != group.group_leader) return NULL;  // only group leader will do binding and submission

  for (int i = group.from; i <= group.to; i++) {
    if (i == sid) continue;                      // skip group leader
    pthread_join(students[i - 1].thread, NULL);  // wait for all members to
                                                 // finish printing
  }
  cout << "Group " << gid << " has finished printing at time " << calculate_time() << endl;

  // binding
  sem_wait(&bs_semaphore);
  cout << "Group " << gid << " has started binding at time " << calculate_time() << endl;
  sleep(BINDING_TIME);
  cout << "Group " << gid << " has finished binding at time " << calculate_time() << endl;
  sem_post(&bs_semaphore);

  // submission (writing)
  sem_wait(&submission_mutex);  // need exclusive access to write
  n_submissions++;
  cout << "Group " << gid << " has submitted the report at time " << calculate_time() << endl;
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
    cout << "Stuff " << sid << " has started reading the entry book at time " << calculate_time()
         << ". No. of submission = " << n_submissions << endl;
    sleep(RW_TIME);
    if (n_submissions == N_GROUP) cont = false;
    cout << "Stuff " << sid << " has finished reading the entry book at time " << calculate_time()
         << ". No. of submission = " << n_submissions << endl;
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
    if (students[sid - 1].group_id == st.group_id && students[sid - 1].state == Student::IDLE)
      test(students[sid - 1], pr);
    sid += N_PRINTER;
  }

  sid = (pr.printer_id + N_PRINTER - 2) % N_PRINTER + 1;
  while (sid <= N_STUDENT) {  // then others
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
  auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();
  return duration;
}