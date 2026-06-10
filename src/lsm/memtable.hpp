#pragma once
#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <utility>

#include "../common/record.hpp"
#include "../config.hpp"

namespace kvstore {

/**
 * Memtable: In memory LSM tree structure.
 *
 * TODO: Reassess the need of mutex_ in this class.
 */
class Memtable {
 public:
  Memtable() = default;

  Memtable(std::map<std::string, Record> &&initial_memtable, bool is_frozen)
      : memtable_(std::move(initial_memtable)), is_frozen_(is_frozen) {}

  ~Memtable() = default;

  template <typename T>
  void Put(T&& record) {
    if (is_frozen_) {
      throw std::runtime_error("Cannot modify a frozen memtable");
    }

    static_assert(
        std::is_base_of_v<Record, std::decay_t<T>>,
        "T must be a Record or derived from Record");

    // std::lock_guard<std::mutex> lock(mutex_);
    memtable_.insert_or_assign(record.Key(), std::forward<T>(record));
  }

  std::optional<Record> Get(const std::string &key) {
    // std::lock_guard<std::mutex> lock(mutex_);
    auto it = memtable_.find(key);
    if (it != memtable_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  Memtable ResetAndFreeze() {
    std::map<std::string, Record> old_map;
    {
    //   std::lock_guard<std::mutex> lock(mutex_);
      old_map = std::move(memtable_);
      memtable_ = std::map<std::string, Record>();
    }
    return Memtable(std::move(old_map), true);
  }

  bool IsFrozen() const { return is_frozen_; }

 private:
  std::map<std::string, Record> memtable_;
  bool is_frozen_ = false;
  mutable std::mutex mutex_;
};
}  // namespace kvstore