#include <iostream>
#include <string>
#include <sstream>
#include "store.hpp"

using namespace geokv;

int main() {
    LSMKVStore store("data", 5);
    std::string line;

    std::cout << "GeoKV CLI - type 'set <key> <val>', 'get <key>', 'flush', 'exit'\n";

    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;

        std::istringstream iss(line);
        std::string cmd, key, val;
        iss >> cmd;

        if (cmd == "exit") break;
        if (cmd == "flush") {
            store.Flush();
            continue;
        }
        if (cmd == "set") {
            iss >> key >> val;
            store.Set(key, val);
        } else if (cmd == "get") {
            iss >> key;
            auto result = store.Get(key);
            if (result) std::cout << key << " = " << *result << "\n";
            else std::cout << "Key not found\n";
        }
    }

    store.Shutdown();
    return 0;
}
