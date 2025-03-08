#include <mutex>
#include <stdint.h>
#include <thread>

#pragma once

namespace checktex
{

class Mutex
{
public:
  Mutex();
  Mutex(const char* name);
  ~Mutex();

  void lock();
  void unlock();

private:
#ifndef NDEBUG
  const uint64_t id_;
  const char* name_;
#endif
  std::recursive_mutex mutex_;

};

void PruneTree();
void PrintTree();
void PrintMutexes();
void PrintPotentialDeadlocks();

}
