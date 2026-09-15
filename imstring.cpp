#include "imstring.h"
#ifndef IMGUI_DISABLE
#include "imgui_internal.h"

#include "misc/hash_utils.h"

#ifndef IM_PLATFORM_MALLOC_ALIGNMENT
#if defined(__APPLE__)
#define IM_PLATFORM_MALLOC_ALIGNMENT 16
#elif defined(__ANDROID__)
#define IM_PLATFORM_MALLOC_ALIGNMENT 8
#else
#define IM_PLATFORM_MALLOC_ALIGNMENT (sizeof(void*) * 2)
#endif
#endif

#define IM_PLATFORM_ALIGN_SIZE(sz) ((sz + IM_PLATFORM_MALLOC_ALIGNMENT - 1) & (~(IM_PLATFORM_MALLOC_ALIGNMENT - 1)))

//-----------------------------------------------------------------------------
// [SECTION] ImString
//-----------------------------------------------------------------------------

ImString::ImString(const char* str)
{
    if(str == nullptr)
    {
        SSO.Data[0] = '\0';
        SSO.Size = 0;
        return;
    }

    size_t len = strlen(str);
    if(len < LayoutSSO::SSO_CAPACITY)
    {
        memcpy(SSO.Data, str, len);
        SSO.Data[len] = '\0';
        SSO.Size = (unsigned char)(len);
    }
    else
    {
        size_t alloc_size = IM_PLATFORM_ALIGN_SIZE(len + 1);
        Heap.Data = (char*)(IM_ALLOC(alloc_size));
        IM_ASSERT(Heap.Data != nullptr);
        memcpy(Heap.Data, str, len);
        Heap.Data[len] = '\0';
        Heap.Size = len;

        set_heap_capacity(alloc_size - 1);
    }
}

ImString::ImString(const char* str, size_t len)
{
    if(str == nullptr || len == 0)
    {
        SSO.Data[0] = '\0';
        SSO.Size = 0;
        return;
    }

    if(len < LayoutSSO::SSO_CAPACITY)
    {
        memcpy(SSO.Data, str, len);
        SSO.Data[len] = '\0';
        SSO.Size = (unsigned char)(len);
    }
    else
    {
        size_t alloc_size = IM_PLATFORM_ALIGN_SIZE(len + 1);
        Heap.Data = (char*)(IM_ALLOC(alloc_size));
        IM_ASSERT(Heap.Data != nullptr);
        memcpy(Heap.Data, str, len);
        Heap.Data[len] = '\0';
        Heap.Size = len;

        set_heap_capacity(alloc_size - 1);
    }
}

ImString::ImString(size_t count, char ch)
{
    if(count < LayoutSSO::SSO_CAPACITY)
    {
        memset(SSO.Data, ch, count);
        SSO.Data[count] = '\0';
        SSO.Size = (unsigned char)(count);
    }
    else
    {
        size_t alloc_size = IM_PLATFORM_ALIGN_SIZE(count + 1);
        Heap.Data = (char*)(IM_ALLOC(alloc_size));
        IM_ASSERT(Heap.Data != nullptr);
        memset(Heap.Data, ch, count);
        Heap.Data[count] = '\0';
        Heap.Size = count;

        set_heap_capacity(alloc_size - 1);
    }
}

ImString::ImString(const ImString& other)
{
    if(!other.is_heap())
    {
        memcpy(SSO.Data, other.SSO.Data, other.SSO.Size);
        SSO.Data[other.SSO.Size] = '\0';
        SSO.Size = other.SSO.Size;
    }
    else
    {
        size_t alloc_size = IM_PLATFORM_ALIGN_SIZE(other.Heap.Size + 1);
        Heap.Data = (char*)(IM_ALLOC(alloc_size));
        IM_ASSERT(Heap.Data != nullptr);
        memcpy(Heap.Data, other.Heap.Data, other.Heap.Size);
        Heap.Data[other.Heap.Size] = '\0';
        Heap.Size = other.Heap.Size;

        set_heap_capacity(alloc_size - 1);
    }
}

