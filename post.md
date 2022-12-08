Before thinking about implementing an algorithm for finding a pattern in memory, let's first consider how we will get the memory itself. A naiive implementation would simply start at address 0, define some arbitrary chunk size and keep reading chunks of that size (and increasing the address) until it hits something like `UINT32_MAX`. Maybe it would look something like the following:

```c++
const size_t chunk_size = 4096;
std::byte chunk_bytes[chunk_size];

for (uintptr_t addr = 0; addr < UINT32_MAX; addr += chunk_size) {
    if (!read_memory<std::byte>(addr, chunk_bytes, chunk_size)) {
        continue;
    }
    
    // ...
}
```

This works; in fact I used a similar approach in [maniac](https://github.com/fs-c/maniac/) for several years without any issues. But it's still a terrible solution, let me tell you why.

Windows (and modern operating systems in general) divide memory into chunks of some fixed size(s) that are called pages. Each process gets its own set of pages, where the first page always starts at 0. I won't get into the magic that goes on behind the scenes here and leave it at that. Importantly though, each of these pages has its own memory protection flags (read-only, execute, and so on), state (free, reserve, etc.) and type. Windows calls adjacent pages where all of these things are the same a region.

![](https://i.imgur.com/Eus5hCq.png)

_Memory regions of lsass.exe viewed in [Process Hacker 2](https://processhacker.sourceforge.io/downloads.php)._

The previous sketch of an algorithm would frequently attempt to read memory inside a page that has read protection or that isn't even committed. It would also read across page boundaries which compounds the issue (and also decreases performance by a bit). Quite clearly, the more elegant solution would be to read regions as whole chunks instead, and to skip regions which aren't committed or otherwise not readable.

We can use [`VirtualQueryEx`](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualqueryex) to get information about regions. It returns the structure
```c++
typedef struct _MEMORY_BASIC_INFORMATION {
    PVOID  BaseAddress;
    PVOID  AllocationBase;
    DWORD  AllocationProtect;
    WORD   PartitionId;
    SIZE_T RegionSize;
    DWORD  State;
    DWORD  Protect;
    DWORD  Type;
} MEMORY_BASIC_INFORMATION, *PMEMORY_BASIC_INFORMATION;
```

which is exactly what we need. (Documentation on [`MEMORY_BASIC_INFORMATION`](https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-memory_basic_information).) An algorithm for iterating all regions of a given process could thus look like

```c++
// iterate over all regions (subsequent pages with equal attributes) of the process
// with the given handle, call given callback function for each
void map_regions(HANDLE handle, const std::function<void(MEMORY_BASIC_INFORMATION *)> &callback) {
    uintptr_t cur_address = 0;
    // ensure architecture compatibility, UINTPTR_MAX is the largest (data) pointer i.e. address
    // for 32bit this is 0xFFFFFFFF or UINT32_MAX
    const uintptr_t max_address = UINTPTR_MAX;

    while (cur_address < max_address) {
        MEMORY_BASIC_INFORMATION info;

        if (!VirtualQueryEx(handle, reinterpret_cast<void *>(cur_address), &info, sizeof(info))) {
            std::cout << std::format("couldn't query at {}\n", cur_address);

            // can't really recover from this gracefully
            return;
        }

        callback(&info);

        // don't just add region size to current address because we might have
        // missed the actual region boundary
        cur_address = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
    }
}
```

where the given `callback` gets called with a pointer to the current memory information struct for each region of pages, in order of their base addresses. For example, we could now print the base addresses of all regions of a given process with

```c++
// ...

map_regions(handle, [](MEMORY_BASIC_INFORMATION *info) {
    std::cout << std::format("region at {}\n", info->BaseAddress);
});
```

yielding

```
region at 0x0
region at 0x7ffe0000
region at 0x7ffe1000
region at 0x7ffef000
region at 0x7fff0000
region at 0xf65f200000
region at 0xf65f217000
region at 0xf65f220000
region at 0xf65f400000
...
```

for some sample process.
