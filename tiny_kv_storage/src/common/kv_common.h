// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#pragma once

#include <string>

namespace tiny_kv {

enum OperationType {
  KGet,
  KPut,
  KDelete,
  Invalid,
};

struct Request {
  OperationType op;
  std::string key;
  std::string value;
};

struct Response {
  bool success;
  std::string message;
  std::string value;
};

#define KV_ASSERT(condition, message)                                          \
  do {                                                                         \
    if (!(condition)) {                                                        \
      fprintf(stderr,                                                          \
              "Assertion failed: %s\nFile: %s\nLine: %d\nMessage: %s\n",       \
              #condition, __FILE__, __LINE__, message);                        \
      std::abort();                                                            \
    }                                                                          \
  } while (0)

} // namespace tiny_kv
