#include <chrono>
#include <iostream>
#include <random>
#include <vector>

#include "ipc.h"

using namespace std;

chrono::high_resolution_clock::time_point start_time;
Random rnd(2);

int N_PRINTER = 4, N_BINDING_STATION = 2, N_STUFFS = 2, N_GROUP;
int N_STUDENT, SZ_GROUP, PRINTING_TIME, BINDING_TIME, RW_TIME;

vector<Printer> printers;
vector<Student> students;
vector<Group> groups;
vector<Stuff> stuffs;

sem_t printing_mutex;
sem_t bs_semaphore;  // binding station

// Submission & Stuffs
int n_submissions = 0, n_readers = 0;
sem_t rc_mutex, submission_mutex;

void init_semaphores() {
  sem_init(&printing_mutex, 0, 1);
  sem_init(&bs_semaphore, 0, N_BINDING_STATION);
  sem_init(&rc_mutex, 0, 1);
  sem_init(&submission_mutex, 0, 1);
}

// g++ -std=c++14 -O3 -g -fsanitize=address ipc.cpp main.cpp && ./a.out
int main() {
  start_time = chrono::high_resolution_clock::now();
  init_semaphores();

  cin >> N_STUDENT >> SZ_GROUP >> PRINTING_TIME >> BINDING_TIME >> RW_TIME;
  N_GROUP = (N_STUDENT + SZ_GROUP - 1) / SZ_GROUP;

  for (int i = 1; i <= N_STUFFS; i++) stuffs.push_back(Stuff(i));
  for (int i = 1; i <= N_PRINTER; i++) printers.push_back(Printer(i));
  for (int i = 1; i <= N_STUDENT; i++) students.push_back(Student(i));
  for (int i = 1; i <= N_GROUP; i++) {
    int to = min(i * SZ_GROUP, N_STUDENT);
    int from = to - SZ_GROUP + 1;
    groups.push_back(Group(from, to));
  }

  for (auto &stuff : stuffs) {
    stuff.start_thread();  // initialize stuffs' reading threads
  }
  for (auto &student : students) {
    student.start_thread();
  }

  for (auto &group : groups) {
    pthread_join(students[group.group_leader - 1].thread, NULL);
  }
  for (auto &stuff : stuffs) {
    pthread_join(stuff.thread, NULL);
  }

  return 0;
}