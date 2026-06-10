#pragma once

#include <iostream>
#include <string>
#include <thread>

#include "../common/kvstore.hpp"
#include "../common/record.hpp"
#include "../config.hpp"
#include "memtable.hpp"
#include "wal.hpp"

namespace kvstore {

class LSMKVStore : public IKVStore {
 public:
  explicit LSMKVStore() {
    auto& config = KVConfig::Instance();
    data_dir_ = config.GetDataDir();
    flush_threshold_ = config.GetLSMFlushThreshold();

    loadSSTables();
    loadFromWAL();
    compaction_thread_ = std::thread(&LSMKVStore::runCompactionLoop, this);
  }

  ~LSMKVStore() = default;

  bool Set(const Record& record) override {
    std::lock_guard lock(mutex_);

    // Write to WAL
    wal_.Write(record);
    wal_.Flush();

    // Update memtable
    memtable_.Put(record);

    // // Check if we need to flush to SST
    // if (memtable_.Size() >= flush_threshold_) {
    //   Flush();
    // }

    return true;
  }

  std::optional<Record> Get(const std::string& key) override {
    std::lock_guard lock(mutex_);

    // 1. Check memtable
    auto record = memtable_.Get(key);
    if (record) {
      return record;
    }

    // 2. Check frozen memtable
    record = frozen_memtable_.Get(key);
    if (record) {
      return record;
    }

    // 3. Scan SSTables (newest first)
    // for (int i = static_cast<int>(sstable_counter_) - 1; i >= 0; --i) {
    //   std::string sst_path = data_dir_ + "/sst_" + std::to_string(i) + ".txt";
    //   std::ifstream sst_file(sst_path);
    //   if (!sst_file.is_open()) continue;

    //   std::string line;
    //   while (std::getline(sst_file, line)) {
    //     auto record = Record::FromString(line);
    //     if (record.Key() == key) {
    //       return record;
    //     }
    //   }
    // }

    return std::nullopt;
  }


  void Flush() override {};
  void Shutdown() override {};

 private:
  void loadFromWAL() {
    wal_.StartReader();
    while (wal_.HasNext()) {
      auto record = wal_.ReadNext();
      if (record) {
        memtable_.Put(std::move(*record));
      }
    }
    wal_.CloseReader();
  }

  void loadSSTables() {};
  // void flushToSST();
  void runCompactionLoop() {};
  // void stopCompaction();

 private:
  std::string data_dir_;
  size_t flush_threshold_;
  std::atomic<size_t> sstable_counter_;
  std::atomic<bool> stop_compaction_;
  std::thread compaction_thread_;

  std::mutex mutex_;

  WriteAheadLogger wal_;
  Memtable memtable_;
  Memtable frozen_memtable_;
};
}  // namespace kvstore