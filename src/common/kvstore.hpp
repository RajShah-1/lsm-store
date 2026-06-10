#pragma once
#include <optional>

#include "record.hpp"

namespace kvstore {
// --- Interface ---
class IKVStore {
public:
    virtual ~IKVStore() = default;
    virtual bool Set(const Record& record) = 0;
    virtual std::optional<Record> Get(const std::string& key) = 0;
    virtual void Flush() = 0;
    virtual void Shutdown() = 0;
};
}