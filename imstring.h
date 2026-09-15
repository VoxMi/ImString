#pragma once
#ifndef IMGUI_DISABLE
#include <imgui/imgui.h>

#include <stdint.h>

#ifndef IM_MIN
#define IM_MIN(A, B)    (((A) < (B)) ? (A) : (B))
#endif
//-----------------------------------------------------------------------------
// About ImString and ImStringPool:
// - The primary goal is not to outperform STL (EASTL or another) strings, hash maps, string hash map in raw throughput, but to strictly adhere
//   to ImGui's paradigms of minimalism, zero external dependencies, and predictable memory control.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// [SECTION] The SSO (Small String Optimization or Short String Optimization) string data-type.
//-----------------------------------------------------------------------------
// Notes:
// - It uses a 24-byte on 64-bit platform (12-byte on 32-bit platform) union layout to store short strings inline (SSO) or allocate on the heap.
// - The code only supports Little-Endian platforms.
//-----------------------------------------------------------------------------

struct ImString
{
    // TODO: Implement support for Big-Endian byte order.
    static constexpr unsigned char HEAP_FLAG_BYTE   = 1u << 7;

    struct LayoutHeap
    {
        char*           Data;
        size_t          Size;
        size_t          Capacity; // Exclude zero terminator
    };

    struct LayoutSSO
    {
        static constexpr size_t SSO_CAPACITY = sizeof(LayoutHeap) - 1;

        char            Data[SSO_CAPACITY];
        unsigned char   Size;
    };

    /*
        Storage on 64-bit LE platform (24 bytes padded for 8-byte alignment)
        ┌────────┬─────────────────────────────┬───────────────────────────┐
        │ Offset │ Heap-Layout                 │ SSO-Layout                │
        ├────────┼─────────────────────────────┼───────────────────────────┤
        │ 00..07 │ Heap.Data (8B pointer)      │ SSO.Data[00..07]          │
        ├────────┼─────────────────────────────┼───────────────────────────┤
        │ 08..15 │ Heap.Size (8B size_t)       │ SSO.Data[08..15]          │
        ├────────┼─────────────────────────────┼───────────────────────────┤
        │ 16..22 │ Heap.Capacity (8B size_t)   │ SSO.Data[16..22]          │
        ├────────┼─────────────────────────────┼───────────────────────────┤
        │ 23     │ Heap.Capacity (1 byte)      │ SSO.Size (1 byte)         │ ←– MODE DETECTION TAG
        └────────┴─────────────────────────────┴───────────────────────────┘
        Total size is 24 bytes. SSO_CAPACITY is 23 bytes.

        Storage on 32-bit LE platform (12 bytes padded for 4-byte alignment)
        ┌────────┬─────────────────────────────┬───────────────────────────┐
        │ Offset │ Heap-Layout                 │ SSO-Layout                │
        ├────────┼─────────────────────────────┼───────────────────────────┤
        │ 00..03 │ Heap.Data (4B pointer)      │ SSO.Data[00..03]          │
        ├────────┼─────────────────────────────┼───────────────────────────┤
        │ 04..07 │ Heap.Size (4B size_t)       │ SSO.Data[04..07]          │
        ├────────┼─────────────────────────────┼───────────────────────────┤
        │ 08..10 │ Heap.Capacity (3 bytes)     │ SSO.Data[08..10]          │
        ├────────┼─────────────────────────────┼───────────────────────────┤
        │ 11     │ Heap.Capacity (1 byte)      │ SSO.Size (1 byte)         │ ←– MODE DETECTION TAG
        └────────┴─────────────────────────────┴───────────────────────────┘
        Total size is 12 bytes. SSO_CAPACITY is 11 bytes.
    */
    union
    {
        LayoutHeap      Heap;
        LayoutSSO       SSO;
    };

    inline ImString()                                           { memset((void*)this, 0, sizeof(*this)); }
    ImString(const char* str);
    ImString(const char* str, size_t len);
    ImString(size_t count, char ch);
    ImString(const ImString& other);
    ImString(ImString&& other) noexcept;
    inline ~ImString()                                          { deallocate_self(); }

