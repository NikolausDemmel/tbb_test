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
