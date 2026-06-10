#pragma once

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "../common/record.hpp"

namespace kvstore {

class SSTable {
 public:
  using Table = std::map<std::string, Record>;

  SSTable(size_t id, std::filesystem::path data_path,
          std::filesystem::path index_path)
      : id_(id),
        data_path_(std::move(data_path)),
        index_path_(std::move(index_path)) {
    LoadIndex();
  }

  size_t Id() const { return id_; }

  const std::filesystem::path& DataPath() const { return data_path_; }

  const std::filesystem::path& IndexPath() const { return index_path_; }

  static SSTable Write(const std::filesystem::path& dir, size_t id,
                       const Table& records, size_t index_frequency) {
    std::filesystem::create_directories(dir);

    auto data_path = dir / ("sst_" + std::to_string(id) + ".sst");
    auto index_path = dir / ("sst_" + std::to_string(id) + ".idx");
    auto tmp_data_path = data_path;
    auto tmp_index_path = index_path;
    tmp_data_path += ".tmp";
    tmp_index_path += ".tmp";

    std::ofstream data_file(tmp_data_path, std::ios::trunc);
    std::ofstream index_file(tmp_index_path, std::ios::trunc);
    if (!data_file.is_open() || !index_file.is_open()) {
      throw std::runtime_error("Failed to create SSTable files");
    }

    size_t counter = 0;
    for (const auto& [key, record] : records) {
      const auto offset = static_cast<size_t>(data_file.tellp());
      data_file << record.ToString() << "\n";
      if (counter % index_frequency == 0) {
        index_file << key << "=" << offset << "\n";
      }
      ++counter;
    }

    data_file.close();
    index_file.close();
    std::filesystem::rename(tmp_data_path, data_path);
    std::filesystem::rename(tmp_index_path, index_path);

    return SSTable(id, data_path, index_path);
  }

  static std::vector<SSTable> LoadAll(const std::filesystem::path& dir) {
    std::vector<SSTable> tables;
    if (!std::filesystem::exists(dir)) {
      return tables;
    }

    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".sst") {
        continue;
      }

      const auto id = ParseId(entry.path());
      const auto index_path = entry.path().parent_path() /
                              ("sst_" + std::to_string(id) + ".idx");
      if (std::filesystem::exists(index_path)) {
        tables.emplace_back(id, entry.path(), index_path);
      }
    }

    std::sort(tables.begin(), tables.end(),
              [](const SSTable& lhs, const SSTable& rhs) {
                return lhs.Id() < rhs.Id();
              });
    return tables;
  }

  std::optional<Record> Get(const std::string& key) const {
    std::ifstream data_file(data_path_);
    if (!data_file.is_open()) {
      throw std::runtime_error("Failed to open SSTable file");
    }

    const auto start = FindSearchOffset(key);
    data_file.seekg(static_cast<std::streamoff>(start), std::ios::beg);

    std::string line;
    while (std::getline(data_file, line)) {
      auto record = Record::FromString(line);
      if (record.Key() == key) {
        return record;
      }
      if (record.Key() > key) {
        break;
      }
    }
    return std::nullopt;
  }

  Table ReadAll() const {
    Table records;
    std::ifstream data_file(data_path_);
    if (!data_file.is_open()) {
      throw std::runtime_error("Failed to open SSTable file");
    }

    std::string line;
    while (std::getline(data_file, line)) {
      if (line.empty()) {
        continue;
      }
      auto record = Record::FromString(line);
      records.insert_or_assign(record.Key(), std::move(record));
    }
    return records;
  }

 private:
  static size_t ParseId(const std::filesystem::path& path) {
    const auto stem = path.stem().string();
    constexpr std::string_view prefix = "sst_";
    if (stem.rfind(prefix, 0) != 0) {
      throw std::runtime_error("Invalid SSTable filename");
    }
    return std::stoull(stem.substr(prefix.size()));
  }

  void LoadIndex() {
    sparse_index_.clear();
    std::ifstream index_file(index_path_);
    if (!index_file.is_open()) {
      throw std::runtime_error("Failed to open SSTable index file");
    }

    std::string line;
    while (std::getline(index_file, line)) {
      if (line.empty()) {
        continue;
      }
      auto pos = line.find('=');
      if (pos == std::string::npos) {
        throw std::runtime_error("Invalid SSTable index entry");
      }
      sparse_index_[line.substr(0, pos)] = std::stoull(line.substr(pos + 1));
    }
  }

  size_t FindSearchOffset(const std::string& key) const {
    size_t offset = 0;
    for (const auto& [indexed_key, indexed_offset] : sparse_index_) {
      if (indexed_key > key) {
        break;
      }
      offset = indexed_offset;
    }
    return offset;
  }

  size_t id_;
  std::filesystem::path data_path_;
  std::filesystem::path index_path_;
  std::map<std::string, size_t> sparse_index_;
};

}  // namespace kvstore