    ImString& operator=(const char* str);
    ImString& operator=(ImString&& other) noexcept;
    ImString& operator=(const ImString& other);

    inline ImString& operator+=(const ImString& other)          { append(other.data(), other.size()); return *this; }
    inline ImString& operator+=(const char* str)                { append(str); return *this; }
    inline ImString& operator+=(char ch)                        { push_back(ch); return *this; }

    inline ImString operator+(const ImString& other) const      { ImString result(*this); result += other; return result; }
    inline ImString operator+(const char* str) const            { ImString result(*this); result.append(str); return result; }

    inline bool             is_heap() const                     { return (((unsigned char*)this)[sizeof(ImString) - 1] & HEAP_FLAG_BYTE) != 0; }
    inline bool             is_sso() const                      { return !is_heap(); }

    inline char*            data()                              { return is_heap() ? Heap.Data : SSO.Data; }
    inline const char*      data() const                        { return is_heap() ? Heap.Data : SSO.Data; }
    inline const char*      c_str() const                       { return data(); }

    void                    clear();
    inline void             deallocate_self()                   { if(is_heap() && Heap.Data) { IM_FREE(Heap.Data); memset((void*)this, 0, sizeof(*this)); } }

    inline size_t           size() const                        { return is_heap() ? Heap.Size : SSO.Size; }
    inline size_t           capacity() const                    { return is_heap() ? get_heap_capacity() : LayoutSSO::SSO_CAPACITY - 1; }
    inline bool             empty() const                       { return size() == 0; }
    inline size_t           length() const                      { return size(); }

    inline char&            operator[](size_t pos)              { IM_ASSERT(pos >= 0 && pos < size()); return data()[pos]; }
    inline const char&      operator[](size_t pos) const        { IM_ASSERT(pos >= 0 && pos < size()); return data()[pos]; }

    inline char*            begin()                             { return data(); }
    inline const char*      begin() const                       { return data(); }
    inline char*            end()                               { return data() + size(); }
    inline const char*      end() const                         { return data() + size(); }

    inline char             front()                             { IM_ASSERT(size() > 0); return data()[0]; }
    inline const char       front() const                       { IM_ASSERT(size() > 0); return data()[0]; }
    inline char             back()                              { IM_ASSERT(size() > 0); return data()[size() - 1]; }
    inline const char       back() const                        { IM_ASSERT(size() > 0); return data()[size() - 1]; }

    inline void             set_size(size_t sz)                 { is_heap() ? Heap.Size = sz : SSO.Size = (unsigned char)sz; }

    inline size_t           get_heap_capacity() const           { size_t cap = Heap.Capacity; ((unsigned char*)&cap)[sizeof(size_t) - 1] &= ~HEAP_FLAG_BYTE; return cap; }
    inline void             set_heap_capacity(size_t cap)       { Heap.Capacity = cap; ((unsigned char*)&Heap.Capacity)[sizeof(size_t) - 1] |= HEAP_FLAG_BYTE; }
    inline void             clear_heap_flag()                   { ((unsigned char*)this)[sizeof(ImString) - 1] &= ~HEAP_FLAG_BYTE;}

    void                    heap_grow_capacity(size_t min_cap);
    void                    resize(size_t new_size, char ch);
    inline void             resize(size_t new_size)             { resize(new_size, '\0'); }
    inline void             reserve(size_t new_cap)             { if(new_cap > capacity()) heap_grow_capacity(new_cap); }
    void                    shrink_to_fit();

    void                    push_back(char ch);
    void                    pop_back();

    void                    append(const char* str, size_t len);
    inline void             append(const char* str)             { append(str, strlen(str)); }
    void                    appendf(const char* fmt, ...) IM_FMTARGS(2);
    void                    appendfv(const char* fmt, va_list args) IM_FMTLIST(2);

    void                    insert(size_t pos, const char* str, size_t len);
    void                    insert(size_t pos, const char* str);
    void                    insert(size_t pos, size_t count, char ch);
    void                    insert(size_t pos, const ImString& other);

