// Copyright 2024 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (lutongjia@163.com)
//

#pragma once

#include <functional>
#include <stddef.h>
#include <vector>

namespace practice {

/************************************************************************/
/* ThreadPool */
/************************************************************************/
class ThreadPool {
public:
  using Task = std::function<void()>;

private:
  class Impl;
  Impl *const impl_;

public:
  ThreadPool();
  ~ThreadPool();

private:
  ThreadPool(const ThreadPool &) = delete;
  ThreadPool(ThreadPool &&) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;
  ThreadPool &operator=(ThreadPool &&) = delete;

public:
  void Start(size_t n);

  void Stop();

  void Post(Task task);

  void Post(std::vector<Task> tasks);
};

} // namespace practice