ImString::ImString(ImString&& other) noexcept
{
    if(other.is_heap())
        Heap = other.Heap; // Shallow copying. Flag is already set in Capacity.
    else
    {
        memcpy(SSO.Data, other.SSO.Data, other.SSO.Size);
        SSO.Data[other.SSO.Size] = '\0';
        SSO.Size = other.SSO.Size;
    }

    // Reset source
    memset((void*)&other, 0, sizeof(other));
}

ImString& ImString::operator=(const char* str)
{
    if(str == nullptr)
    {
        SSO.Data[0] = '\0';
        SSO.Size = 0;
        return *this;
    }

    char* old_heap_data = is_heap() ? Heap.Data : nullptr;

    size_t len = strlen(str);
    if(len < LayoutSSO::SSO_CAPACITY)
    {
        memcpy(SSO.Data, str, len);
        SSO.Data[len] = '\0';
        SSO.Size = (unsigned char)(len);
    }
    else
    {
        size_t alloc_size = IM_PLATFORM_ALIGN_SIZE(len + 1);
        Heap.Data = (char*)(IM_ALLOC(alloc_size));
        IM_ASSERT(Heap.Data != nullptr);
        memcpy(Heap.Data, str, len);
        Heap.Data[len] = '\0';
        Heap.Size = len;

        set_heap_capacity(alloc_size - 1);
    }

    if(old_heap_data)
        IM_FREE(old_heap_data);

    return *this;
}

ImString& ImString::operator=(ImString&& other) noexcept
{
    if(this != &other)
    {
        if(is_heap() && Heap.Data)
            IM_FREE(Heap.Data);

        if(other.is_heap())
            Heap = other.Heap; // Shallow copying. Flag is already set in Capacity.
        else
        {
            memcpy(SSO.Data, other.SSO.Data, other.SSO.Size);
            SSO.Data[other.SSO.Size] = '\0';
            SSO.Size = other.SSO.Size;
        }

        // Reset source
        memset((void*)&other, 0, sizeof(other));
    }

    return *this;
}

ImString& ImString::operator=(const ImString& other)
{
    if(this != &other)
    {
        if(is_heap() && Heap.Data)
            IM_FREE(Heap.Data);

        if(other.is_sso())
        {
            memcpy(SSO.Data, other.SSO.Data, other.SSO.Size);
            SSO.Data[other.SSO.Size] = '\0';
            SSO.Size = other.SSO.Size;
        }
        else
        {
            size_t alloc_size = IM_PLATFORM_ALIGN_SIZE(other.Heap.Size + 1);
            Heap.Data = (char*)(IM_ALLOC(alloc_size));
            IM_ASSERT(Heap.Data != nullptr);
            memcpy(Heap.Data, other.Heap.Data, other.Heap.Size);
            Heap.Data[other.Heap.Size] = '\0';
            Heap.Size = other.Heap.Size;

            set_heap_capacity(alloc_size - 1);
        }
    }

    return *this;
}

void ImString::clear()
{
    if(is_sso())
    {
        SSO.Data[0] = '\0';
        SSO.Size = 0;
    }
    else
    {
        Heap.Data[0] = '\0';
        Heap.Size = 0;
    }
}

void ImString::heap_grow_capacity(size_t min_cap)
{
    size_t cur_cap = capacity();
    if(min_cap <= cur_cap)
        return;

    size_t new_cap = cur_cap + cur_cap / 2; // Exponential growth 1.5x

    if(new_cap < min_cap)
        new_cap = min_cap;

    size_t alloc_size = IM_PLATFORM_ALIGN_SIZE(new_cap + 1);
    char* new_data = (char*)(IM_ALLOC(alloc_size));
    IM_ASSERT(new_data != nullptr);
    size_t sz = size();
    memcpy(new_data, data(), sz);
    new_data[sz] = '\0';

    if(is_heap() && Heap.Data)
        IM_FREE(Heap.Data);

    Heap.Data = new_data;
    Heap.Size = sz;

    set_heap_capacity(alloc_size - 1);
}

