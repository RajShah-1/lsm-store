#pragma once

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <map>
#include <mutex>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

#include "../common/kvstore.hpp"
#include "../common/record.hpp"
#include "../config.hpp"
#include "memtable.hpp"
#include "sstable.hpp"
#include "wal.hpp"

namespace kvstore {

class LSMKVStore : public IKVStore {
 public:
  LSMKVStore() {
    auto& config = KVConfig::Instance();
    data_dir_ = std::filesystem::path(config.GetDataDir());
    sstable_dir_ = std::filesystem::path(config.GetSSTableDir());
    flush_threshold_ = config.GetLSMFlushThreshold();
    index_frequency_ = config.GetIndexFrequency();

    std::filesystem::create_directories(data_dir_);
    std::filesystem::create_directories(sstable_dir_);

    LoadSSTables();
    LoadFromWAL();
  }

  ~LSMKVStore() override { LSMKVStore::Shutdown(); }

  bool Set(const Record& record) override {
    std::lock_guard lock(mutex_);

    wal_.Write(record);
    wal_.Flush();
    memtable_.Put(record);

    if (memtable_.Size() >= flush_threshold_) {
      FlushLocked();
    }

    return true;
  }

  std::optional<Record> Get(const std::string& key) override {
    std::lock_guard lock(mutex_);

    auto record = memtable_.Get(key);
    if (record) {
      return record;
    }

    for (auto & sstable : std::ranges::reverse_view(sstables_)) {
      record = sstable.Get(key);
      if (record) {
        return record;
      }
    }

    return std::nullopt;
  }

  void Flush() override {
    std::lock_guard lock(mutex_);
    FlushLocked();
  }

  void Shutdown() override {
    std::lock_guard lock(mutex_);
    wal_.Flush();
    wal_.Close();
  }

 private:
  void LoadFromWAL() {
    wal_.StartReader();
    while (wal_.HasNext()) {
      auto record = wal_.ReadNext();
      if (record) {
        memtable_.Put(std::move(*record));
      }
    }
    wal_.CloseReader();
  }

  void LoadSSTables() {
    sstables_ = SSTable::LoadAll(sstable_dir_);
    size_t next_id = 0;
    for (const auto& table : sstables_) {
      next_id = std::max(next_id, table.Id() + 1);
    }
    next_sstable_id_.store(next_id);
  }

  void FlushLocked() {
    if (memtable_.Empty()) {
      return;
    }

    auto frozen = memtable_.ResetAndFreeze();
    auto table = SSTable::Write(sstable_dir_, next_sstable_id_.fetch_add(1),
                                frozen.Records(), index_frequency_);
    sstables_.push_back(std::move(table));
    wal_.Reset();
    CompactIfNeeded();
  }

  void CompactIfNeeded() {
    constexpr size_t kMaxSSTablesBeforeCompaction = 4;
    if (sstables_.size() <= kMaxSSTablesBeforeCompaction) {
      return;
    }

    std::map<std::string, Record> merged;
    for (const auto& table : sstables_) {
      for (auto& [key, record] : table.ReadAll()) {
        merged.insert_or_assign(key, std::move(record));
      }
    }

    auto compacted = SSTable::Write(sstable_dir_, next_sstable_id_.fetch_add(1),
                                    merged, index_frequency_);
    for (const auto& table : sstables_) {
      std::filesystem::remove(table.DataPath());
      std::filesystem::remove(table.IndexPath());
    }
    sstables_.clear();
    sstables_.push_back(std::move(compacted));
  }

 private:
  std::filesystem::path data_dir_;
  std::filesystem::path sstable_dir_;
  size_t flush_threshold_ = 0;
  size_t index_frequency_ = 0;
  std::atomic<size_t> next_sstable_id_{0};

  std::mutex mutex_;

  WriteAheadLogger wal_;
  Memtable memtable_;
  std::vector<SSTable> sstables_;
};
}  // namespace kvstore
