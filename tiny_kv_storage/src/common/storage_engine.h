// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <parallel_hashmap/phmap.h>

namespace tiny_kv {

using KVMap = phmap::parallel_flat_hash_map<std::string, std::string>;

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
  KVMap data_;
  std::string file_path_;
};

inline std::unique_ptr<StorageEngine>
CreateStorageEngine(const std::string &engine_type = "memory",
                    const std::string &file_path = "") {
  if (engine_type == "memory") {
    return std::make_unique<MemoryStorage>();
  } else {
    return std::make_unique<FileStorage>(file_path);
  }
}

} // namespace tiny_kv
