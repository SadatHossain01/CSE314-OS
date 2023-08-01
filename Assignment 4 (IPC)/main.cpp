#include <chrono>
#include <iostream>
#include <random>
#include <vector>

#include "ipc.h"

using namespace std;

int N_PRINTER = 4;
int N_STUDENT, SZ_GROUP, PRINTING_TIME, BINDING_TIME, RW_TIME;
int N_GROUP;
vector<Student> students;
vector<P_STATE> printers(N_PRINTER, IDLE);
vector<Group> groups;
sem_t printing_mutex;

Random rnd(2);
chrono::high_resolution_clock::time_point start_time;

int main() {
  start_time = chrono::high_resolution_clock::now();
  sem_init(&printing_mutex, 0, 1);

  cin >> N_STUDENT >> SZ_GROUP >> PRINTING_TIME >> BINDING_TIME >> RW_TIME;
  N_GROUP = (N_STUDENT + SZ_GROUP - 1) / SZ_GROUP;

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

  while (1);
}