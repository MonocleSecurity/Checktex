#include <checktex.hpp>
#include <thread>

int main(int argc, char* argv[])
{
  checktex::Mutex m1;
  checktex::Mutex m2;
  checktex::Mutex m3;
  checktex::Mutex m4;
  checktex::Mutex m5;
  checktex::Mutex m6;
  checktex::Mutex m7;
  checktex::Mutex m8;
  checktex::Mutex m9;
  checktex::Mutex m10;
  std::thread([&m1, &m2, &m3, &m4, &m5, &m6, &m7, &m8, &m9, &m10]()
              {
                m10.lock(); m10.lock(); m10.unlock(); m10.unlock();
                m1.lock(); m2.lock(); m2.unlock(); m3.lock(); m3.unlock(); m1.unlock();
                m4.lock(); m5.lock(); m6.lock(); m6.unlock(); m7.lock(); m7.unlock(); m8.lock(); m8.unlock(); m5.unlock(); m4.unlock();
                m8.lock(); m9.lock(); m9.unlock(); m8.unlock();
                m1.lock(); m5.lock(); m6.lock(); m6.unlock(); m7.lock(); m7.unlock(); m8.lock(); m9.lock(); m9.unlock(); m8.unlock(); m5.unlock(); m1.unlock();
              }).join();
  std::thread([&m1, &m2, &m3, &m4, &m5, &m6, &m7, &m8, &m9]() { m1.lock(); m2.lock(); m2.unlock(); m1.unlock(); }).join();
  std::thread([&m1, &m2, &m3, &m4, &m5, &m6, &m7, &m8, &m9]() { m2.lock(); m1.lock(); m1.unlock(); m2.unlock(); }).join();
  // Create "potential" deadlock
  std::thread([&m1, &m2, &m3, &m4]() { m1.lock(); m2.lock(); m2.unlock(); m1.unlock();}).join();
  std::thread([&m1, &m2, &m3, &m4]() { m2.lock(); m3.lock(); m3.unlock(); m2.unlock();}).join();
  std::thread([&m1, &m2, &m3, &m4]() { m3.lock(); m4.lock(); m4.unlock(); m3.unlock();}).join();
  std::thread([&m1, &m2, &m3, &m4]() { m4.lock(); m1.lock(); m1.unlock(); m4.unlock();}).join();
  // Print
  checktex::PrintTree();
}
