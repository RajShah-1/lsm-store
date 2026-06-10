#pragma once
#include <atomic>
#include <filesystem>
#include <mutex>
#include <stdexcept>
#include <string>

namespace kvstore {
class KVConfig {
 public:
  static KVConfig &Instance() {
    static KVConfig instance;
    return instance;
  }

  void SetDataDir(const std::string &dir) {
    std::lock_guard<std::mutex> lock(mutex_);
    data_dir_ = dir;
  }

  std::string GetDataDir() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (data_dir_.empty()) {
      throw std::runtime_error("Data directory not set");
    }

    return data_dir_;
  }

  std::string GetWalDir() const { return (std::filesystem::path(GetDataDir()) / "wal").string(); }

  std::string GetSSTableDir() const { return (std::filesystem::path(GetDataDir()) / "sst").string(); }

  void SetLSMFlushThreshold(size_t threshold) {
    if (threshold == 0) {
      throw std::invalid_argument("Flush threshold must be greater than zero");
    }
    lsm_flush_threshold_.store(threshold);
  }

  size_t GetLSMFlushThreshold() const {
    if (lsm_flush_threshold_.load() == 0) {
      throw std::runtime_error("Flush threshold not set");
    }

    return lsm_flush_threshold_.load();
  }

  void SetIndexFrequency(size_t frequency) {
    if (frequency == 0) {
      throw std::invalid_argument("Index frequency must be greater than zero");
    }
    index_frequency_.store(frequency);
  }

  size_t GetIndexFrequency() const {
    if (index_frequency_.load() == 0) {
      throw std::runtime_error("Index frequency not set");
    }

    return index_frequency_.load();
  }

 private:
  KVConfig() : data_dir_(""), lsm_flush_threshold_(128), index_frequency_(4) {}
  ~KVConfig() = default;

  std::string data_dir_;
  std::atomic<size_t> lsm_flush_threshold_;
  std::atomic<size_t> index_frequency_;
  mutable std::mutex mutex_;
};
}  // namespace kvstore
