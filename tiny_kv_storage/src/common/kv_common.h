// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#pragma once

#include <string>
#include <vector>
#include <utility>

namespace tiny_kv {

enum OperationType {
  KGet,
  KPut,
  KDelete,
  KMultiGet,
  KMultiPut,
  KMultiDelete,
  Invalid,
};

struct KeyValuePair {
  std::string key;
  std::string value;
};

struct Request {
  OperationType op;
  std::string key;
  std::string value;
  std::vector<KeyValuePair> kvs;  // for multi-key operations
};

struct Response {
  bool success;
  std::string message;
  std::string value;
  std::vector<KeyValuePair> kvs;  // for multi-key operations
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
