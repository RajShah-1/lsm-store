#pragma once
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include "../common/record.hpp"

namespace kvstore {

// In-memory sorted write buffer. LSMKVStore owns synchronization.
class Memtable {
 public:
  using Table = std::map<std::string, Record>;

  Memtable() = default;

  explicit Memtable(Table &&initial_memtable, bool is_frozen = false)
      : memtable_(std::move(initial_memtable)), is_frozen_(is_frozen) {}

  ~Memtable() = default;

  template <typename T>
  void Put(T&& record) {
    if (is_frozen_) {
      throw std::runtime_error("Cannot modify a frozen memtable");
    }

    static_assert(std::is_same_v<Record, std::decay_t<T>>,
                  "T must be a Record");

    memtable_.insert_or_assign(record.Key(), std::forward<T>(record));
  }

  std::optional<Record> Get(const std::string &key) const {
    auto it = memtable_.find(key);
    if (it != memtable_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  Memtable ResetAndFreeze() {
    Table old_map = std::move(memtable_);
    memtable_ = Table();
    return Memtable(std::move(old_map), true);
  }

  void Clear() { memtable_.clear(); }

  bool Empty() const { return memtable_.empty(); }

  bool IsFrozen() const { return is_frozen_; }

  size_t Size() const { return memtable_.size(); }

  const Table& Records() const { return memtable_; }

 private:
  Table memtable_;
  bool is_frozen_ = false;
};
}  // namespace kvstore