void ImString::resize(size_t new_size, char ch)
{
    size_t cur_size = size();
    if(new_size < cur_size)
    {
        char* ptr = data();
        ptr[new_size] = '\0';
        set_size(new_size);
    }
    else if(new_size > cur_size)
    {
        heap_grow_capacity(new_size);
        char* ptr = data();
        memset(ptr + cur_size, ch, new_size - cur_size);
        ptr[new_size] = '\0';
        set_size(new_size);
    }
}

void ImString::shrink_to_fit()
{
    if(!is_heap())
        return;

    if(Heap.Size < LayoutSSO::SSO_CAPACITY)
    {
        char* data_ptr = Heap.Data;
        size_t data_size = Heap.Size;

        memcpy(SSO.Data, data_ptr, data_size);
        SSO.Data[data_size] = '\0';
        SSO.Size = (unsigned char)(data_size);
        IM_FREE(data_ptr);
    }
    else
    {
        size_t cur_cap = get_heap_capacity();
        if(Heap.Size >= cur_cap)
            return;

        size_t alloc_size = IM_PLATFORM_ALIGN_SIZE(Heap.Size + 1);
        if(alloc_size - 1 >= cur_cap)
            return;

        char* new_data = (char*)(IM_ALLOC(alloc_size));
        IM_ASSERT(new_data != nullptr);
        memcpy(new_data, Heap.Data, Heap.Size);
        new_data[Heap.Size] = '\0';

        IM_FREE(Heap.Data);
        Heap.Data = new_data;

        set_heap_capacity(alloc_size - 1);
    }
}

void ImString::push_back(char ch)
{
    size_t sz = size();
    if(sz + 1 > capacity())
        heap_grow_capacity(sz + 1);
    char* ptr = data();
    ptr[sz] = ch;
    ptr[sz + 1] = '\0';
    set_size(sz + 1);
}

void ImString::pop_back()
{
    size_t sz = size();
    if(sz > 0)
    {
        char* ptr = data();
        ptr[sz - 1] = '\0';
        set_size(sz - 1);
    }
}

void ImString::append(const char* str, size_t len)
{
    size_t sz = size();
    if(sz + len > capacity())
        heap_grow_capacity(sz + len);
    char* ptr = data();
    memmove(ptr + sz, str, len); // memmove guarantees correct operation with overlapping memory areas
    ptr[sz + len] = '\0';
    set_size(sz + len);
}

void ImString::appendf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    appendfv(fmt, args);
    va_end(args);
}

void ImString::appendfv(const char* fmt, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);

    int len = ImFormatStringV(NULL, 0, fmt, args);
    if(len <= 0)
    {
        va_end(args_copy);
        return;
    }

    const size_t write_off = size();
    const size_t needed_sz = write_off + len;
    resize(needed_sz);

    ImFormatStringV(&data()[write_off], (size_t)len + 1, fmt, args_copy);
    va_end(args_copy);
}

void ImString::insert(size_t pos, const char* str, size_t len)
{
    size_t sz = size();
    if(pos > sz)
        pos = sz;

    if(!str || len == 0)
        return;

    size_t new_size = sz + len;

    if(new_size > capacity())
        heap_grow_capacity(new_size);

    char* ptr = data();

    size_t tail = sz - pos;
    if(tail > 0)
        memmove(ptr + pos + len, ptr + pos, tail);

    memmove(ptr + pos, str, len);

    ptr[new_size] = '\0';
    set_size(new_size);
}

void ImString::insert(size_t pos, const char* str)
{
    if(!str)
        return;

    insert(pos, str, strlen(str));
}

void ImString::insert(size_t pos, size_t count, char ch)
{
    size_t sz = size();

    if(pos > sz)
        pos = sz;

    if(count == 0)
        return;

    size_t new_size = sz + count;
    if(new_size > capacity())
        heap_grow_capacity(new_size);

    char* ptr = data();

    size_t tail = sz - pos;
    if(tail > 0)
        memmove(ptr + pos + count, ptr + pos, tail);

    memset(ptr + pos, ch, count);

    ptr[new_size] = '\0';
    set_size(new_size);
}

