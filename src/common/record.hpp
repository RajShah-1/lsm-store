#pragma once

#include <iostream>
#include <string>

namespace kvstore {

class Record {
 public:
  Record(const std::string& key, const std::string& value)
      : key_(key), value_(value) {}

  Record(std::string&& key, std::string&& value)
      : key_(std::move(key)), value_(std::move(value)) {}

  Record(std::string_view key, std::string_view value)
      : key_(key), value_(value) {}

  const std::string& Key() const { return key_; }
  const std::string& Value() const { return value_; }

  static Record FromString(const std::string& str) {
    auto pos = str.find('=');
    if (pos == std::string::npos) {
      throw std::invalid_argument("Invalid record format");
    }
    return Record(str.substr(0, pos), str.substr(pos + 1));
  }

  std::string ToString() const { return key_ + "=" + value_; }

 private:
  std::string key_;
  std::string value_;
};

inline std::ostream& operator<<(std::ostream& os, const Record& record) {
  os << record.ToString();
  return os;
}

}  // namespace kvstore