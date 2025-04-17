// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#include "storage_engine.h"
#include <filesystem>
#include <gtest/gtest.h>
#include <memory>

namespace tiny_kv {
TEST(MemoryStorageTest, BasicOperations) {
  auto storage = std::make_unique<MemoryStorage>();

  // test `Put` and `Get`
  EXPECT_TRUE(storage->Put("key1", "value1"));

  auto value1 = storage->Get("key1");
  EXPECT_TRUE(value1.has_value());
  EXPECT_EQ(*value1, "value1");

  // get missing key
  auto missing_value = storage->Get("missing");
  EXPECT_FALSE(missing_value.has_value());

  // test `Delete`
  EXPECT_TRUE(storage->Delete("key1"));
  EXPECT_FALSE(storage->Get("key1").has_value());

  // delete missing key
  EXPECT_FALSE(storage->Delete("key1"));
}

TEST(MemoryStorageTest, BatchOperations) {
  auto storage = std::make_unique<MemoryStorage>();

  EXPECT_TRUE(storage->Put("key1", "value1"));
  EXPECT_TRUE(storage->Put("key2", "value2"));
  EXPECT_TRUE(storage->Put("key3", "value3"));

  auto entries = storage->GetAllEntries();
  EXPECT_EQ(entries["key1"], "value1");
  EXPECT_EQ(entries["key2"], "value2");
  EXPECT_EQ(entries["key3"], "value3");

  EXPECT_TRUE(storage->Delete("key2"));
  entries = storage->GetAllEntries();
  EXPECT_EQ(entries.size(), 2);
  EXPECT_EQ(entries.count("key2"), 0);
}

TEST(FileStorageTest, PersistAndLoad) {
  const std::string test_file = "test.db";
  if (std::filesystem::exists(test_file)) {
    std::filesystem::remove(test_file);
  }

  {
    auto storage = std::make_unique<FileStorage>(test_file);

    EXPECT_TRUE(storage->Put("key1", "value1"));
    EXPECT_TRUE(storage->Put("key2", "value2"));

    auto value1 = storage->Get("key1");
    EXPECT_TRUE(value1.has_value());
    EXPECT_EQ(*value1, "value1");

    // `~FileStorage()` will call `~Persist()`
  }

  EXPECT_TRUE(std::filesystem::exists(test_file));

  {
    auto storage = std::make_unique<FileStorage>(test_file);

    // check "key1" "key2"
    auto value1 = storage->Get("key1");
    EXPECT_TRUE(value1.has_value());
    EXPECT_EQ(*value1, "value1");

    auto value2 = storage->Get("key2");
    EXPECT_TRUE(value2.has_value());
    EXPECT_EQ(*value2, "value2");

    // update
    EXPECT_TRUE(storage->Put("key3", "value3"));
    EXPECT_TRUE(storage->Delete("key1"));
  }

  {
    auto storage = std::make_unique<FileStorage>(test_file);

    // check update data
    auto value3 = storage->Get("key3");
    EXPECT_TRUE(value3.has_value());
    EXPECT_EQ(*value3, "value3");

    EXPECT_FALSE(storage->Get("key1").has_value());
  }

  std::filesystem::remove(test_file);
}

TEST(StorageEngineFactory, CreateEngines) {
  auto memory_storage = CreateStorageEngine();
  EXPECT_TRUE(memory_storage->Put("key", "value"));
  EXPECT_TRUE(memory_storage->Get("key").has_value());

  auto file_storage = CreateStorageEngine("file", "test.db");
  EXPECT_TRUE(file_storage->Put("key", "value"));
  EXPECT_TRUE(file_storage->Get("key").has_value());
  std::filesystem::remove("test.db");
}

} // namespace tiny_kv
