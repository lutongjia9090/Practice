// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#include "storage_engine.h"
#include <filesystem>
#include <fstream>
#include <ios>

namespace tiny_kv {

/************************************************************************/
/* MemoryStorage */
/************************************************************************/
bool MemoryStorage::Put(const std::string &key, const std::string &value) {
  std::lock_guard<std::mutex> lock(mutex_);
  data_[key] = value;
  return true;
}

std::optional<std::string> MemoryStorage::Get(const std::string &key) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = data_.find(key);
  if (it != data_.end()) {
    return it->second;
  }

  return std::nullopt;
}

bool MemoryStorage::Delete(const std::string &key) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = data_.find(key);
  if (it != data_.end()) {
    data_.erase(it);
    return true;
  }

  return false;
}

KVMap MemoryStorage::GetAllEntries() {
  std::lock_guard<std::mutex> lock(mutex_);
  return data_;
}

/************************************************************************/
/* FileStorage */
/************************************************************************/
FileStorage::FileStorage(const std::string &file_path) : file_path_(file_path) {
  Load();
}

FileStorage::~FileStorage() { Persist(); }

bool FileStorage::Put(const std::string &key, const std::string &value) {
  std::lock_guard<std::mutex> lock(mutex_);
  data_[key] = value;
  return true;
}

std::optional<std::string> FileStorage::Get(const std::string &key) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = data_.find(key);
  if (it != data_.end()) {
    return it->second;
  }

  return std::nullopt;
}

bool FileStorage::Delete(const std::string &key) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = data_.find(key);
  if (it != data_.end()) {
    data_.erase(it);
    return true;
  }

  return false;
}

bool FileStorage::Load() {
  std::lock_guard<std::mutex> lock(mutex_);
  data_.clear();

  if (!std::filesystem::exists(file_path_)) {
    return true;
  }

  std::ifstream file(file_path_, std::ios::binary);
  if (!file) {
    return false;
  }

  size_t count = 0;
  file.read(reinterpret_cast<char *>(&count), sizeof(count));

  for (size_t i = 0; i < count && file.good(); ++i) {
    size_t key_length = 0;
    file.read(reinterpret_cast<char *>(&key_length), sizeof(key_length));
    std::string key(key_length, '\0');
    file.read(&key[0], key_length);

    size_t value_length = 0;
    file.read(reinterpret_cast<char *>(&value_length), sizeof(value_length));
    std::string value(value_length, '\0');
    file.read(&value[0], value_length);

    data_[key] = value;
  }

  return file.good() || file.eof();
}

bool FileStorage::Persist() {
  std::lock_guard<std::mutex> lock(mutex_);
  std::ofstream file(file_path_, std::ios::binary);
  if (!file) {
    return false;
  }

  size_t count = data_.size();
  file.write(reinterpret_cast<char *>(&count), sizeof(count));

  for (const auto & [ key, value ] : data_) {
    size_t key_length = key.length();
    file.write(reinterpret_cast<char *>(&key_length), sizeof(key_length));
    file.write(key.data(), key_length);

    size_t value_length = value.length();
    file.write(reinterpret_cast<char *>(&value_length), sizeof(value_length));
    file.write(value.data(), value_length);
  }

  return file.good();
}

KVMap FileStorage::GetAllEntries() {
  std::lock_guard<std::mutex> lock(mutex_);
  return data_;
}

} // namespace tiny_kv
