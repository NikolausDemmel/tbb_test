
#include <tbb/concurrent_unordered_map.h>
//#include <tbb/concurrent_map.h>

#include <iostream>

int main() {
  tbb::concurrent_unordered_map<int, int> map;
  // tbb::concurrent_map<int, int> map;

  // map[1] = 2;

  auto range = map.range();

  std::cout << range.empty() << std::endl;
}
