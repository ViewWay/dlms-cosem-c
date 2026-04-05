# dlms-cosem-c

**Complete DLMS/COSEM protocol stack for C99** — zero malloc, embedded-friendly implementation with ASN.1, A-XDR, HDLC, and COSEM IC classes.

[![Tests](https://img.shields.io/badge/tests-36%20passed-brightgreen)]()
[![C99](https://img.shields.io/badge/C-99-blue.svg)]()
[![Embedded](https://img.shields.io/badge/embedded-friendly-green.svg)]()
[![License: BSL 1.1](https://img.shields.io/badge/license-BSL%201.1-orange.svg)]()

## Features

- **Zero malloc**: All stack allocation, suitable for bare-metal embedded systems
- **Pure C99**: No C11/C++ dependencies, compiles on any C99 toolchain
- **ASN.1 BER**: Tag-length-value encoding/decoding
- **A-XDR Codec**: DLMS data encoding
- **HDLC Framing**: LLC/MAC layer with CRC-16
- **COSEM IC Classes**: Interface classes with virtual dispatch pattern
- **Security**: SM4 block cipher

## Building

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
ctest --output-on-failure
```

Or with GCC directly:

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic -I include src/*.c tests/*.c -o test && ./test
```

## Design Principles

- **Fixed-size buffers**: No dynamic allocation, predictable memory usage
- **No STL/libc++**: Only standard C library functions
- **Virtual dispatch**: C-style vtable pattern for IC class polymorphism
- **Compact**: ~6K lines across 23 files

## COSEM IC Classes

Demand, Register Monitor, Disconnect Control, Limiter, Account, Day Profile, Week Profile, and core infrastructure classes.

## Multi-Language Family

| Language | Tests | Lines |
|----------|-------|-------|
| [Python](https://github.com/ViewWay/dlms-cosem) | 5,146 | 37K |
| [Rust](https://github.com/ViewWay/dlms-cosem-rust) | 739 | 21K |
| [Go](https://github.com/ViewWay/dlms-cosem-go) | 362 | 8K |
| [C++](https://github.com/ViewWay/dlms-cosem-cpp) | 280+ | 6.5K |
| **C** | **36** | **6.2K** |

## License

BSL 1.1
