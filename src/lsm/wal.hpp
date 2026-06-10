#pragma once

#include <atomic>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>

#include "../common/record.hpp"
#include "../config.hpp"

namespace kvstore {

/**
 * Write-Ahead Log (WAL) implementation.
 * Note: This class is NOT thread-safe.
 * TODO: Implement rotation and cleanup of old WAL files when we save SSTs.
 */
class WriteAheadLogger {
 public:
  WriteAheadLogger() {
    auto& config = KVConfig::Instance();
    const auto wal_dir = config.GetDataDir() + "/wal";

    if (!std::filesystem::exists(wal_dir)) {
      std::filesystem::create_directories(wal_dir);
    }

    path_ = wal_dir + "/wal.log";

    wal_ =
        std::make_unique<std::ofstream>(path_, std::ios::app | std::ios::out);
    if (!wal_->is_open()) {
      throw std::runtime_error("Failed to open WAL file");
    }
  }

  ~WriteAheadLogger() {
    Close();
    CloseReader();
  }

  void Write(const Record& record) {
    *wal_ << record << "\n";
  }

  void Flush() {
    if (wal_ && wal_->is_open()) {
      wal_->flush();
    }
  }

  void Close() {
    if (wal_ && wal_->is_open()) {
      wal_->close();
    }
  }

  void StartReader() {
    if (wal_reader_ && wal_reader_->is_open()) {
      wal_reader_->close();
    }
    wal_reader_ = std::make_unique<std::ifstream>(path_);
    if (!wal_reader_->is_open()) {
      throw std::runtime_error("Failed to open WAL file for reading");
    }

    wal_reader_->seekg(0, std::ios::beg);
  }

  void CloseReader() {
    if (wal_reader_ && wal_reader_->is_open()) {
      wal_reader_->close();
    }
  }

  bool HasNext() {
    return wal_reader_ &&
           wal_reader_->peek() != std::ifstream::traits_type::eof();
  }

  std::optional<Record> ReadNext() {
    if (!wal_reader_ || !wal_reader_->is_open()) {
      return std::nullopt;
    }

    std::string line;
    if (std::getline(*wal_reader_, line)) {
      return Record::FromString(line);
    }
    return std::nullopt;
  }

 private:
  std::string path_;
  std::unique_ptr<std::ofstream> wal_;
  std::unique_ptr<std::ifstream> wal_reader_;
};
}  // namespace kvstore