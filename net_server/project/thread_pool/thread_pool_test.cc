// Copyright 2024 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (lutongjia@163.com)
//

#include "thread_pool.h"
#include <atomic>
#include <cstdint>
#include <gtest/gtest.h>

namespace practice {

class ThreadPoolTest : public testing::Test {
protected:
  static constexpr size_t N = 10000;
  static constexpr size_t THREAD = 5;
  const uint64_t SUM = (uint64_t)N * ((uint64_t)N - 1) / 2;

protected:
  ThreadPool pool;
};

TEST_F(ThreadPoolTest, all) {
  std::atomic<uint64_t> sum(0);
  pool.Start(THREAD);
  for (size_t i = 0; i < N; ++i) {
    pool.Post([&sum, i]() { sum.fetch_add(i, std::memory_order_relaxed); });
  }

  pool.Stop();

  EXPECT_EQ(sum, SUM);
}

} // namespace practice
