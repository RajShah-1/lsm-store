# Test Coverage

Generated on 2026-06-10 with AppleClang coverage instrumentation and `gcov`.

## Summary

```text
File                         Covered / Lines    Coverage
tests/test_geokv.cpp             110 / 111        99.10%
src/common/record.hpp             13 / 16         81.25%
src/common/kvstore.hpp             1 / 1         100.00%
src/config.hpp                    30 / 38         78.95%
src/lsm/memtable.hpp              25 / 25        100.00%
src/lsm/wal.hpp                   58 / 66         87.88%
src/lsm/sstable.hpp              103 / 112        91.96%
src/lsm/lsmstore.hpp              82 / 88         93.18%
```

Project files covered: 422 / 457 lines, 92.34%.

## Test Run

```text
100% tests passed, 0 tests failed out of 6
```

Covered behavior:

- Record parsing and serialization.
- Memtable overwrites, reads, reset, and freeze protection.
- WAL replay for unflushed writes.
- Threshold flushes into SSTables.
- WAL truncation after successful SSTable flushes.
- Newest-first SSTable reads for overwritten keys.
- Synchronous compaction and post-compaction recovery.

## Reproduce

```sh
cmake -S . -B cmake-build-coverage -DGEOKV_ENABLE_COVERAGE=ON
cmake --build cmake-build-coverage
ctest --test-dir cmake-build-coverage --output-on-failure
gcov cmake-build-coverage/tests/CMakeFiles/geokv_tests.dir/test_geokv.cpp.gcda
```

The raw `gcov` output includes C++ standard library and GoogleTest internals.
The summary above filters that output to project files only.
