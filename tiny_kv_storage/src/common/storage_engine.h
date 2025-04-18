// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#pragma once

#include "cache.h"
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace tiny_kv {

using KVMap = std::unordered_map<std::string, std::string>;

constexpr size_t DEFAULT_CACHE_CAPACITY = 1000; // hard code

/************************************************************************/
/* StorageEngine */
/************************************************************************/
class StorageEngine {
public:
  virtual ~StorageEngine() = default;

  virtual bool Put(const std::string &key, const std::string &value) = 0;
  virtual std::optional<std::string> Get(const std::string &key) = 0;
  virtual bool Delete(const std::string &key) = 0;
  virtual KVMap GetAllEntries() = 0;
};

/************************************************************************/
/* MemoryStorage */
/************************************************************************/
class MemoryStorage : public StorageEngine {
public:
  bool Put(const std::string &key, const std::string &value) override;
  std::optional<std::string> Get(const std::string &key) override;
  bool Delete(const std::string &key) override;
  KVMap GetAllEntries() override;

private:
  std::mutex mutex_;
  KVMap data_;
};

/************************************************************************/
/* FileStorage */
/************************************************************************/
class FileStorage : public StorageEngine {
public:
  explicit FileStorage(const std::string &file_path);
  ~FileStorage() override;

  bool Put(const std::string &key, const std::string &value) override;
  std::optional<std::string> Get(const std::string &key) override;
  bool Delete(const std::string &key) override;
  KVMap GetAllEntries() override;
  bool Persist();

private:
  bool Load();

private:
  std::mutex mutex_;
  KVMap data_;
  std::string file_path_;
};

/************************************************************************/
/* CacheMemoryStorage */
/************************************************************************/
class CacheMemoryStorage : public StorageEngine {
public:
  explicit CacheMemoryStorage(size_t capacity = DEFAULT_CACHE_CAPACITY);

  bool Put(const std::string &key, const std::string &value) override;
  std::optional<std::string> Get(const std::string &key) override;
  bool Delete(const std::string &key) override;
  KVMap GetAllEntries() override;

private:
  std::mutex mutex_;
  KVMap data_;
  LRUCache<std::string, std::string> cache_;
};

inline std::unique_ptr<StorageEngine>
CreateStorageEngine(const std::string &engine_type = "memory",
                    const std::string &file_path = "", bool use_cache = true,
                    size_t cache_capacity = DEFAULT_CACHE_CAPACITY) {
  if (engine_type == "memory") {
    return std::make_unique<MemoryStorage>();
  } else if (engine_type == "memory" && use_cache) {
    return std::make_unique<CacheMemoryStorage>(cache_capacity);
  } else {
    return std::make_unique<FileStorage>(file_path);
  }
}

} // namespace tiny_kv
