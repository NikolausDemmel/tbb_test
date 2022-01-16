#include <glog/logging.h>
#include <tbb/blocked_range.h>
#include <tbb/enumerable_thread_specific.h>
#include <tbb/global_control.h>
#include <tbb/parallel_for.h>
#include <tbb/task_arena.h>

#include <iostream>
#include <numeric>
#include <thread>

#include "tbb_utils.hpp"

void use_and_count_threads() {
  LOG(INFO) << "> entering use_and_count_threads";
  rootba::TbbConcurrencyObserver co;

  std::vector<int> foo;
  foo.resize(1000000);

  tbb::enumerable_thread_specific<int> tls(0);
  auto body = [&](const tbb::blocked_range<size_t>& range) {
    for (size_t r = range.begin(); r != range.end(); ++r) {
      tls.local() += 1;
    }
  };

  tbb::blocked_range<size_t> range(0, foo.size());
  tbb::parallel_for(range, body);

  int sum = std::accumulate(tls.begin(), tls.end(), 0);
  // LOG(INFO) << "sum: " << sum;
  //  for (int i : tls) {
  //    std::cout << i << ", ";
  //  }
  //  std::cout << std::endl;

  LOG(INFO) << "> tls size: " << tls.size();
  LOG(INFO) << "> current concurrency: " << co.get_current_concurrency();
  LOG(INFO) << "> peak concurrency:    " << co.get_peak_concurrency();

  LOG(INFO) << "> tbb::this_task_arena::max_concurrency(): "
            << tbb::this_task_arena::max_concurrency();
  LOG(INFO) << "tbb::global_control::max_allowed_parallelism: "
            << tbb::global_control::active_value(
                   tbb::global_control::max_allowed_parallelism);

  LOG(INFO) << "> finished use_and_count_threads";
}

int main(int /*argc*/, char** argv) {
  FLAGS_logtostderr = true;
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();

  LOG(INFO) << "std::thread::hardware_concurrency(): "
            << std::thread::hardware_concurrency();
  LOG(INFO) << "tbb::this_task_arena::max_concurrency(): "
            << tbb::this_task_arena::max_concurrency();
  LOG(INFO) << "tbb::global_control::max_allowed_parallelism: "
            << tbb::global_control::active_value(
                   tbb::global_control::max_allowed_parallelism);

  {
    LOG(INFO) << "create task area";
    tbb::task_arena task_arena(3);

    LOG(INFO) << "execute normal";
    use_and_count_threads();

    LOG(INFO) << "execute in task area";
    task_arena.execute([] { use_and_count_threads(); });

    LOG(INFO) << "set global limit";
    rootba::ScopedTbbThreadLimit scoped_limit(4);

    LOG(INFO) << "execute normal";
    use_and_count_threads();

    LOG(INFO) << "execute in task area";
    task_arena.execute([] { use_and_count_threads(); });

    LOG(INFO) << "final info (in scope of global limit)";
    LOG(INFO) << "tbb::this_task_arena::max_concurrency(): "
              << tbb::this_task_arena::max_concurrency();
    LOG(INFO) << "tbb::global_control::max_allowed_parallelism: "
              << tbb::global_control::active_value(
                     tbb::global_control::max_allowed_parallelism);
  }

  LOG(INFO) << "final info (out of scope of global limit)";
  LOG(INFO) << "tbb::this_task_arena::max_concurrency(): "
            << tbb::this_task_arena::max_concurrency();
  LOG(INFO) << "tbb::global_control::max_allowed_parallelism: "
            << tbb::global_control::active_value(
                   tbb::global_control::max_allowed_parallelism);

  return 0;
}

