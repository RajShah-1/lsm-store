#pragma once

#include <string>
#include <optional>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <atomic>
#include <thread>

namespace geokv {

// --- Interface ---
class IKVStore {
public:
    virtual ~IKVStore() = default;
    virtual bool Set(const std::string& key, const std::string& value) = 0;
    virtual std::optional<std::string> Get(const std::string& key) = 0;
    virtual void Flush() = 0;
    virtual void Shutdown() = 0;
};

// --- LSM Tree Implementation ---
class LSMKVStore : public IKVStore {
public:
    explicit LSMKVStore(const std::string& data_dir, size_t flush_threshold = 5);
    ~LSMKVStore();

    bool Set(const std::string& key, const std::string& value) override;
    std::optional<std::string> Get(const std::string& key) override;
    void Flush() override;
    void Shutdown() override;

private:
    void loadFromWAL();
    void flushToSST();
    void runCompactionLoop();
    void stopCompaction();

    // Data
    std::unordered_map<std::string, std::string> memtable_;
    std::mutex mutex_;

    std::string data_dir_;
    size_t flush_threshold_;
    std::unique_ptr<std::ofstream> wal_;

    std::atomic<bool> stop_compaction_;
    std::thread compaction_thread_;
    size_t sstable_counter_;
};

} // namespace geokv