void ImString::insert(size_t pos, const ImString& other)
{
    insert(pos, other.data(), other.size());
}

char* ImString::insert(char* position, char ch)
{
    IM_ASSERT(position >= data() && position <= data() + size());
    size_t pos = position - data();
    insert(pos, 1, ch);
    return data() + pos;
}

char* ImString::insert(char* position, size_t count, char ch)
{
    IM_ASSERT(position >= data() && position <= data() + size());
    size_t pos = position - data();
    insert(pos, count, ch);
    return data() + pos;
}

void ImString::erase(size_t pos, size_t len)
{
    size_t sz = size();
    if(pos >= sz)
        return;

    if(len == size_t(-1) || pos + len > sz)
        len = sz - pos;

    if(len == 0)
        return;

    char* ptr = data();
    size_t tail = sz - (pos + len);

    if(tail > 0)
        memmove(ptr + pos, ptr + pos + len, tail);

    ptr[sz - len] = '\0';
    set_size(sz - len);
}

char* ImString::erase(char* position)
{
    size_t pos = position - data();
    erase(pos, 1);
    return data() + pos;
}

char* ImString::erase(char* first, char* last)
{
    size_t pos = first - data();
    size_t len = last - first;
    erase(pos, len);
    return data() + pos;
}

//-----------------------------------------------------------------------------
// [SECTION] ImStringPool
//-----------------------------------------------------------------------------

void ImStringPool::grow(int new_capacity)
{
    if(new_capacity <= 0 || new_capacity <= Capacity)
        return;

    IM_ASSERT((new_capacity > 0) && ((new_capacity & (new_capacity - 1)) == 0));

    Strings.reserve(new_capacity / 2);

    ImVector<Slot> new_slots;
    new_slots.resize(new_capacity, { 0, INVALID_IDX });

    // Rehashing existing elements
    for(int idx = 0; idx < Strings.Size; ++idx)
    {
        uint32_t hash = Hash::fnv1a_32(Strings[idx].c_str());
        int slot = hash & (new_capacity - 1);

        // Linear probing to find an empty slot
        while(new_slots[slot].Index != -1)
            slot = (slot + 1) & (new_capacity - 1);

        new_slots[slot].Hash = hash;
        new_slots[slot].Index = idx;
    }

    Capacity = new_capacity;
    Slots.swap(new_slots);
}

void ImStringPool::clear()
{
    if(Strings.Size != 0)
    {
        for(int i = 0; i < Strings.Size; ++i)
            Strings[i].deallocate_self();

        Strings.Size = 0;

        for(int i = 0; i < Slots.Size; ++i)
        {
            Slots[i].Hash = 0;
            Slots[i].Index = INVALID_IDX;
        }
    }
}

bool ImStringPool::find_erase(int idx)
{
    if(idx < 0 || idx >= Strings.Size)
        return false;

    uint32_t hash = Hash::fnv1a_32(Strings[idx].c_str());
    int slot_to_remove = hash & (Capacity - 1);
    while(Slots[slot_to_remove].Index != idx)
        slot_to_remove = (slot_to_remove + 1) & (Capacity - 1);
    Slots[slot_to_remove].Index = INVALID_IDX;

    int last_idx = Strings.Size - 1;

    // If the element being removed is not the last one, move the last element to its place
    if(idx != last_idx)
    {
        // Look for the slot that points to the last element
        uint32_t last_hash = Hash::fnv1a_32(Strings[last_idx].c_str());
        int last_slot = last_hash & (Capacity - 1);
        while(Slots[last_slot].Index != last_idx)
            last_slot = (last_slot + 1) & (Capacity - 1);

        Strings[idx].deallocate_self();                                 // Clear the memory of the element being deleted
        memcpy(&Strings[idx], &Strings[last_idx], sizeof(ImString));    // Move the last element to the location of the one being removed
        Slots[last_slot].Index = idx;                                   // Update the slot to point to the new index
    }
    else
        Strings[idx].deallocate_self();

    Strings.Size--;

    // Recovering linear probing chains
    int next_slot = (slot_to_remove + 1) & (Capacity - 1);
    while(Slots[next_slot].Index != INVALID_IDX)
    {
        Slot tmp_slot = Slots[next_slot];
        Slots[next_slot].Index = INVALID_IDX;

        int new_slot = tmp_slot.Hash & (Capacity - 1);
        while(Slots[new_slot].Index != INVALID_IDX)
            new_slot = (new_slot + 1) & (Capacity - 1);
        Slots[new_slot] = tmp_slot;

        next_slot = (next_slot + 1) & (Capacity - 1);
    }

    return true;
}

