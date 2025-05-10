#include "store.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>

namespace fs = std::filesystem;

namespace geokv {

LSMKVStore::LSMKVStore(const std::string& data_dir, size_t flush_threshold)
    : data_dir_(data_dir),
      flush_threshold_(flush_threshold),
      stop_compaction_(false),
      sstable_counter_(0)
{
    if (!fs::exists(data_dir_)) {
        fs::create_directories(data_dir_);
    }

    std::string wal_path = data_dir_ + "/wal.log";
    wal_ = std::make_unique<std::ofstream>(wal_path, std::ios::app | std::ios::out);
    if (!wal_->is_open()) {
        throw std::runtime_error("Failed to open WAL file");
    }

    loadFromWAL();
    compaction_thread_ = std::thread(&LSMKVStore::runCompactionLoop, this);
}

LSMKVStore::~LSMKVStore() {
    Shutdown();
}

void LSMKVStore::Shutdown() {
    stopCompaction();
    if (wal_ && wal_->is_open()) {
        wal_->close();
    }
}

bool LSMKVStore::Set(const std::string& key, const std::string& value) {
    std::lock_guard lock(mutex_);

    // Write to WAL
    *wal_ << key << "=" << value << "\n";
    wal_->flush();

    // Update memtable
    memtable_[key] = value;

    if (memtable_.size() >= flush_threshold_) {
        flushToSST();
    }

    return true;
}

std::optional<std::string> LSMKVStore::Get(const std::string& key) {
    std::lock_guard lock(mutex_);

    // 1. Check memtable
    if (memtable_.find(key) != memtable_.end()) {
        return memtable_[key];
    }

    // 2. Scan SSTables (newest first)
    for (int i = static_cast<int>(sstable_counter_) - 1; i >= 0; --i) {
        std::string sst_path = data_dir_ + "/sst_" + std::to_string(i) + ".txt";
        std::ifstream sst_file(sst_path);
        if (!sst_file.is_open()) continue;

        std::string line;
        while (std::getline(sst_file, line)) {
            auto pos = line.find('=');
            if (pos != std::string::npos) {
                std::string k = line.substr(0, pos);
                std::string v = line.substr(pos + 1);
                if (k == key) return v;
            }
        }
    }

    return std::nullopt;
}

void LSMKVStore::Flush() {
    std::lock_guard lock(mutex_);
    flushToSST();
}

void LSMKVStore::loadFromWAL() {
    std::ifstream infile(data_dir_ + "/wal.log");
    std::string line;
    while (std::getline(infile, line)) {
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        memtable_[key] = value;
    }
}

void LSMKVStore::flushToSST() {
    if (memtable_.empty()) return;

    // Create SSTable
    std::vector<std::pair<std::string, std::string>> entries;
    for (const auto& [k, v] : memtable_) {
        entries.emplace_back(k, v);
    }
    std::sort(entries.begin(), entries.end());

    std::string sst_path = data_dir_ + "/sst_" + std::to_string(sstable_counter_++) + ".txt";
    std::ofstream sst_file(sst_path);
    for (const auto& [k, v] : entries) {
        sst_file << k << "=" << v << "\n";
    }
    sst_file.close();

    // Reset WAL
    wal_->close();
    std::ofstream reset_wal(data_dir_ + "/wal.log", std::ios::trunc);
    reset_wal.close();
    wal_ = std::make_unique<std::ofstream>(data_dir_ + "/wal.log", std::ios::app | std::ios::out);

    // Clear memtable
    memtable_.clear();

    std::cout << "Flushed to " << sst_path << "\n";
}

void LSMKVStore::runCompactionLoop() {
    using namespace std::chrono_literals;
    while (!stop_compaction_.load()) {
        std::this_thread::sleep_for(10s); // Periodic compaction every 10 sec
        // TODO: Implement merge logic (in future step)
    }
}

void LSMKVStore::stopCompaction() {
    stop_compaction_.store(true);
    if (compaction_thread_.joinable()) {
        compaction_thread_.join();
    }
}

} // namespace geokv
