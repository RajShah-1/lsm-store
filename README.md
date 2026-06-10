# GeoKV

GeoKV is a small C++20 key-value store project. The current version is an
LSM-tree-style storage engine with a write-ahead log, sorted memtable, immutable
SSTables, sparse indexes, startup recovery, and synchronous compaction.

## Current State

The project currently builds a CLI binary, `geokv_cli`, that starts an
`LSMKVStore` and supports a small interactive command set:

```text
set <key> <value>
get <key>
flush
exit
```

Implemented pieces:

- CMake-based C++20 project structure.
- `IKVStore` interface for the storage engine contract.
- `Record` type with simple `key=value` serialization.
- Process-local configuration through `KVConfig`.
- Memtable backed by `std::map`.
- Write-ahead log under `./data/wal/wal.log`.
- WAL replay on startup into the memtable.
- Threshold-based memtable flushes into SSTables.
- Sparse SSTable indexes for bounded lookup scans.
- Newest-first SSTable reads so later writes win.
- WAL truncation after successful SSTable flushes.
- Synchronous compaction when too many SSTables accumulate.
- CLI entry point for manual testing.

## Repository Layout

```text
.
|-- CMakeLists.txt
|-- src
    |-- CMakeLists.txt
    |-- config.hpp
    |-- main.cpp
    |-- common
    |   |-- kvstore.hpp
    |   `-- record.hpp
    `-- lsm
        |-- lsmstore.hpp
        |-- memtable.hpp
        |-- sstable.hpp
        `-- wal.hpp
`-- tests
    |-- CMakeLists.txt
    `-- test_geokv.cpp
```

## Build

From the repository root:

```sh
cmake -S . -B build
cmake --build build
```

The CLI binary is emitted under:

```text
build/bin/geokv_cli
```

## Test

GeoKV uses GoogleTest. With GTest installed and discoverable by CMake:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Run

```sh
./build/bin/geokv_cli
```

Example session:

```text
GeoKV CLI - type 'set <key> <val>', 'get <key>', 'flush', 'exit'
> set city sf
> get city
city: {city=sf}
> exit
```

The CLI currently configures:

- data directory: `./data`
- LSM flush threshold: `5`
- SSTable index frequency: default `4`

WAL data is written to `./data/wal/wal.log`.
SSTables and indexes are written to `./data/sst/`.

## Development Notes

- The project targets C++20.
- Most implementation is currently header-based.
- `KVConfig` must be initialized before constructing `LSMKVStore`.
- Values are parsed as a single token in the CLI, so values with spaces are not
  supported yet.
- The WAL format is line-oriented `key=value` records.
- Delete `data/` to reset local persisted state during development.

## Future Work

Likely next steps:

- Add tombstones and delete support.
- Add range scans over memtable and SSTables.
- Replace whole-table compaction with leveled or size-tiered compaction.
- Add checksums or length-prefixing for stronger WAL/SSTable corruption
  handling.
- Add CLI support for quoted values containing spaces.
