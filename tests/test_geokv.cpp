#include <filesystem>
#include <fstream>
#include <string>

#include <gtest/gtest.h>

#include "config.hpp"
#include "lsm/lsmstore.hpp"
#include "lsm/memtable.hpp"

namespace fs = std::filesystem;

namespace {

fs::path FreshDir(const std::string& name) {
  auto dir = fs::temp_directory_path() / ("geokv_test_" + name);
  fs::remove_all(dir);
  fs::create_directories(dir);
  return dir;
}

void Configure(const fs::path& dir, size_t flush_threshold = 128,
               size_t index_frequency = 2) {
  kvstore::KVConfig::Instance().SetDataDir(dir.string());
  kvstore::KVConfig::Instance().SetLSMFlushThreshold(flush_threshold);
  kvstore::KVConfig::Instance().SetIndexFrequency(index_frequency);
}

size_t CountFiles(const fs::path& dir, const std::string& extension) {
  if (!fs::exists(dir)) {
    return 0;
  }

  size_t count = 0;
  for (const auto& entry : fs::directory_iterator(dir)) {
    if (entry.is_regular_file() && entry.path().extension() == extension) {
      ++count;
    }
  }
  return count;
}

std::string ReadFile(const fs::path& path) {
  std::ifstream input(path);
  return std::string(std::istreambuf_iterator<char>(input),
                     std::istreambuf_iterator<char>());
}

TEST(RecordTest, RoundTripsValuesContainingEquals) {
  auto record = kvstore::Record::FromString("key=value=with=equals");

  EXPECT_EQ(record.Key(), "key");
  EXPECT_EQ(record.Value(), "value=with=equals");
  EXPECT_EQ(record.ToString(), "key=value=with=equals");
}

TEST(MemtableTest, StoresOverwritesAndFreezes) {
  kvstore::Memtable memtable;
  memtable.Put(kvstore::Record("city", "sf"));
  memtable.Put(kvstore::Record("city", "oakland"));

  auto record = memtable.Get("city");
  ASSERT_TRUE(record.has_value());
  EXPECT_EQ(record->Value(), "oakland");
  EXPECT_EQ(memtable.Size(), 1U);

  auto frozen = memtable.ResetAndFreeze();
  EXPECT_TRUE(memtable.Empty());
  EXPECT_TRUE(frozen.IsFrozen());
  EXPECT_TRUE(frozen.Get("city").has_value());
  EXPECT_THROW(frozen.Put(kvstore::Record("x", "y")), std::runtime_error);
}

TEST(LSMKVStoreTest, ReplaysUnflushedWritesFromWal) {
  auto dir = FreshDir("wal_replay");
  Configure(dir, 10);

  {
    kvstore::LSMKVStore store;
    store.Set(kvstore::Record("alpha", "1"));
    store.Set(kvstore::Record("beta", "2"));
  }

  {
    kvstore::LSMKVStore store;
    auto alpha = store.Get("alpha");
    auto beta = store.Get("beta");
    ASSERT_TRUE(alpha.has_value());
    ASSERT_TRUE(beta.has_value());
    EXPECT_EQ(alpha->Value(), "1");
    EXPECT_EQ(beta->Value(), "2");
  }
}

TEST(LSMKVStoreTest, FlushPersistsToSSTableAndClearsWal) {
  auto dir = FreshDir("flush_sstable");
  Configure(dir, 2);

  {
    kvstore::LSMKVStore store;
    store.Set(kvstore::Record("a", "1"));
    store.Set(kvstore::Record("b", "2"));
    EXPECT_EQ(CountFiles(dir / "sst", ".sst"), 1U);
    EXPECT_EQ(ReadFile(dir / "wal" / "wal.log"), "");
  }

  {
    kvstore::LSMKVStore store;
    auto a = store.Get("a");
    auto b = store.Get("b");
    ASSERT_TRUE(a.has_value());
    ASSERT_TRUE(b.has_value());
    EXPECT_EQ(a->Value(), "1");
    EXPECT_EQ(b->Value(), "2");
  }
}

TEST(LSMKVStoreTest, NewerSSTableWins) {
  auto dir = FreshDir("newer_sstable_wins");
  Configure(dir, 1);

  {
    kvstore::LSMKVStore store;
    store.Set(kvstore::Record("same", "old"));
    store.Set(kvstore::Record("same", "new"));
  }

  {
    kvstore::LSMKVStore store;
    auto record = store.Get("same");
    ASSERT_TRUE(record.has_value());
    EXPECT_EQ(record->Value(), "new");
  }
}

TEST(LSMKVStoreTest, CompactionMergesSSTables) {
  auto dir = FreshDir("compaction");
  Configure(dir, 1);

  {
    kvstore::LSMKVStore store;
    for (int i = 0; i < 6; ++i) {
      store.Set(kvstore::Record("k" + std::to_string(i),
                                "v" + std::to_string(i)));
    }
    EXPECT_LE(CountFiles(dir / "sst", ".sst"), 4U);
  }

  {
    kvstore::LSMKVStore store;
    for (int i = 0; i < 6; ++i) {
      auto record = store.Get("k" + std::to_string(i));
      ASSERT_TRUE(record.has_value());
      EXPECT_EQ(record->Value(), "v" + std::to_string(i));
    }
  }
}

}  // namespace
