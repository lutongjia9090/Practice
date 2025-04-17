// Copyright 2024 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (lutongjia@163.com)
//

#include "thread_pool.h"
#include <blockingconcurrentqueue.h>
#include <chrono>
#include <iterator>
#include <thread>
#include <utility>

namespace practice {

/************************************************************************/
/* ThreadPool::Impl */
/************************************************************************/
class ThreadPool::Impl {
private:
  using BlockingQueue = moodycamel::BlockingConcurrentQueue<Task>;

private:
  volatile int stop_ = 0;
  std::vector<std::thread> threads_;
  BlockingQueue task_queue_;

public:
  ~Impl() { Stop(); }

private:
  void WorkThread() {
    Task task;
    for (;;) {
      if (stop_) {
        break;
      }

      while (task_queue_.wait_dequeue_timed(
          task, std::chrono::milliseconds(100))) { // hard code
        task();
      }
    }

    while (task_queue_.try_dequeue(task)) {
      task();
    }
  }

public:
  void Start(size_t n) {
    if (threads_.empty()) {
      stop_ = 0;
      threads_.reserve(n);
      for (size_t i = 0; i < n; ++i) {
        threads_.emplace_back([this]() { WorkThread(); });
      }
    }
  }

  void Stop() {
    if (!threads_.empty()) {
      stop_ = 1;
      for (std::thread &t : threads_) {
        t.join();
      }
      threads_.clear();
    }
  }

  void Post(Task task) { task_queue_.enqueue(std::move(task)); }

  void Post(std::vector<Task> tasks) {
    task_queue_.enqueue_bulk(std::make_move_iterator(tasks.begin()),
                             std::distance(tasks.begin(), tasks.end()));
  }
};

/************************************************************************/
/* ThreadPool */
/************************************************************************/
ThreadPool::ThreadPool() : impl_(new Impl) {}

ThreadPool::~ThreadPool() { delete impl_; }

void ThreadPool::Start(size_t n) { impl_->Start(n); }

void ThreadPool::Stop() { impl_->Stop(); }

void ThreadPool::Post(Task task) { impl_->Post(std::move(task)); }

void ThreadPool::Post(std::vector<Task> tasks) {
  impl_->Post(std::move(tasks));
}

} // namespace practice
