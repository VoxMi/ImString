# About
This repository contains a lightweight string management solution developed as part of experimental projects built on top of [Dear ImGui](https://github.com/ocornut/imgui). It provides ImString, ImStringView, and ImStringPool to handle text operations with minimal overhead. The primary goal is not to outperform STL (EASTL or another) strings, hash maps, string hash map in raw throughput, but to strictly adhere to Dear ImGui's paradigms of minimalism, ero external dependencies, and predictable memory control.

# ImString
A custom string class featuring Small String Optimization (SSO). It utilizes a compact union layout (24 bytes on 64-bit platforms, 12 bytes on 32-bit platforms) to store short strings inline, completely avoiding heap allocations for common, small-scale use cases. When the string exceeds the SSO capacity, it seamlessly transitions to heap allocation. Notably, when memory is allocated for the heap layout, it is strictly aligned according to platform-specific requirements.

**Endianness**: *The current implementation of ImString relies on a specific byte-level union layout for heap/SSO mode detection and exclusively supports Little-Endian platforms. Big-Endian architectures are not currently supported.*

# ImStringView
A lightweight, non-owning string view consisting of a simple pointer and a size. It provides a zero-allocation mechanism to pass, slice, and inspect string data without taking ownership or incurring the cost of string duplication.

# ImStringPool
A minimalist string interning pool. Each unique string is stored exactly once in a contiguous array and mapped to a stable integer ID (its array index). By passing these lightweight integer IDs around instead of raw string pointers or full string objects, the pool eliminates redundant heap allocations, significantly reduces the memory footprint, and replaces expensive string comparisons with trivial integer equality checks. It utilizes a 32-bit FNV-1a hash function with linear probing, which has proven highly effective for hashing short keys with minimal collisions.