// Conclusions:
//
// 1. std::thread::hardware_concurrency() seems to give hardware cpu / thread
//    count independet of any cgroup / limits for the current process.
// 2. In contrast, tbb::this_task_arena::max_concurrency() /
//    tbb::global_control::active_value(
//    tbb::global_control::max_allowed_parallelism) seems to respect process
//    limits set by SLURM and only return the cpu / thread count usable by this
//    process.
// 3. Without further restrcitions, both max_concurrency() and
//    max_allowed_parallelism return the same value.
// 4. max_allowed_parallelism can be set by global control; max_concurrency()
//    can be set by executing in a custom task_arena, but they don't affect each
//    other.
// 5. The effective parallelism is min(max_concurrency,
//    max_allowed_parallelism).
//
// Running this on SLURM with 11 cpus assigned gives:
//
// clang-format off
//I0115 01:52:04.531363 1377109 thread_test.cpp:56] std::thread::hardware_concurrency(): 36
//I0115 01:52:04.531679 1377109 thread_test.cpp:58] tbb::this_task_arena::max_concurrency(): 11
//I0115 01:52:04.531716 1377109 thread_test.cpp:60] tbb::global_control::max_allowed_parallelism: 11
//I0115 01:52:04.531721 1377109 thread_test.cpp:65] create task area
//I0115 01:52:04.531724 1377109 thread_test.cpp:68] execute normal
//I0115 01:52:04.531728 1377109 thread_test.cpp:15] > entering use_and_count_threads
//I0115 01:52:04.547863 1377109 thread_test.cpp:38] > tls size: 11
//I0115 01:52:04.547879 1377109 thread_test.cpp:39] > current concurrency: 11
//I0115 01:52:04.547883 1377109 thread_test.cpp:40] > peak concurrency:    11
//I0115 01:52:04.547902 1377109 thread_test.cpp:42] > tbb::this_task_arena::max_concurrency(): 11
//I0115 01:52:04.547905 1377109 thread_test.cpp:44] tbb::global_control::max_allowed_parallelism: 11
//I0115 01:52:04.547909 1377109 thread_test.cpp:48] > finished use_and_count_threads
//I0115 01:52:04.548146 1377109 thread_test.cpp:71] execute in task area
//I0115 01:52:04.548168 1377109 thread_test.cpp:15] > entering use_and_count_threads
//I0115 01:52:04.581773 1377109 thread_test.cpp:38] > tls size: 3
//I0115 01:52:04.581780 1377109 thread_test.cpp:39] > current concurrency: 3
//I0115 01:52:04.581782 1377109 thread_test.cpp:40] > peak concurrency:    3
//I0115 01:52:04.581784 1377109 thread_test.cpp:42] > tbb::this_task_arena::max_concurrency(): 3
//I0115 01:52:04.581785 1377109 thread_test.cpp:44] tbb::global_control::max_allowed_parallelism: 11
//I0115 01:52:04.581787 1377109 thread_test.cpp:48] > finished use_and_count_threads
//I0115 01:52:04.581809 1377109 thread_test.cpp:74] set global limit
//I0115 01:52:04.581812 1377109 thread_test.cpp:77] execute normal
//I0115 01:52:04.581816 1377109 thread_test.cpp:15] > entering use_and_count_threads
//I0115 01:52:04.608183 1377109 thread_test.cpp:38] > tls size: 4
//I0115 01:52:04.608189 1377109 thread_test.cpp:39] > current concurrency: 4
//I0115 01:52:04.608191 1377109 thread_test.cpp:40] > peak concurrency:    4
//I0115 01:52:04.608193 1377109 thread_test.cpp:42] > tbb::this_task_arena::max_concurrency(): 11
//I0115 01:52:04.608196 1377109 thread_test.cpp:44] tbb::global_control::max_allowed_parallelism: 4
//I0115 01:52:04.608213 1377109 thread_test.cpp:48] > finished use_and_count_threads
//I0115 01:52:04.608217 1377109 thread_test.cpp:80] execute in task area
//I0115 01:52:04.608220 1377109 thread_test.cpp:15] > entering use_and_count_threads
//I0115 01:52:04.640399 1377109 thread_test.cpp:38] > tls size: 3
//I0115 01:52:04.640404 1377109 thread_test.cpp:39] > current concurrency: 3
//I0115 01:52:04.640406 1377109 thread_test.cpp:40] > peak concurrency:    3
//I0115 01:52:04.640408 1377109 thread_test.cpp:42] > tbb::this_task_arena::max_concurrency(): 3
//I0115 01:52:04.640410 1377109 thread_test.cpp:44] tbb::global_control::max_allowed_parallelism: 4
//I0115 01:52:04.640411 1377109 thread_test.cpp:48] > finished use_and_count_threads
//I0115 01:52:04.640431 1377109 thread_test.cpp:83] final info (in scope of global limit)
//I0115 01:52:04.640434 1377109 thread_test.cpp:84] tbb::this_task_arena::max_concurrency(): 11
//I0115 01:52:04.640435 1377109 thread_test.cpp:86] tbb::global_control::max_allowed_parallelism: 4
//I0115 01:52:04.640441 1377109 thread_test.cpp:91] final info (out of scope of global limit)
//I0115 01:52:04.640444 1377109 thread_test.cpp:92] tbb::this_task_arena::max_concurrency(): 11
//I0115 01:52:04.640446 1377109 thread_test.cpp:94] tbb::global_control::max_allowed_parallelism: 11
//
// clang-format on