bool ImStringPool::find_erase(const char* key)
{
    if(!key || Strings.Size == 0)
        return false;

    int idx = find_index(key);
    if(idx == INVALID_IDX)
        return false;

    uint32_t hash = Hash::fnv1a_32(key);
    int slot_to_remove = hash & (Capacity - 1);
    while(Slots[slot_to_remove].Index != idx)
        slot_to_remove = (slot_to_remove + 1) & (Capacity - 1);
    Slots[slot_to_remove].Index = INVALID_IDX;

    int last_idx = Strings.Size - 1;

    // If the element being removed is not the last one, move the last element to its place
    if(idx != last_idx)
    {
        // Look for the slot that points to the last element
        uint32_t last_hash = Hash::fnv1a_32(Strings[last_idx].c_str());
        int last_slot = last_hash & (Capacity - 1);
        while(Slots[last_slot].Index != last_idx)
            last_slot = (last_slot + 1) & (Capacity - 1);

        Strings[idx].deallocate_self();                                 // Clear the memory of the element being deleted
        memcpy(&Strings[idx], &Strings[last_idx], sizeof(ImString));    // Move the last element to the location of the one being removed
        Slots[last_slot].Index = idx;                                   // Update the slot to point to the new index
    }
    else
        Strings[idx].deallocate_self();

    Strings.Size--;

    // Recovering linear probing chains
    int next_slot = (slot_to_remove + 1) & (Capacity - 1);
    while(Slots[next_slot].Index != INVALID_IDX)
    {
        Slot tmp_slot = Slots[next_slot];
        Slots[next_slot].Index = INVALID_IDX;

        int new_slot = tmp_slot.Hash & (Capacity - 1);
        while(Slots[new_slot].Index != INVALID_IDX)
            new_slot = (new_slot + 1) & (Capacity - 1);
        Slots[new_slot] = tmp_slot;

        next_slot = (next_slot + 1) & (Capacity - 1);
    }

    return true;
}

int ImStringPool::insert(const char* key)
{
    if(!key)
        return INVALID_IDX;

    // First check if an index exists for the specified key
    int idx = find_index(key);
    if(idx != INVALID_IDX)
        return idx;

    // Support Load Factor <= 0.5 to avoid long collision chains
    if(Strings.Size * 2 > Capacity)
        grow(Capacity * 2);

    idx = Strings.Size;
    ImVectorPushBack(Strings, key);

    uint32_t hash = Hash::fnv1a_32(key);
    int slot = hash & (Capacity - 1);

    // Be sure to search for the first empty slot using linear probing
    while(Slots[slot].Index != INVALID_IDX)
        slot = (slot + 1) & (Capacity - 1);

    Slots[slot].Hash = hash;
    Slots[slot].Index = idx;

    return idx;
}

int ImStringPool::find_index(const char* key) const
{
    if(!key || Strings.Size == 0)
        return INVALID_IDX;

    uint32_t hash = Hash::fnv1a_32(key);
    int slot = hash & (Capacity - 1);

    while(Slots[slot].Index != INVALID_IDX)
    {
        if(Slots[slot].Hash == hash)
        {
            const ImString& str = Strings[Slots[slot].Index];
            if(strcmp(str.c_str(), key) == 0)
                return Slots[slot].Index;
        }

        slot = (slot + 1) & (Capacity - 1);
    }

    return INVALID_IDX;
}
#endif // #ifndef IMGUI_DISABLE