    char*                   insert(char* position, char ch);
    char*                   insert(char* position, size_t count, char ch);

    void                    erase(size_t pos = 0, size_t len = size_t(-1));
    char*                   erase(char* position);
    char*                   erase(char* first, char* last);

    inline ImString         substr(size_t offset, size_t count) const { return (offset > size()) ? ImString() : ImString(begin() + offset, IM_MIN(count, size() - offset)); }
};

inline bool operator==(const ImString& a, const ImString& b)    { return a.size() == b.size() && memcmp(a.data(), b.data(), a.size()) == 0; }
inline bool operator!=(const ImString& a, const ImString& b)    { return !(a == b); }

template <typename T, typename U>
inline void ImVectorPushBack(ImVector<T>& vec, U&& elem)
{
    vec.reserve(vec.Size + 1);
    IM_PLACEMENT_NEW(&vec.Data[vec.Size]) T(elem);
    vec.Size += 1;
}

//-----------------------------------------------------------------------------
// [SECTION] ImStringView: a lightweight, non-owning string view (pointer + size)
//-----------------------------------------------------------------------------

struct ImStringView
{
    const char*     DataPtr;
    size_t          DataSize;

    inline ImStringView()                                  : DataPtr(nullptr), DataSize(0) {}
    inline ImStringView(const char* str)                   : DataPtr(str), DataSize(str ? strlen(str) : 0) {}
    inline ImStringView(const char* str, size_t len)       : DataPtr(str), DataSize(len) {}

    inline char             operator[](size_t pos) const   { return DataPtr[pos]; }
    inline const char*      begin() const                  { return DataPtr; }
    inline const char*      end() const                    { return DataSize > 0 ? DataPtr + DataSize : DataPtr; }
    inline size_t           size() const                   { return DataSize; }
    inline bool             empty() const                  { return DataSize == 0; }
    inline const char*      data() const                   { return DataPtr; }

    inline ImStringView substr(size_t offset, size_t count = size_t(-1)) const
    {
        if(offset >= DataSize)
            return ImStringView();
        count = IM_MIN(count, DataSize - offset);
        return ImStringView(DataPtr + offset, count);
    }
};

//-----------------------------------------------------------------------------
// [SECTION] A minimalist string interning pool.
//-----------------------------------------------------------------------------
// Notes:
// - Each unique string is stored exactly once in the contiguous 'Strings' array and mapped to a stable integer ID (its array index).
//   By passing these lightweight integer IDs around instead of raw string pointers, we eliminate redundant heap allocations,
//   reduce memory footprint, and replace expensive string comparisons with simple integer equality checks.
//
// - The 32-bit version of "FNV-1a" is used as the hash function. This is a good choice for hashing very short keys.
//   Testing on 466K words from a dictionary such as https://github.com/dwyl/english-words/blob/master/words.txt revealed only 23 collisions.
//-----------------------------------------------------------------------------

struct ImStringPool
{
    static constexpr int INVALID_IDX = -1;

    struct Slot
    {
        uint32_t    Hash;
        int         Index;
    };

    ImVector<ImString>  Strings;
    ImVector<Slot>      Slots;
    int                 Capacity;

    inline ImStringPool()                               { Capacity = 1 << 8;  Strings.reserve(Capacity / 2); Slots.resize(Capacity, { 0, INVALID_IDX }); } // Reserves 256 by default
    inline ~ImStringPool()                              { Strings.clear_destruct(); }

    ImStringPool(const ImStringPool&)                   = delete;
    ImStringPool&       operator=(const ImStringPool&)  = delete;

    inline int          size() const                    { return Strings.size(); }
    inline const char*  at(int idx) const               { if(idx < 0 || idx >= Strings.Size) return nullptr; return Strings[idx].c_str(); };

    void                grow(int new_capacity); // Increasing the size and rehashing
    void                clear();                // Important: only destroys the elements of the ImStringPool::Strings array.
    bool                find_erase(int idx);
    bool                find_erase(const char* key);

    int                 insert(const char* key);
    int                 find_index(const char* key) const;
};
#endif  // #ifndef IMGUI_DISABLE