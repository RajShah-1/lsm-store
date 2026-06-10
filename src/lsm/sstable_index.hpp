#pragma once

#include <string>
#include <optional>
#include <unordered_map>
#include <map>
#include <mutex>
#include <memory>
#include <atomic>
#include <thread>
#include <iostream>
#include <fstream>

#include <filesystem>
#include "../config.hpp"

namespace kvstore {
class SSTableIndex
{
private:
    std::unordered_map<std::string, size_t> sparse_index_;
    std::string sstable_path_;
    std::string index_path_;
    size_t sstable_id_;

    size_t index_frequency_;

public:
    SSTableIndex(const size_t index_frequency) : index_frequency_(index_frequency)
    {
        // Load the index from the file
        auto &config = KVConfig::Instance();
        index_path_ = config.GetDataDir() + "sst/index.txt";

        if (!std::filesystem::exists(index_path_))
        {
            throw std::runtime_error("Index file does not exist");
        }

        // Open the index file and read the contents
        LoadIndex();
    }


    /**
     *  File Format:
     *      sst_id = 3
     *      
     *      min_key = apple
     *      max_key = zebra
     *      
     *      # key = byte offset
     *      apple = 0
     *      banana = 47
     *      mango = 94
     *      zebra = 141
     */
    void LoadIndex()
    {
        std::ifstream index_file(index_path_);
        if (!index_file.is_open())
        {
            throw std::runtime_error("Failed to open index file");
        }

        std::string line;
        while (std::getline(index_file, line))
        {
            auto pos = line.find('=');
            if (pos != std::string::npos)
            {
                std::string key = line.substr(0, pos);
                size_t offset = std::stoull(line.substr(pos + 1));
                sparse_index_[key] = offset;
            }
        }
        index_file.close();
    }

    SSTableIndex(Memtable &memtable, size_t id)
    {
        // Create a new index for the memtable
        sstable_id_ = id;

        // TODO: Create a base-dir const in KVConfig singleton
        sstable_path_ = "sst_" + std::to_string(sstable_id_) + ".txt";
        index_path_ = sstable_path_ + ".index";

        if (std::filesystem::exists(index_path_))
        {
            throw std::runtime_error("Index file already exists");
        }

        if (std::filesystem::exists(sstable_path_))
        {
            throw std::runtime_error("SSTable file already exists");
        }

        std::ofstream index_file(index_path_);
        std::ofstream sstable_file(sstable_path_);

        size_t counter = 0;

        for (const auto &[key, record] : memtable)
        {
            counter++;
            sstable_file << record.ToString() << "\n";
            if (counter % INDEX_FREQUENCY == 0)
            {
                sparse_index_[key] = sstable_file.tellp();
                index_file << key << "=" << sparse_index_[key] << "\n";
            }
        }

        sstable_file.close();
        index_file.close();
        std::cout << "SSTable created: " << sstable_path_ << "\n";
        std::cout << "Index created: " << index_path_ << "\n";
    }

    std::optional<size_t> GetOffset(const std::string &key) const
    {
        auto it = sparse_index_.find(key);
        if (it != sparse_index_.end())
        {
            return it->second;
        }
        return std::nullopt;
    }

    void SaveIndex()
    {
        std::ofstream index_file(index_path_, std::ios::app);
        for (const auto &[key, offset] : sparse_index_)
        {
            index_file << key << "=" << offset << "\n";
        }
        index_file.close();
    }
};
}