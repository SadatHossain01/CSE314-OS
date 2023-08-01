#include <chrono>
#include <iostream>
#include <random>
#include <vector>

#include "ipc.h"

using namespace std;

int N_PRINTER = 4, N_BINDING_STATION = 2;
int N_GROUP;

int N_STUDENT, SZ_GROUP, PRINTING_TIME, BINDING_TIME, RW_TIME;

vector<Student> students;
vector<Printer> printers;
vector<Group> groups;
sem_t printing_mutex;
sem_t bs_semaphore;  // binding station

Random rnd(2);
chrono::high_resolution_clock::time_point start_time;

void init_semaphores() {
  sem_init(&printing_mutex, 0, 1);
  sem_init(&bs_semaphore, 0, N_BINDING_STATION);
}

int main() {
  start_time = chrono::high_resolution_clock::now();
  init_semaphores();

  cin >> N_STUDENT >> SZ_GROUP >> PRINTING_TIME >> BINDING_TIME >> RW_TIME;
  N_GROUP = (N_STUDENT + SZ_GROUP - 1) / SZ_GROUP;

  for (int i = 1; i <= N_PRINTER; i++) {
    printers.push_back(Printer(i));
  }
  for (int i = 1; i <= N_STUDENT; i++) {
    students.push_back(Student(i));
  }
  for (int i = 1; i <= N_GROUP; i++) {
    int to = min(i * SZ_GROUP, N_STUDENT);
    int from = to - SZ_GROUP + 1;
    groups.push_back(Group(from, to));
  }
  for (auto &student : students) {
    student.start_thread();
  }

  for (auto &group : groups) {
    group.start_thread();
  }

  for (auto &group : groups) {
    pthread_join(group.thread, NULL);
  }
}