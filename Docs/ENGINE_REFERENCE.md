# ENGINE_REFERENCE — MapleStory DX11 2D 엔진 코드 레퍼런스

이 문서는 `main` 브랜치에 실제로 존재하는 엔진 소스 코드를 직접 읽고, Phase 0부터
Phase 7.7(+ Phase 7.5+ 최적화)까지 지금까지 구현된 모든 서브시스템이
**어떻게 구현되어 있고 실행 시점에 어떻게 동작하는지**를 정리한 기술 레퍼런스다.
`Docs/ARCHITECTURE.md`가 포트폴리오용 발췌 요약본이라면, 이 문서는 실제 코드 인용을
근거로 한 완전판이다.

각 시스템은 다음 형식을 따른다.

- **무엇을 위한 것인가** — 한 줄 목적
- **실제 코드 인용** — 핵심 구조체/함수, 파일 경로 명시 (모두 `main` 브랜치 실제 소스에서 발췌)
- **단계별 동작 설명** — 그 코드가 런타임에 어떻게 동작하는지
- **공개 API 요약** — 표 또는 목록

인용된 코드는 전부 실제 파일에서 그대로 가져온 것이며, 코드에 없는 동작은 서술하지 않는다.
(단, 실제 코드를 원본 문서화 대비 대조하는 과정에서 CLAUDE.md 로드맵 상 서술과 실제
구현이 어긋나는 지점을 발견한 경우 — 예: `FName::ToString()`의 실제 위치 — 별도로
명시했다.)

---

## 목차

1. [Memory](#memory)
2. [Templates](#templates)
3. [Containers — TArray / TArrayView](#tarray--tarrayview)
4. [Containers — TSparseArray](#tsparsearray)
5. [Containers — TSet / TMap / TMultiMap](#tset--tmap--tmultimap)
6. [Containers — HashFunctions](#hashfunctions)
7. [Math 라이브러리](#math-라이브러리)
8. [String — FString](#fstring)
9. [String — FName / FNamePool](#fname--fnamepool)
10. [String — FText](#ftext)
11. [Logging / Assert 시스템](#logging--assert-시스템)
12. [SmartPointer — TSharedPtr / TWeakPtr / TSharedRef](#스마트-포인터-tsharedptr--tweakptr--tsharedref)
13. [Object 시스템 — UClass / Cast / UObject / AActor](#object-시스템-uclass--cast--uobject--aactor)
14. [Timer 시스템](#timer-시스템-ftimermanager)
15. [Gameplay Ability System (GAS)](#gameplay-ability-system-gas)

---

## Memory

### IAllocator — abstract allocator interface

**Purpose:** Defines the minimal virtual contract (`Malloc`/`Realloc`/`Free`) that every concrete allocator (`FMallocAnsi`, `FMallocBinned`) must implement, so the rest of the engine can be decoupled from the concrete allocation strategy behind a single global pointer.

**Code** (`Engine/Include/Core/Memory/IAllocator.h`):
```cpp
struct IAllocator
{
	virtual void* Malloc(size_t size, uint32 alignment) = 0;
	virtual void* Realloc(void* ptr, size_t newSize, uint32 alignment) = 0;
	virtual void  Free(void* ptr) = 0;
	virtual ~IAllocator() = default;
};
```

**Runtime behavior:** This is a pure abstract base — it has no `.cpp` and does nothing at runtime by itself. It exists purely as a vtable contract. Any call through an `IAllocator*` dispatches virtually to whichever concrete allocator (`FMallocAnsi` or `FMallocBinned`) is currently bound (see `FMemory`/`GMalloc` below). The virtual destructor ensures a derived allocator is destroyed correctly if ever deleted through a base pointer.

**Public API surface:**
| Method | Purpose |
|---|---|
| `virtual void* Malloc(size_t size, uint32 alignment)` | Allocate `size` bytes aligned to `alignment` |
| `virtual void* Realloc(void* ptr, size_t newSize, uint32 alignment)` | Resize/reallocate a previous allocation |
| `virtual void Free(void* ptr)` | Release a previous allocation |
| `virtual ~IAllocator()` | Default virtual dtor |

---

### FMallocAnsi — thin `_aligned_malloc` wrapper

**Purpose:** A minimal `IAllocator` implementation that simply forwards to the CRT's aligned allocation functions; used as the simplest possible allocator (no bucketing/pooling logic of its own).

**Code** (`Engine/Include/Core/Memory/FMallocAnsi.h` and `.cpp`):
```cpp
class FMallocAnsi : public IAllocator
{
public:
	void* Malloc(size_t size, uint32 alignment) override;
	void* Realloc(void* ptr, size_t newSize, uint32 alignment) override;
	void  Free(void* ptr) override;
};
```
```cpp
void* FMallocAnsi::Malloc(size_t size, uint32 alignment)
{
	return _aligned_malloc(size, alignment);
}

void* FMallocAnsi::Realloc(void* ptr, size_t newSize, uint32 alignment)
{
	return _aligned_realloc(ptr, newSize, alignment);
}

void FMallocAnsi::Free(void* ptr)
{
	_aligned_free(ptr);
}
```

**Step-by-step runtime behavior:**
1. `Malloc(size, alignment)` calls straight through to the MSVC CRT `_aligned_malloc`, which internally over-allocates and stores a small header before the returned pointer so it can locate the true block start on free — this bookkeeping is entirely inside the CRT, invisible to this class.
2. `Realloc` similarly forwards to `_aligned_realloc`, which the CRT may implement by growing in place or by allocating new + copying + freeing old, again transparently.
3. `Free` forwards to `_aligned_free`, which uses the CRT's internal header to locate and release the true allocation.
4. There is no bin/pool/tracking logic — every call is a 1:1 passthrough. This makes `FMallocAnsi` effectively the "unoptimized" baseline allocator that `FMallocBinned` replaced (per `CLAUDE.md`'s Phase 7.5+ optimization notes: "현재: `_aligned_malloc` 래핑... 목표: 크기 클래스 버킷 방식").

**Public API surface:**
| Method | Purpose |
|---|---|
| `Malloc(size, alignment)` | `_aligned_malloc(size, alignment)` |
| `Realloc(ptr, newSize, alignment)` | `_aligned_realloc(ptr, newSize, alignment)` |
| `Free(ptr)` | `_aligned_free(ptr)` |

---

### FMallocBinned — size-class bin allocator with 64KB pages

**Purpose:** A bucketed/binned allocator that services small allocations (≤512 bytes, ≤16-byte alignment) from free-lists carved out of 64KB-aligned pages, and falls back to a page-aligned "large allocation" path for anything else — eliminating per-allocation CRT overhead and external fragmentation for small objects.

**Code** (`Engine/Include/Core/Memory/FMallocBinned.h`):
```cpp
static const int32 NUM_BINS = 6;                 // 16, 32, 64, 128, 256, 512
static const int32 MAX_SMALL_SIZE = 512;
static const uint32 BIN_MAX_ALIGN = 16;
static const size_t PAGE_SIZE = 65536;           // 64 KB, pages are PAGE_SIZE-aligned
static const uint32 PAGE_MAGIC = 0xB17EED00u;
static const int32 LARGE_BIN = -1;

struct FFreeBlock { FFreeBlock* m_pNext; };

struct FPageHeader
{
    uint32 m_Magic;
    int32 m_BinIndex;      // 0..NUM_BINS-1, or LARGE_BIN
    size_t m_AllocSize;     // user-requested size (for Realloc copy)
    FPageHeader* m_pNextPage;    // intrusive list of all pages (for shutdown)
};
static const size_t HEADER_SIZE = 32;
```

Bin table initialization (`Engine/Include/Core/Memory/FMallocBinned.cpp`):
```cpp
FMallocBinned::FMallocBinned() : m_pAllPages(nullptr)
{
    int32 size = 16;
    for (int32 i = 0; i < NUM_BINS; i++)
    {
        m_FreeLists[i] = nullptr;
        m_BinBlockSize[i] = size;
        size *= 2;
    }
}
```

Page → header recovery via pointer masking:
```cpp
FMallocBinned::FPageHeader* FMallocBinned::PageOf(void* ptr)
{
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t base = addr & ~(uintptr_t)(PAGE_SIZE - 1);
    return reinterpret_cast<FPageHeader*>(base);
}
```

Growing a bin (page carving):
```cpp
void FMallocBinned::GrowBin(int32 binIndex)
{
    const size_t blockSize = (size_t)m_BinBlockSize[binIndex];
    void* pRaw = _aligned_malloc(PAGE_SIZE, PAGE_SIZE);
    ...
    uint8* pStart = reinterpret_cast<uint8*>(pRaw) + HEADER_SIZE;
    const size_t usable = PAGE_SIZE - HEADER_SIZE;
    const size_t blockCount = usable / blockSize;

    for (size_t i = 0; i < blockCount; i++)
    {
        FFreeBlock* pBlock = reinterpret_cast<FFreeBlock*>(pStart + i * blockSize);
        pBlock->m_pNext = m_FreeLists[binIndex];
        m_FreeLists[binIndex] = pBlock;
    }
}
```

`Malloc`:
```cpp
void* FMallocBinned::Malloc(size_t size, uint32 alignment)
{
    if (size == 0) size = 1;

    if (size <= (size_t)MAX_SMALL_SIZE && alignment <= BIN_MAX_ALIGN)
    {
        const int32 bin = SizeToBin(size);
        if (bin != LARGE_BIN)
        {
            if (!m_FreeLists[bin]) GrowBin(bin);
            FFreeBlock* pBlock = m_FreeLists[bin];
            m_FreeLists[bin] = pBlock->m_pNext;
            return pBlock;
        }
    }
    return AllocateLarge(size, alignment);
}
```

`Free` (pointer-masking to find bin/page, no per-pointer size lookup needed for small blocks):
```cpp
void FMallocBinned::Free(void* ptr)
{
    if (!ptr) return;
    FPageHeader* pHeader = PageOf(ptr);
    check(pHeader->m_Magic == PAGE_MAGIC);
    ...
    if (pHeader->m_BinIndex == LARGE_BIN)
    {
        // unlink from m_pAllPages, then _aligned_free(pHeader)
    }
    const int32 bin = pHeader->m_BinIndex;
    FFreeBlock* pBlock = reinterpret_cast<FFreeBlock*>(ptr);
    pBlock->m_pNext = m_FreeLists[bin];
    m_FreeLists[bin] = pBlock;
}
```

**Step-by-step: `Malloc(size, alignment)`**
1. If `size == 0`, it is bumped to `1` (so a zero-size request still returns a valid, freeable pointer).
2. **Small-path check:** if `size <= 512` and `alignment <= 16`, `SizeToBin(size)` linearly scans `m_BinBlockSize[0..5]` (16, 32, 64, 128, 256, 512) and returns the first index whose block size is `>= size`; otherwise returns `LARGE_BIN`.
3. If a valid bin is found: if the bin's free-list (`m_FreeLists[bin]`) is empty, `GrowBin(bin)` is called to carve a fresh page (see below).
4. The head node of the free-list (`FFreeBlock* pBlock`) is popped off (`m_FreeLists[bin] = pBlock->m_pNext`) and its address is returned directly as the user pointer — an O(1) pop, no header search per-allocation.
5. If size/alignment don't fit a bin, control falls through to `AllocateLarge`.

**Step-by-step: `GrowBin(binIndex)` (page carving)**
1. Allocates one full `PAGE_SIZE` (64KB) block via `_aligned_malloc(PAGE_SIZE, PAGE_SIZE)`, guaranteeing the page's base address is itself 64KB-aligned (checked via `check((addr & (PAGE_SIZE-1)) == 0)`).
2. The first `HEADER_SIZE` (32) bytes of the page become an `FPageHeader`: magic number `PAGE_MAGIC`, the owning `m_BinIndex`, `m_AllocSize = 0` (unused for small bins), and it's linked into the intrusive `m_pAllPages` list (prepended).
3. The remaining `PAGE_SIZE - HEADER_SIZE` bytes are treated as a raw array of `blockSize`-sized slots (`blockCount = usable / blockSize`).
4. Each slot is walked and pushed onto `m_FreeLists[binIndex]` as an `FFreeBlock` node, building the free-list back-to-front (last slot in memory ends up at the tail of the list since each new node is pushed to the front). After this, the bin has a full page's worth of free blocks.

**Step-by-step: `Free(ptr)` (pointer-masking)**
1. `PageOf(ptr)` masks off the low `PAGE_SIZE-1` bits of the pointer (`addr & ~(PAGE_SIZE-1)`) to jump directly to the page's base address — this works because every page (small-bin or large) is allocated 64KB-aligned, so *any* pointer inside it shares the same masked base as the `FPageHeader` sitting at offset 0. This is how the allocator recovers "which bin does this belong to" without a separate lookup table.
2. The header's `m_Magic` is checked against `PAGE_MAGIC` (corruption/foreign-pointer guard), and `m_BinIndex` is validated as either `LARGE_BIN` or a valid bin index.
3. If `LARGE_BIN`: the page header is unlinked from the singly-linked `m_pAllPages` list by cursor-walking to find the matching node, then the whole page is released via `_aligned_free`.
4. Otherwise (small bin): the block is simply pushed back onto `m_FreeLists[bin]` (`pBlock->m_pNext = m_FreeLists[bin]; m_FreeLists[bin] = pBlock`) — no memory is returned to the OS; the page stays owned by the allocator for reuse. Note pages are never returned to the small-bin free pool cooperatively across bins, and there's no bin-empty-page release logic — pages live until the allocator itself is destroyed.

**Step-by-step: `AllocateLarge(size, alignment)` (large fallback)**
1. `alignment` is clamped up to at least `BIN_MAX_ALIGN` (16), and `check`ed to be `< PAGE_SIZE`.
2. Required bytes = `HEADER_SIZE + alignment + size` (header, plus worst-case alignment slack, plus payload), then rounded up to the next multiple of `PAGE_SIZE` (`totalSize`).
3. `_aligned_malloc(totalSize, PAGE_SIZE)` allocates a 64KB-page-aligned block sized to a whole number of pages.
4. An `FPageHeader` is written at the front with `m_BinIndex = LARGE_BIN` and `m_AllocSize = size` (the exact user-requested size, needed later for `Realloc`'s copy-size calculation since large blocks don't have a fixed bin size). The page is linked into `m_pAllPages`.
5. The payload pointer is computed as `pRaw + HEADER_SIZE`, then rounded up to the requested `alignment` boundary, and that aligned address is returned. This wastes up to `alignment - 1` bytes of padding between the header and the payload.

**Step-by-step: `Realloc(ptr, newSize, alignment)`**
1. `ptr == nullptr` → delegates entirely to `Malloc(newSize, alignment)`.
2. `newSize == 0` → frees `ptr` and returns `nullptr`.
3. Otherwise: locates the page header via `PageOf(ptr)`, validates the magic, and determines `oldSize` — for `LARGE_BIN` it's the stored `m_AllocSize`; for a small bin it's `m_BinBlockSize[binIndex]` (the bin's fixed slot size, not the original requested size — small bins don't track exact request sizes).
4. Always allocates a brand-new block via `Malloc(newSize, alignment)` (there is no in-place growth), copies `min(oldSize, newSize)` bytes via `FMemory::Memcpy`, frees the old block, and returns the new pointer. This is a copy-based realloc with no shrink/grow-in-place optimization, even within the same bin.

**Public API surface:**
| Member | Purpose |
|---|---|
| `Malloc(size, alignment)` | Bin-based small alloc or large-page fallback |
| `Realloc(ptr, newSize, alignment)` | Always copy-based (new alloc + memcpy + free) |
| `Free(ptr)` | Pointer-masks to page header, frees large or returns block to bin free-list |
| `NUM_BINS`, `MAX_SMALL_SIZE`, `BIN_MAX_ALIGN`, `PAGE_SIZE`, `PAGE_MAGIC`, `LARGE_BIN` | Public constants describing bin/page geometry |
| `SizeToBin`, `GrowBin`, `AllocateLarge`, `PageOf` *(private)* | Internal bin lookup, page carving, large-path, header recovery |

---

### FMemory — global allocator entry points

**Purpose:** A static facade over the process-wide `GMalloc` pointer, providing the engine-wide allocation API (and raw memory ops) that all other systems call instead of touching `IAllocator`/CRT directly.

**Code** (`Engine/Include/Core/Memory/FMemory.h`):
```cpp
extern IAllocator* GMalloc;

class FMemory
{
public:
	static void  InitMemory();
	static void* Malloc(size_t size, uint32 alignment = 16);
	static void* Realloc(void* ptr, size_t newSize, uint32 alignment = 16);
	static void  Free(void* ptr);
	static void* Memcpy(void* dest, const void* src, size_t count);
	static void* Memset(void* dest, int32 val, size_t count);
	static void* Memmove(void* dest, const void* src, size_t count);
	static void  Memzero(void* dest, size_t count);
};
```

`Engine/Include/Core/Memory/FMemory.cpp`:
```cpp
void FMemory::InitMemory()
{
	static FMallocBinned BinnedAllocator;
	GMalloc = &BinnedAllocator;
}

void* FMemory::Malloc(size_t size, uint32 alignment)
{
	check(GMalloc);
	return GMalloc->Malloc(size, alignment);
}
```

**Step-by-step runtime behavior:**
1. `InitMemory()` must be called once at engine startup. It constructs a **function-local static** `FMallocBinned` (so it's lazily constructed on first call and lives for the program's duration) and points the global `GMalloc` at it. This is the single place in the codebase that decides the concrete allocator strategy — swapping to `FMallocAnsi` would only require changing this one function.
2. `Malloc`/`Realloc`/`Free` are thin static wrappers: each does `check(GMalloc)` (hard-asserts the allocator was initialized) and then virtually dispatches to `GMalloc->Malloc/Realloc/Free`. This is the polymorphism seam — every caller in the engine goes through `IAllocator`'s vtable via this single global pointer.
3. `Memcpy`/`Memset`/`Memmove` are direct passthroughs to the CRT `memcpy`/`memset`/`memmove` (not allocator-related, just centralizing raw memory ops in one class per the "no direct CRT calls scattered around" convention).
4. `Memzero(dest, count)` is `memset(dest, 0, count)` — a convenience wrapper.

**Public API surface:**
| Member | Purpose |
|---|---|
| `GMalloc` (global `extern IAllocator*`) | The active allocator instance, set by `InitMemory()` |
| `InitMemory()` | Constructs the static `FMallocBinned` and binds `GMalloc` |
| `Malloc(size, alignment=16)` | Dispatches to `GMalloc->Malloc` |
| `Realloc(ptr, newSize, alignment=16)` | Dispatches to `GMalloc->Realloc` |
| `Free(ptr)` | Dispatches to `GMalloc->Free` |
| `Memcpy/Memset/Memmove/Memzero` | Thin CRT `mem*` wrappers |

---

### MemoryOverride — global `new`/`delete` routing

**Purpose:** Overrides the global `operator new`/`operator delete` (scalar, array, and sized-delete forms) so that *every* engine-wide `new`/`delete` call transparently routes through `FMemory`/`GMalloc`, and (in Debug) through `FMemoryTracker`.

**Code** (`Engine/Include/Core/Memory/MemoryOverride.cpp`):
```cpp
IAllocator* GMalloc = nullptr;

void* operator new(size_t size)
{
#ifdef _DEBUG
	FMemoryTracker::OnAlloc(size);
#endif
	return FMemory::Malloc(size);
}

void operator delete(void* ptr) noexcept
{
#ifdef _DEBUG
	FMemoryTracker::OnFree();
#endif
	FMemory::Free(ptr);
}

void operator delete(void* ptr, size_t) noexcept
{
#ifdef _DEBUG
	FMemoryTracker::OnFree();
#endif
	FMemory::Free(ptr);
}
```
(Array forms `operator new[]`/`operator delete[]` mirror the scalar ones exactly.)

**Step-by-step runtime behavior:**
1. This `.cpp` is also where the actual storage for the `extern IAllocator* GMalloc` declared in `FMemory.h` is defined (`IAllocator* GMalloc = nullptr;`) — it starts null until `FMemory::InitMemory()` runs.
2. Any `new T(...)` in the engine resolves to this overridden `operator new(size_t)` (since RTTI/exceptions are disabled and no custom placement-new is used for a plain `new`). In `_DEBUG` builds it first calls `FMemoryTracker::OnAlloc(size)` to bump the tracker's allocation counters, then calls `FMemory::Malloc(size)` (using `FMemory`'s default 16-byte alignment) which ultimately dispatches to `GMalloc->Malloc`.
3. `delete ptr` resolves to `operator delete(void*)`, which (Debug) calls `FMemoryTracker::OnFree()` then `FMemory::Free(ptr)`.
4. The two **sized-delete** overloads (`operator delete(void*, size_t)` / `operator delete[](void*, size_t)`) are provided because C++14+ compilers may prefer the sized-delete overload when it's available (part of the mandatory global operator-delete overload set) — both are implemented identically to the unsized versions, just ignoring the extra `size_t` parameter, since `FMallocBinned::Free` recovers all needed size/bin info itself via pointer masking rather than relying on the size passed by the compiler.
5. Because this happens *before* `FMemory::InitMemory()` runs for any static/global object constructed before `main`, any such early `new` would dereference a null `GMalloc` and hit the `check(GMalloc)` assert in `FMemory::Malloc` — i.e., `InitMemory()` must run before any heap-allocating global/static construction that isn't itself statically initialized.

**Public API surface:**
| Symbol | Purpose |
|---|---|
| `IAllocator* GMalloc` | Definition (storage) of the global allocator pointer |
| `operator new(size_t)` / `operator new[](size_t)` | Routes to `FMemoryTracker::OnAlloc` (Debug) + `FMemory::Malloc` |
| `operator delete(void*)` / `operator delete[](void*)` | Routes to `FMemoryTracker::OnFree` (Debug) + `FMemory::Free` |
| `operator delete(void*, size_t)` / `operator delete[](void*, size_t)` | Sized-delete overloads, identical behavior, `size_t` unused |

---

### FPoolAllocator — fixed-block-size pool

**Purpose:** A fixed-size-block allocator over one contiguous buffer, intended (per `CLAUDE.md`) for bulk homogeneous allocations like monsters or particles, using an intrusive singly-linked free-list embedded in the unused blocks themselves.

**Code** (`Engine/Include/Core/Memory/FPoolAllocator.h`):
```cpp
class FPoolAllocator
{
public:
	void  Init(size_t blockSize, uint32 blockCount);
	void* Acquire();
	void  Release(void* ptr);
	void  Destroy();

private:
	void* m_pMemory = nullptr;
	void* m_pFreeList = nullptr;
	size_t  m_BlockSize = 0;
	uint32  m_BlockCount = 0;
};
```

`Engine/Include/Core/Memory/FPoolAllocator.cpp`:
```cpp
void FPoolAllocator::Init(size_t blockSize, uint32 blockCount)
{
	m_BlockSize = blockSize > sizeof(void*) ? blockSize : sizeof(void*);
	m_BlockCount = blockCount;
	m_pMemory = FMemory::Malloc(m_BlockSize * m_BlockCount, 16);

	uint8* pBlock = static_cast<uint8*>(m_pMemory);
	for (uint32 i = 0; i < m_BlockCount - 1; i++)
	{
		void** pNext = reinterpret_cast<void**>(pBlock + i * m_BlockSize);
		*pNext = pBlock + (i + 1) * m_BlockSize;
	}

	void** pLast = reinterpret_cast<void**>(pBlock + (m_BlockCount - 1) * m_BlockSize);
	*pLast = nullptr;

	m_pFreeList = m_pMemory;
}

void* FPoolAllocator::Acquire()
{
	check(m_pFreeList && "[FPoolAllocator] Pool exhausted");
	void* pBlock = m_pFreeList;
	m_pFreeList = *reinterpret_cast<void**>(m_pFreeList);
	return pBlock;
}

void FPoolAllocator::Release(void* ptr)
{
	check(ptr);
	*reinterpret_cast<void**>(ptr) = m_pFreeList;
	m_pFreeList = ptr;
}
```

**Step-by-step runtime behavior:**
1. `Init(blockSize, blockCount)`: clamps `m_BlockSize` up to at least `sizeof(void*)` (so a free block can always hold a next-pointer), then allocates one contiguous buffer of `blockSize * blockCount` bytes via `FMemory::Malloc` (16-byte aligned).
2. It then walks the buffer block-by-block, writing into the *first `sizeof(void*)` bytes of each block* the address of the next block, forming a forward-linked free-list from block 0 through block `N-2`; the last block's link is set to `nullptr` (list terminator). `m_pFreeList` is set to point at block 0 (the head of this list).
3. `Acquire()`: `check`s the free-list isn't empty (pool exhaustion is a hard assert, not a graceful failure — matching the "no exceptions" engine philosophy), pops the head block off the list (reads the block's embedded next-pointer to advance `m_pFreeList`), and returns that block's address to the caller. O(1), no scanning.
4. `Release(ptr)`: pushes the returned block back onto the head of the free-list by writing the current `m_pFreeList` into the first `sizeof(void*)` bytes of `ptr`, then making `ptr` the new head. O(1). This is destructive to whatever data was in the block — callers must ensure the object's destructor has already run (or that overwriting the object's first pointer-width bytes is safe) before calling `Release`.
5. `Destroy()`: frees the whole backing buffer via `FMemory::Free(m_pMemory)` and zeroes out the bookkeeping fields. Note this does **not** call destructors on any live objects still "acquired" — it's purely a raw-memory pool, with no ownership/lifetime tracking of what's stored in each block.

**Public API surface:**
| Method | Purpose |
|---|---|
| `Init(blockSize, blockCount)` | Allocates the backing buffer and builds the intrusive free-list |
| `Acquire()` | Pops and returns one free block (asserts if pool exhausted) |
| `Release(void* ptr)` | Pushes a block back onto the free-list |
| `Destroy()` | Frees the backing buffer, resets state |

---

### FStackAllocator — frame/linear (bump) allocator

**Purpose:** A linear "bump-pointer" allocator over one preallocated buffer intended for per-frame temporary allocations that are all discarded together via a single `Reset()` (no per-allocation `Free`).

**Code** (`Engine/Include/Core/Memory/FStackAllocator.h`):
```cpp
class FStackAllocator
{
public:
	void  Init(size_t capacity);
	void* Alloc(size_t size, uint32 alignment = 16);
	void  Reset();
	void  Destroy();

private:
	uint8* m_pBuffer = nullptr;
	size_t  m_Capacity = 0;
	size_t  m_Offset = 0;
};
```

`Engine/Include/Core/Memory/FStackAllocator.cpp`:
```cpp
void FStackAllocator::Init(size_t capacity)
{
	m_Capacity = capacity;
	m_Offset = 0;
	m_pBuffer = static_cast<uint8*>(FMemory::Malloc(capacity, 16));
}

void* FStackAllocator::Alloc(size_t size, uint32 alignment)
{
	size_t aligned = (m_Offset + alignment - 1) & ~(static_cast<size_t>(alignment) - 1);
	check(aligned + size <= m_Capacity && "[FStackAllocator] Out of capacity");
	void* ptr = m_pBuffer + aligned;
	m_Offset = aligned + size;
	return ptr;
}

void FStackAllocator::Reset()
{
	m_Offset = 0;
}
```

**Step-by-step runtime behavior:**
1. `Init(capacity)` allocates one buffer of `capacity` bytes (16-byte aligned) via `FMemory::Malloc` and sets the bump-cursor `m_Offset` to `0`.
2. `Alloc(size, alignment)`: rounds the current `m_Offset` up to the requested `alignment` boundary using the standard `(offset + align - 1) & ~(align - 1)` bit-mask trick (requires `alignment` be a power of two, unchecked). It `check`s that the aligned offset plus `size` doesn't exceed `m_Capacity` (hard-assert, no graceful growth — this is a fixed-capacity linear allocator). It then returns `m_pBuffer + aligned` and advances `m_Offset` to `aligned + size`. There is no per-allocation free — objects allocated this way are expected to have trivial/no destructors that need calling, or the caller manages destruction separately, since only a raw bump-pointer is tracked.
3. `Reset()` simply sets `m_Offset` back to `0`, instantly "freeing" everything allocated since the last reset (or since `Init`) by discarding the cursor — no memory is actually returned to the OS or zeroed; the next `Alloc` calls will just overwrite the old contents. This is the intended per-frame idiom: `Reset()` once per frame, `Alloc()` many times during the frame.
4. `Destroy()` frees the backing buffer and zeroes the bookkeeping fields.

Note: the `.cpp` file's inline comments are encoded as garbled/mojibake text in the source (likely UTF-8 Korean comments misread as another codepage) — this doesn't affect compiled behavior, just documentation readability in that file.

**Public API surface:**
| Method | Purpose |
|---|---|
| `Init(capacity)` | Allocates the backing buffer, resets cursor to 0 |
| `Alloc(size, alignment=16)` | Bump-allocates `size` bytes at the next aligned offset (asserts on overflow) |
| `Reset()` | Rewinds the cursor to 0, discarding all allocations since last reset |
| `Destroy()` | Frees the backing buffer |

---

### FMemoryTracker — debug-only leak/allocation tracking

**Purpose:** A `_DEBUG`-only global allocation/free counter hooked into the overridden `operator new`/`delete` to detect leaks (mismatched alloc/free counts) at shutdown; compiled out entirely in non-debug builds.

**Code** (`Engine/Include/Core/Memory/FMemoryTracker.h`):
```cpp
#ifdef _DEBUG
class FMemoryTracker
{
public:
	static void OnAlloc(size_t size);
	static void OnFree();
	static void ReportLeaks();

private:
	static int64  m_AllocCount;
	static int64  m_FreeCount;
	static int64  m_TotalAllocBytes;
};
#endif
```

`Engine/Include/Core/Memory/FMemoryTracker.cpp`:
```cpp
void FMemoryTracker::ReportLeaks()
{
	int64 leaked = m_AllocCount - m_FreeCount;
	if (leaked > 0)
	{
		wchar_t buf[256];
		swprintf_s(buf, L"[MemoryTracker] LEAK detected: %lld alloc, %lld free, %lld leaked, %lld bytes total\n",
			m_AllocCount, m_FreeCount, leaked, m_TotalAllocBytes);
		OutputDebugStringW(buf);
		wprintf(buf);
	}
	else
	{
		wprintf(L"[MemoryTracker] No leaks detected (%lld alloc / %lld free)\n", m_AllocCount, m_FreeCount);
	}
}
```

**Step-by-step runtime behavior:**
1. The whole class (declaration and definition) is wrapped in `#ifdef _DEBUG` — in Release builds it doesn't exist at all, and the calls to it in `MemoryOverride.cpp` are compiled out by the same `#ifdef` guard there, so there is zero tracking overhead in Release.
2. `OnAlloc(size)` is called from every overridden global `operator new`/`new[]` before the actual `FMemory::Malloc` call; it increments `m_AllocCount` and adds `size` to `m_TotalAllocBytes`.
3. `OnFree()` is called from every overridden `operator delete`/`delete[]` (all four overloads, sized and unsized) before the actual `FMemory::Free` call; it increments `m_FreeCount`. Note it does **not** know the freed size (the sized-delete's `size_t` parameter is discarded even here), so `m_TotalAllocBytes` only ever tracks cumulative bytes allocated, not net/current bytes.
4. `ReportLeaks()` is intended to be called at shutdown. It computes `leaked = m_AllocCount - m_FreeCount`. If positive, it formats a leak-warning message (counts and total allocated bytes) into a `wchar_t` buffer via `swprintf_s`, then emits it twice: once via `OutputDebugStringW` (visible in a debugger's Output window) and once via `wprintf` (visible on stdout/console). If `leaked <= 0`, it prints a "No leaks detected" message via `wprintf` only.
5. Important caveat: this only tracks **counts**, not individual pointers/call-sites — it cannot report *which* allocation leaked, only whether the alloc/free counts are imbalanced and how many bytes were allocated in total over the process lifetime.

**Public API surface (Debug builds only):**
| Method | Purpose |
|---|---|
| `OnAlloc(size)` | Increments `m_AllocCount`, adds to `m_TotalAllocBytes` |
| `OnFree()` | Increments `m_FreeCount` |
| `ReportLeaks()` | Compares alloc/free counts and prints a leak report to debugger output + stdout |

---


---

## Templates

### TypeTraits.h — compiler-intrinsic-based type trait library

**Purpose:** A hand-rolled, STL-free set of compile-time type traits (integral-constant wrapper, same-type check, reference/CV stripping, pointer detection, POD/trivial/enum/class checks via compiler intrinsics, `TEnableIf`/`TConditional` SFINAE helpers, `TDecay`, and a `TIsBaseOf` implemented from scratch via SFINAE overload resolution).

**Code** (`Engine/Include/Core/Templates/TypeTraits.h`):
```cpp
template<typename T, T Val>
struct TIntegralConstant
{
	static constexpr T Value = Val;
	using ValueType = T;
	using Type = TIntegralConstant;
	constexpr operator ValueType() const noexcept { return Value; }
};

using FTrueType = TIntegralConstant<bool, true>;
using FFalseType = TIntegralConstant<bool, false>;

template<typename A, typename B> struct TIsSame : FFalseType {};
template<typename T> struct TIsSame<T, T> : FTrueType {};

template<typename T> struct TIsPointer : FFalseType {};
template<typename T> struct TIsPointer<T*> : FTrueType {};
template<typename T> struct TIsPointer<T* const> : FTrueType {};

template<typename T> struct TIsPOD : TIntegralConstant<bool, __is_pod(T)> {};
template<typename T> struct TIsTriviallyCopyable : TIntegralConstant<bool, __is_trivially_copyable(T)> {};
template<typename T> struct TIsEnum : TIntegralConstant<bool, __is_enum(T)> {};
template<typename T> struct TIsClass : TIntegralConstant<bool, __is_class(T)> {};

template<bool Condition, typename T = void> struct TEnableIf {};
template<typename T> struct TEnableIf<true, T> { using Type = T; };

template<bool Cond, typename TrueT, typename FalseT> struct TConditional { using Type = FalseT; };
template<typename TrueT, typename FalseT>            struct TConditional<true, TrueT, FalseT> { using Type = TrueT; };

template<typename T>
struct TDecay { using Type = typename TRemoveCV<typename TRemoveReference<T>::Type>::Type; };

namespace UObjectPrivate
{
	template<typename Base, typename Derived>
	struct TIsBaseOfHelper
	{
		static char Test(const Base*);
		static char (&Test(...))[2];
		static constexpr bool Value = sizeof(Test(static_cast<const Derived*>(nullptr))) == sizeof(char);
	};
}

template<typename Base, typename Derived>
struct TIsBaseOf : TIntegralConstant<bool, UObjectPrivate::TIsBaseOfHelper<Base, Derived>::Value> {};
```

**Step-by-step: what happens at compile time when this code is used**
1. `TIntegralConstant<T, Val>` is the foundation: it wraps a compile-time constant of type `T` as a `static constexpr` member `Value`, and provides an implicit `constexpr operator ValueType()`, so any `TIntegralConstant`-derived trait can be used directly as a boolean in `if constexpr` or `static_assert` contexts. `FTrueType`/`FFalseType` are the boolean specializations that virtually every other trait below inherits from.
2. `TIsSame<A, B>` uses partial specialization: the primary template inherits `FFalseType`; the specialization `TIsSame<T, T>` (only matches when both template args are identical) inherits `FTrueType`. The compiler picks the more specialized match when `A == B`.
3. `TIsPointer<T>` similarly defaults to `FFalseType`, with partial specializations for `T*` and `T* const` matching and yielding `FTrueType` — this is pure pattern-matching over the type, no runtime code generated.
4. `TIsPOD`, `TIsTriviallyCopyable`, `TIsEnum`, `TIsClass` all delegate directly to MSVC/Clang **compiler builtins** (`__is_pod`, `__is_trivially_copyable`, `__is_enum`, `__is_class`) — these traits cannot be implemented in portable C++ without such intrinsics, so the library leans on the compiler here rather than reimplementing them from scratch. These feed into things like `TArray`'s POD branch optimization mentioned in `CLAUDE.md` (`if constexpr` dispatch on `TIsPOD<T>::Value` to skip constructor/destructor calls for trivial types).
5. `TEnableIf<Condition, T>`: the primary template has **no** `Type` member at all when `Condition` is false; only the `TEnableIf<true, T>` specialization defines `::Type`. Used in a function's return type or template parameter, referencing `TEnableIf<false, X>::Type` triggers SFINAE (substitution failure is not an error) — the overload silently drops out of the overload set instead of hard-erroring, which is how conditional-overload dispatch is achieved without `if constexpr` at the declaration level.
6. `TConditional<Cond, TrueT, FalseT>`: compile-time ternary — the primary template picks `FalseT`, the `<true, ...>` partial specialization picks `TrueT`. This is exactly what `TAnd`/`TOr` (in `AndOrNot.h`) use internally to select between continuing the recursion or short-circuiting.
7. `TDecay<T>` composes `TRemoveReference<T>::Type` and then `TRemoveCV<...>::Type`, replicating `std::decay`'s reference/cv-stripping behavior (though notably *not* replicating `std::decay`'s array-to-pointer/function-to-pointer decay, since there's no such specialization shown here — only the CV/reference part is implemented).
8. `TIsBaseOf<Base, Derived>` is the most involved: it's implemented via classic **SFINAE overload resolution**, no compiler intrinsic. `TIsBaseOfHelper::Test` is overloaded: one overload accepts `const Base*` exactly (returns `char`, size 1), the other is a C-style varargs catch-all `Test(...)` (returns a reference to `char[2]`, size 2). When called with a `const Derived*` (via `static_cast<const Derived*>(nullptr)`), the compiler prefers the `const Base*` overload **only if** `Derived*` is implicitly convertible to `Base*` (i.e., `Derived` publicly/unambiguously derives from `Base`, or `Base == Derived`); otherwise only the varargs catch-all can bind. `sizeof(Test(...))` is then `1` (true) or `2` (false), computed entirely at compile time with no function actually called at runtime — `Value` is a `constexpr bool` derived from that `sizeof` comparison. This lives in a `UObjectPrivate` namespace, hinting it was purpose-built to support the engine's RTTI-free `Cast<T>()`/`UClass` system (Phase 7 per `CLAUDE.md`) where compile-time base/derived relationships need verifying without `dynamic_cast`.

**Public API surface:**
| Trait | Meaning |
|---|---|
| `TIntegralConstant<T,Val>` / `FTrueType` / `FFalseType` | Compile-time constant wrapper base |
| `TIsSame<A,B>` | `true` iff `A` and `B` are the exact same type |
| `TRemoveReference<T>`, `TRemoveConst<T>`, `TRemoveVolatile<T>`, `TRemoveCV<T>` | Strip `&`/`&&`, `const`, `volatile` |
| `TIsPointer<T>`, `TRemovePointer<T>` | Detect / strip a pointer type |
| `TIsLValueReference<T>`, `TIsRValueReference<T>` | Detect `&` vs `&&` |
| `TIsPOD<T>`, `TIsTriviallyCopyable<T>`, `TIsEnum<T>`, `TIsClass<T>` | Compiler-intrinsic-backed trivial/enum/class checks |
| `TEnableIf<Cond,T=void>` | SFINAE gate; `::Type` exists only if `Cond` is true |
| `TConditional<Cond,TrueT,FalseT>` | Compile-time ternary type selection |
| `TDecay<T>` | Reference + CV stripped type |
| `TIsBaseOf<Base,Derived>` | SFINAE-based base/derived relationship check |

---

### Utility.h — MoveTemp / Forward / Swap

**Purpose:** STL-free replacements for `std::move`, `std::forward`, and `std::swap`, built directly on the `TRemoveReference` trait above.

**Code** (`Engine/Include/Core/Templates/Utility.h`):
```cpp
template<typename T>
inline typename TRemoveReference<T>::Type&& MoveTemp(T&& Obj) noexcept
{
	return static_cast<typename TRemoveReference<T>::Type&&>(Obj);
}

template<typename T>
inline T&& Forward(typename TRemoveReference<T>::Type& Obj) noexcept
{
	return static_cast<T&&>(Obj);
}

template<typename T>
inline T&& Forward(typename TRemoveReference<T>::Type&& Obj) noexcept
{
	return static_cast<T&&>(Obj);
}

template<typename T>
inline void Swap(T& A, T& B) noexcept
{
	T Tmp = MoveTemp(A);
	A = MoveTemp(B);
	B = MoveTemp(Tmp);
}
```

**Step-by-step runtime/compile-time behavior:**
1. `MoveTemp(T&& Obj)`: `T` is deduced via a forwarding/universal reference, so it can bind to lvalues (`T` deduced as `U&`) or rvalues (`T` deduced as `U`). Regardless of what binds, the body unconditionally `static_cast`s to `TRemoveReference<T>::Type&&` — an rvalue reference to the unqualified `T`. This strips lvalue-ness unconditionally, exactly matching `std::move`'s semantics: it doesn't itself move anything, it just produces an rvalue-typed expression that subsequent move-constructors/move-assignment operators will bind to.
2. `Forward<T>(Obj)` has two overloads, mirroring `std::forward`'s lvalue/rvalue overload pair: the first takes `TRemoveReference<T>::Type&` (binds an lvalue), the second takes `TRemoveReference<T>::Type&&` (binds an rvalue), and both cast to `T&&`. When used inside a forwarding-reference function template as `Forward<T>(arg)`, if the caller passed an lvalue, `T` is deduced as `U&`, so `T&&` collapses (reference collapsing) to `U&` — an lvalue is forwarded as an lvalue. If the caller passed an rvalue, `T` is deduced as `U`, so `T&&` is `U&&` — forwarded as an rvalue. This is the standard perfect-forwarding idiom, reimplemented without `<utility>`.
3. `Swap(T& A, T& B)`: does the textbook three-move swap — moves `A` into a temporary `Tmp` (invokes `T`'s move constructor via `MoveTemp`), move-assigns `B` into `A`, then move-assigns `Tmp` into `B`. It relies on `T` having a move constructor and move-assignment operator (or falls back to copy semantics if those aren't user-provided/available, per normal C++ overload resolution) — there's no `TIsTriviallyCopyable`-based memcpy fast path here (unlike what `TArray`'s POD optimization does elsewhere per `CLAUDE.md`).

**Public API surface:**
| Function | Purpose |
|---|---|
| `MoveTemp(T&& Obj)` | Casts to an rvalue reference (replaces `std::move`) |
| `Forward<T>(Obj)` (2 overloads) | Perfect-forwarding cast (replaces `std::forward`) |
| `Swap(T& A, T& B)` | Three-move swap of two objects |

---

### AndOrNot.h — variadic compile-time boolean combinators

**Purpose:** Variadic-template compile-time logical AND/OR/NOT over a pack of trait types (each exposing a `::Value`), built via recursive `TConditional` short-circuiting — a common piece of trait-composition plumbing (e.g., combining several `TIsX<T>::Value` checks into a single condition for `TEnableIf`).

**Code** (`Engine/Include/Core/Templates/AndOrNot.h`):
```cpp
// TAnd: all true → true (empty pack = true, short-circuits)
template<typename... Types> struct TAnd;
template<>                  struct TAnd<> : FTrueType {};
template<typename First, typename... Rest>
struct TAnd<First, Rest...>
	: TConditional<First::Value, TAnd<Rest...>, FFalseType>::Type {
};

// TOr: any true → true (empty pack = false, short-circuits)
template<typename... Types> struct TOr;
template<>                  struct TOr<> : FFalseType {};
template<typename First, typename... Rest>
struct TOr<First, Rest...>
	: TConditional<First::Value, FTrueType, TOr<Rest...>>::Type {
};

// TNot: logical negation
template<typename T>
struct TNot : TIntegralConstant<bool, !T::Value> {};
```

**Step-by-step: what happens when `TAnd<A, B, C>::Value` is evaluated**
1. `TAnd<A, B, C>` matches the variadic partial specialization with `First = A`, `Rest = {B, C}`. It inherits from `TConditional<A::Value, TAnd<B, C>, FFalseType>::Type`.
2. If `A::Value` is `false`, `TConditional` selects `FalseT = FFalseType`, so `TAnd<A,B,C>` directly inherits `FFalseType` — critically, **`TAnd<B, C>` is never instantiated** in this branch (it only appears as an uninstantiated template argument to `TConditional`, and `TConditional`'s primary/specialization only ever names `Type` as one of its two arguments without instantiating the other for evaluation... note: both branches *are* named as template arguments so the compiler must at least form the type `TAnd<Rest...>`, but its base-class-list/body is only instantiated when actually inherited from). This gives genuine short-circuit *evaluation* of `::Value` (the recursion stops being *evaluated* further, since `TAnd<B,C>` is only ever used as an unevaluated template argument once `A::Value` is false, and its own `Value` need not be computed).
3. If `A::Value` is `true`, `TConditional` selects `TrueT = TAnd<Rest...>`, so `TAnd<A,B,C>` inherits from `TAnd<B,C>`, recursing — this repeats until either a `false` is hit (short-circuit to `FFalseType`) or the pack is exhausted, hitting the `TAnd<>` base case which is `FTrueType` (vacuously true for an empty pack, standard convention for AND).
4. `TOr<A,B,C...>` is the mirror image: on `First::Value == true` it immediately inherits `FTrueType` (short-circuit true), otherwise recurses into `TOr<Rest...>`; the empty-pack base case `TOr<>` is `FFalseType` (vacuously false for OR).
5. `TNot<T>` is much simpler — no recursion, just `TIntegralConstant<bool, !T::Value>`, directly negating whatever trait `T` was passed.
6. In all three, the result trait (`TAnd<...>`, `TOr<...>`, `TNot<T>`) is itself usable anywhere a `TIntegralConstant`-derived type is expected (e.g., as the `Condition` in `TEnableIf<TAnd<TIsPOD<T>, TNot<TIsPointer<T>>>::Value, X>`), since it transitively inherits the `constexpr operator ValueType()` from `TIntegralConstant`.

**Public API surface:**
| Trait | Meaning |
|---|---|
| `TAnd<Types...>` | `true` iff every `Types::Value` is `true`; empty pack → `true` |
| `TOr<Types...>` | `true` iff any `Types::Value` is `true`; empty pack → `false` |
| `TNot<T>` | `!T::Value` |

---

### TResult.h — `Ok`/`Err` result type for exception-free error propagation

**Purpose:** A `std::expected`/`Result`-style value-or-error wrapper used as the engine's exception-free error propagation strategy (per `CLAUDE.md`'s Phase 5.5: "예외 없는 에러 전파 전략 (`TResult<T,E>` 패턴)"), defaulting its error type to the engine-wide `EEngineError` enum.

**Code** (`Engine/Include/Core/Templates/TResult.h`):
```cpp
enum class EEngineError : uint8
{
    None = 0,
    FileNotFound,
    OutOfMemory,
    InvalidArgument,
    Unknown
};

template<typename T, typename E = EEngineError>
class TResult
{
private:
    T m_Value;
    E m_Error;
    bool m_bOk;

    TResult() : m_Value(), m_Error(), m_bOk(false) {}

public:
    static TResult Ok(const T& Value)
    {
        TResult R;
        R.m_Value = Value;
        R.m_bOk = true;
        return R;
    }

    static TResult Ok(T&& Value)
    {
        TResult R;
        R.m_Value = static_cast<T&&>(Value);
        R.m_bOk = true;
        return R;
    }

    static TResult Fail(E Error = E{})
    {
        TResult R;
        R.m_Error = Error;
        R.m_bOk = false;
        return R;
    }

    bool IsOk() const { return m_bOk; }
    bool IsErr() const { return !m_bOk; }

    const T& GetValue() const { check(m_bOk); return m_Value; }
    T& GetValue() { check(m_bOk); return m_Value; }

    E GetError() const { check(!m_bOk); return m_Error; }
};
```

**Step-by-step runtime behavior:**
1. The default constructor `TResult()` is `private` — the only way to construct a `TResult<T,E>` from outside the class is through the two static factory functions `Ok(...)` and `Fail(...)`. This forces every result to be explicitly tagged as success or failure at creation, rather than left in an ambiguous default state.
2. `Ok(const T& Value)` builds a default-constructed `TResult` (which zero/default-inits `m_Value`, default-inits `m_Error`, and sets `m_bOk = false`), then copies `Value` into `m_Value` and flips `m_bOk = true` before returning it by value (relying on the compiler's copy/move elision or the implicit move constructor for the return).
3. `Ok(T&& Value)` is the move-optimized overload: same as above but uses `static_cast<T&&>(Value)` (a raw cast equivalent to `MoveTemp`, not actually calling the engine's `MoveTemp` helper here) to move-assign into `m_Value` instead of copying.
4. `Fail(E Error = E{})` builds a default `TResult`, sets `m_Error` to the given error (or a default-constructed `E{}`, which for `EEngineError` is `None = 0` since it's the first enumerator — note this means `Fail()` with no argument produces an error result whose `GetError()` returns `EEngineError::None`, which is semantically odd but is exactly what the code does), and leaves `m_bOk` as `false` (already set by the private default ctor). Note `m_Value` is never touched here — it remains whatever the default `T()` constructor produced.
5. `IsOk()`/`IsErr()` are trivial boolean accessors reading `m_bOk`.
6. `GetValue()` (both const and non-const overloads) `check(m_bOk)` — a **hard assert** — before returning a reference to `m_Value`. Calling `GetValue()` on a `Fail`-constructed result triggers `assert(m_bOk)` (per `EnginePCH.h`'s `#define check(expr) assert(expr)`), aborting in debug builds; in a build where `assert` is compiled out (e.g., `NDEBUG`), this would silently return the default-constructed `m_Value` with no protection — the header doesn't special-case that.
7. `GetError()` inversely `check(!m_bOk)`s before returning `m_Error` by value — calling it on an `Ok` result also hard-asserts.
8. There is no `.cpp` file for `TResult.h` — it is a fully header-only template class (as it must be, being a template), consistent with the project's general rule that templates live entirely in headers even though the project's stated convention is normally "header + .cpp pair" for classes.

**Public API surface:**
| Member | Purpose |
|---|---|
| `EEngineError` (`None`, `FileNotFound`, `OutOfMemory`, `InvalidArgument`, `Unknown`) | Default error enum used as `TResult`'s second template parameter |
| `TResult<T, E=EEngineError>::Ok(const T&)` / `Ok(T&&)` | Construct a success result (copy or move) |
| `TResult<T, E>::Fail(E Error = E{})` | Construct a failure result carrying an error code |
| `IsOk()` / `IsErr()` | Query success/failure state |
| `GetValue()` (const/non-const) | Access the success value; `check(m_bOk)`-asserts if called on a failure |
| `GetError()` | Access the error code; `check(!m_bOk)`-asserts if called on a success |

---

**Note on source encoding:** `TypeTraits.h` and `FStackAllocator.cpp` contain comments that render as mojibake (e.g., `// ��� Ÿ�� ---`) in the raw source — these appear to be Korean-language comments saved/read with a mismatched text encoding. This does not affect compiled behavior; it's purely a documentation/readability artifact in those two files, called out here rather than guessed at or "corrected."
---

## TArray / TArrayView

#### Purpose
`TArray<T, AllocatorType>` is a STL-free, Unreal-style dynamic array supporting both always-heap (`TDefaultAllocator`) and small-buffer/inline (`TInlineAllocator<N>`) storage policies; `TArrayView<T>` is a non-owning, read-only slice over any contiguous `T` buffer (raw pointer+size or a `TArray`).

Both live in `/home/user/MapleStory_UnrealSource/Engine/Include/Core/Containers/TArray.h` and `TArrayView.h`.

#### Real code excerpts

Allocator policies and the EBO trick for `N == 0`:

```cpp
struct TDefaultAllocator
{
	static const int32 InlineCapacity = 0;
};

template<int32 N>
struct TInlineAllocator
{
	static const int32 InlineCapacity = N;
};

template<typename T, int32 N>
struct TArrayInlineStorage
{
	alignas(T) uint8 m_InlineBytes[sizeof(T) * N];
	T* GetInline() { return reinterpret_cast<T*>(m_InlineBytes); }
	const T* GetInline() const { return reinterpret_cast<const T*>(m_InlineBytes); }
};

template<typename T>
struct TArrayInlineStorage<T, 0>
{
	T* GetInline() { return nullptr; }
	const T* GetInline() const { return nullptr; }
};

template<typename T, typename AllocatorType = TDefaultAllocator>
class TArray : private TArrayInlineStorage<T, AllocatorType::InlineCapacity>
```
`TArray` privately inherits `TArrayInlineStorage<T, InlineCapacity>`; when `InlineCapacity == 0` this is the specialized empty struct with no data members, so the Empty Base Optimization (EBO) means `TDefaultAllocator`-based arrays pay zero extra size for the (unused) inline buffer.

Growth policy (`Add`) with the trivially-copyable branch:

```cpp
int32 Add(const T& Element)
{
	EnsureCapacity(m_Size + 1);

	if constexpr (TIsTriviallyCopyable<T>::Value)
	{
		FMemory::Memcpy(m_pData + m_Size, &Element, sizeof(T));
	}
	else
	{
		new (m_pData + m_Size) T(Element);
	}

	return m_Size++;
}
```

```cpp
void EnsureCapacity(int32 Required)
{
	if (m_Capacity >= Required) return;

	int32 NewCap = (m_Capacity == 0) ? 4 : m_Capacity * 2;
	while (NewCap < Required) NewCap *= 2;

	GrowTo(NewCap);
}
```

`RemoveAt` (order-preserving, O(n)) vs `RemoveAtSwap` (O(1)):

```cpp
void RemoveAt(int32 Index)
{
	check(IsValidIndex(Index));
	if constexpr (!TIsTriviallyCopyable<T>::Value) { m_pData[Index].~T(); }

	int32 NumToMove = m_Size - Index - 1;
	if (NumToMove > 0)
	{
		if constexpr (TIsTriviallyCopyable<T>::Value)
			FMemory::Memmove(m_pData + Index, m_pData + Index + 1, static_cast<size_t>(NumToMove) * sizeof(T));
		else
			for (int32 i = Index; i < m_Size - 1; i++)
			{
				new (m_pData + i) T(MoveTemp(m_pData[i + 1]));
				m_pData[i + 1].~T();
			}
	}
	m_Size--;
}

void RemoveAtSwap(int32 Index)
{
	check(IsValidIndex(Index));
	if constexpr (!TIsTriviallyCopyable<T>::Value) { m_pData[Index].~T(); }

	int32 LastIndex = m_Size - 1;
	if (Index != LastIndex)
	{
		if constexpr (TIsTriviallyCopyable<T>::Value)
			FMemory::Memcpy(m_pData + Index, m_pData + LastIndex, sizeof(T));
		else
		{
			new (m_pData + Index) T(MoveTemp(m_pData[LastIndex]));
			m_pData[LastIndex].~T();
		}
	}
	m_Size--;
}
```

`Sort` (introsort: insertion sort ≤16 elements, otherwise median-of-3 Lomuto quicksort) vs `StableSort` (top-down merge sort):

```cpp
template<typename Pred>
void SortImpl(int32 Low, int32 High, Pred InPred)
{
	if (Low >= High) return;

	if (High - Low + 1 <= 16)
	{
		// insertion sort ...
		return;
	}

	int32 Mid = Low + (High - Low) / 2;
	if (InPred(m_pData[Mid], m_pData[Low]))  SwapElements(Low, Mid);
	if (InPred(m_pData[High], m_pData[Low])) SwapElements(Low, High);
	if (InPred(m_pData[Mid], m_pData[High])) SwapElements(Mid, High);

	int32 StoreIdx = Low - 1;
	for (int32 j = Low; j < High; j++)
	{
		if (!InPred(m_pData[High], m_pData[j])) { StoreIdx++; SwapElements(StoreIdx, j); }
	}
	SwapElements(StoreIdx + 1, High);
	int32 PivotFinal = StoreIdx + 1;

	SortImpl(Low, PivotFinal - 1, InPred);
	SortImpl(PivotFinal + 1, High, InPred);
}

template<typename Pred>
void StableSortImpl(int32 Low, int32 High, Pred InPred)
{
	if (Low >= High) return;
	int32 Mid = Low + (High - Low) / 2;
	StableSortImpl(Low, Mid, InPred);
	StableSortImpl(Mid + 1, High, InPred);
	MergeImpl(Low, Mid, High, InPred);
}
```
`MergeImpl` allocates two temporary `FMemory::Malloc` buffers (`Left`, `Right`), copies/moves the two halves into them, merges back element-by-element (branching on `TIsTriviallyCopyable<T>` for `Memcpy` vs `MoveTemp`), then destroys and frees the temporaries.

`TArrayView` construction from `TArray`:

```cpp
template<typename AllocatorType>
TArrayView(const TArray<T, AllocatorType>& InArray) noexcept
	: m_pData(InArray.GetData()), m_Size(InArray.Num())
{
}
```

#### Step-by-step runtime behavior

**Allocator policy.** `TArray` derives from `TArrayInlineStorage<T, AllocatorType::InlineCapacity>`. With `TDefaultAllocator`, `InlineCapacity == 0`, the specialized empty storage type has no members, and thanks to EBO the base subobject contributes no extra bytes — the array behaves exactly like a classic heap-only dynamic array (`m_pData` starts `nullptr`, `m_Capacity` starts `0`). With `TInlineAllocator<N>`, the base class embeds an `alignas(T) uint8 m_InlineBytes[sizeof(T)*N]` buffer directly inside the `TArray` object; `InitInlineBaseline()` points `m_pData` at that buffer and sets `m_Capacity = InlineCapacity` so a freshly constructed array is immediately "full-capacity" without touching the heap.

**Small-buffer transitions.** `IsInline()` compares `m_pData == StorageType::GetInline()` to tell whether the array is currently living in the inline buffer or on the heap. `GrowTo()` always allocates a brand-new heap block via `FMemory::Malloc`, relocates existing elements into it with `RelocateElements`, then calls `FreeRaw()` (which only actually frees when `!IsInline()`, so the inline buffer itself is never `FMemory::Free`d) — this is how an inline array "spills" to the heap once it exceeds `N` elements. `Shrink()` can move data back into the inline buffer (`MoveToInline()`) if `m_Size <= InlineCapacity` after shrinking. `MoveFrom` for move-construction/assignment either transfers the heap pointer (steal) when `Other` is on the heap, or element-wise relocates when `Other` is inline (since a stack buffer's address can't be "stolen").

**Growth (2x).** `EnsureCapacity(Required)` does nothing if `m_Capacity >= Required`; otherwise it starts a new capacity at 4 (from empty) or doubles the current capacity (`m_Capacity * 2`), doubling again in a loop until it's `>= Required`, then calls `GrowTo`. `GrowTo` is a straightforward "allocate new, relocate old, free old" sequence.

**Trivial vs non-trivial branch (`if constexpr TIsTriviallyCopyable<T>`).** Nearly every mutating operation (`Add`, `RemoveAt`, `RemoveAtSwap`, `RemoveAll`, `RelocateElements`, `CopyElementsFrom`, `DestroyElements`, `MergeImpl`) branches at compile time: trivially-copyable types use `FMemory::Memcpy`/`Memmove` (raw bytes, no constructor/destructor calls, no branch cost at runtime since it's resolved at compile time), while non-trivial types go through placement-new (`new (ptr) T(...)`) plus explicit `~T()` calls and `MoveTemp`. This means POD types (e.g. `int32`, `FVector2D`) get memcpy-speed array operations while class types with user-defined constructors/destructors get correct construction/destruction semantics.

**RemoveAt vs RemoveAtSwap.** `RemoveAt` destroys the element at `Index`, then shifts every following element down by one slot (`Memmove` for trivial types, or a per-element move+destroy loop otherwise), preserving relative order — cost is O(n - Index). `RemoveAtSwap` destroys the element at `Index`, then (if it isn't already the last element) overwrites it with the last live element (`Memcpy`/move) and shrinks `m_Size` by one — O(1) but destroys ordering.

**Sort / StableSort.** `Sort()` is unstable introsort-lite: for ranges of 16 or fewer elements it falls back to a plain insertion sort; larger ranges pick a median-of-three pivot (`Low`, `Mid`, `High`), move it to `High`, run a Lomuto partition, then recurse on both sides — no true "insort" depth-limited introsort with heapsort fallback is present, just insertion-sort + quicksort. `StableSort()` is a textbook top-down merge sort: recursively sort `[Low, Mid]` and `[Mid+1, High]`, then `MergeImpl` allocates two scratch buffers via `FMemory::Malloc`, copies both halves in, merges back comparing `!InPred(Right[j], Left[i])` to prefer the left run on ties (stability), and frees the scratch buffers.

**TArrayView.** Purely non-owning: it stores `const T* m_pData` and `int32 m_Size`, with no allocation/deallocation logic anywhere. It can be implicitly built from any `TArray<T, AllocatorType>` (templated on the allocator so it works with both default and inline arrays) or from a raw `(data, size)` pair, and supports `Slice()` to produce sub-views with bounds checks (`check`).

#### Public API surface

| Type | Member | Behavior |
|---|---|---|
| TArray | `Add(const T&)/Add(T&&)` | append, grow if needed |
| TArray | `Emplace(Args&&...)` | in-place construct at end |
| TArray | `RemoveAt(Index)` | O(n), order-preserving |
| TArray | `RemoveAtSwap(Index)` | O(1), swap-with-last |
| TArray | `Remove(const T&)` / `RemoveAll(const T&)` | remove first / all matches |
| TArray | `Find`/`Contains` | linear search |
| TArray | `Sort()`/`Sort(Pred)` | unstable introsort-ish |
| TArray | `StableSort()`/`StableSort(Pred)` | merge sort |
| TArray | `Reserve`/`Shrink`/`Reset`/`Empty` | capacity management |
| TArray | `operator[]`, `Last`, `GetData` | element access |
| TArray | `Num`, `Max`, `IsEmpty`, `IsValidIndex` | state |
| TArray | `begin`/`end` | range-for |
| TArrayView | `Slice`, `operator[]`, `GetData` | slicing/access |
| TArrayView | `Find`, `Contains`, `Num`, `IsEmpty`, `IsValidIndex` | query |
| TArrayView | `begin`/`end` | range-for |

---

## TSparseArray

#### Purpose
`TSparseArray<T>` is the free-list-backed, index-stable, holey array that underlies both `TSet` and `TMap` — elements keep a fixed index for their lifetime, and freed slots are recycled via an intrusive singly-linked free list instead of being compacted.

File: `/home/user/MapleStory_UnrealSource/Engine/Include/Core/Containers/TSparseArray.h`

#### Real code excerpts

The slot layout:

```cpp
struct FSlot
{
    alignas(T) uint8 m_Storage[sizeof(T)];
    int32 m_NextFree;     // next free slot index; valid only when not allocated
    bool  m_bAllocated;
};

FSlot* m_pSlots;
int32 m_Capacity;         // slots in the buffer
int32 m_HighWater;        // slots ever used; indices >= HighWater are untouched
int32 m_NumAllocated;     // live element count
int32 m_FirstFree;        // free list head inside [0, HighWater), INDEX_NONE if empty
```

`Emplace` (free-list reuse or append) and `RemoveAt` (push onto free list):

```cpp
template<typename... Args>
int32 Emplace(Args&&... InArgs)
{
    int32 Index;

    if (m_FirstFree != INDEX_NONE)
    {
        Index = m_FirstFree;
        m_FirstFree = m_pSlots[Index].m_NextFree;
    }
    else
    {
        if (m_HighWater == m_Capacity)
        {
            Grow(m_Capacity == 0 ? 4 : m_Capacity * 2);
        }
        Index = m_HighWater++;
    }

    new (m_pSlots[Index].m_Storage) T(Forward<Args>(InArgs)...);
    m_pSlots[Index].m_bAllocated = true;
    m_NumAllocated++;

    return Index;
}

void RemoveAt(int32 Index)
{
    check(IsAllocated(Index));

    GetPtr(Index)->~T();

    m_pSlots[Index].m_bAllocated = false;
    m_pSlots[Index].m_NextFree = m_FirstFree;
    m_FirstFree = Index;
    m_NumAllocated--;
}
```

`Grow` (relocation into a larger buffer):

```cpp
void Grow(int32 NewCapacity)
{
    FSlot* pNew = (FSlot*)FMemory::Malloc(sizeof(FSlot) * NewCapacity, alignof(FSlot));
    check(pNew != nullptr);

    for (int32 i = 0; i < m_HighWater; i++)
    {
        pNew[i].m_bAllocated = m_pSlots[i].m_bAllocated;
        pNew[i].m_NextFree = m_pSlots[i].m_NextFree;

        if (m_pSlots[i].m_bAllocated)
        {
            if constexpr (TIsTriviallyCopyable<T>::Value)
            {
                FMemory::Memcpy(pNew[i].m_Storage, m_pSlots[i].m_Storage, sizeof(T));
            }
            else
            {
                new (pNew[i].m_Storage) T(MoveTemp(*GetPtr(i)));
                GetPtr(i)->~T();
            }
        }
    }

    if (m_pSlots) { FMemory::Free(m_pSlots); }

    m_pSlots = pNew;
    m_Capacity = NewCapacity;
}
```

`CopyFrom` (structural copy, preserving indices):

```cpp
void CopyFrom(const TSparseArray& Other)
{
    if (Other.m_HighWater > 0)
    {
        Grow(Other.m_HighWater);

        for (int32 i = 0; i < Other.m_HighWater; i++)
        {
            m_pSlots[i].m_bAllocated = Other.m_pSlots[i].m_bAllocated;
            m_pSlots[i].m_NextFree = Other.m_pSlots[i].m_NextFree;

            if (Other.m_pSlots[i].m_bAllocated)
            {
                new (m_pSlots[i].m_Storage) T(*Other.GetPtr(i));
            }
        }
    }

    m_HighWater = Other.m_HighWater;
    m_NumAllocated = Other.m_NumAllocated;
    m_FirstFree = Other.m_FirstFree;
}
```

Iterator that skips holes:

```cpp
void SkipToAllocated()
{
    while (m_Index < m_pArray->m_HighWater && !m_pArray->m_pSlots[m_Index].m_bAllocated)
    {
        m_Index++;
    }
}
```

#### Step-by-step runtime behavior

**FSlot layout.** Each `FSlot` reserves raw, aligned storage for one `T` (`alignas(T) uint8 m_Storage[sizeof(T)]`), plus `int32 m_NextFree` and `bool m_bAllocated`. The `T` is never constructed until the slot is allocated (`Emplace`), and it's explicitly destroyed (`~T()`) on `RemoveAt`/`Reset`/`Empty` — the raw byte buffer means an unallocated slot costs no live `T` lifetime.

**Free-list-based Emplace.** `Emplace` first checks `m_FirstFree`. If it's not `INDEX_NONE`, there's a previously-removed slot available: it pops the head of the free list (`Index = m_FirstFree; m_FirstFree = m_pSlots[Index].m_NextFree`) and reuses that slot's storage — no allocation or relocation needed, and existing indices elsewhere are undisturbed. If the free list is empty, it falls back to appending: growing the backing buffer (doubling, or starting at 4) if `m_HighWater == m_Capacity`, then taking `Index = m_HighWater++`. Either way it placement-news the new `T` into `m_pSlots[Index].m_Storage`, marks `m_bAllocated = true`, and increments `m_NumAllocated`.

**RemoveAt.** Destroys the live object in place (`GetPtr(Index)->~T()`), flips `m_bAllocated = false`, and pushes the now-free slot onto the head of the free list (`m_pSlots[Index].m_NextFree = m_FirstFree; m_FirstFree = Index`). This is O(1) and leaves a "hole" at `Index` — the index itself is never reused by anything else until a future `Emplace` pulls it back off the free list. `m_HighWater` (the high-water mark of ever-used slots) is untouched by removal.

**Grow relocation.** `Grow(NewCapacity)` allocates a brand-new `FSlot` array via `FMemory::Malloc`, then walks `[0, m_HighWater)` copying `m_bAllocated`/`m_NextFree` metadata for every slot (allocated or not — this preserves the free-list linkage across the whole `[0, m_HighWater)` range, not just live elements) and relocating the live `T`s (`Memcpy` for trivially-copyable types, or move-construct + destroy otherwise). The old buffer is then freed and the pointer/`m_Capacity` are swapped in. Because free/allocated bookkeeping is copied for the full `HighWater` range, the free list remains valid and every live element retains its original index after growth.

**CopyFrom (structural copy).** Used by the copy constructor and copy-assignment: it `Grow`s to `Other.m_HighWater` capacity, then for every slot index in `[0, Other.m_HighWater)` copies the `m_bAllocated`/`m_NextFree` metadata verbatim and, if allocated, copy-constructs the `T` from `Other`. Finally it copies `m_HighWater`, `m_NumAllocated`, and `m_FirstFree` directly. This is why the header comment in `TMap`/`TSet` notes "sparse indices are preserved by the structural copy" — every element in the copy lands at the exact same index it had in `Other`, which lets `TSet`/`TMap` simply `Memcpy` their bucket arrays after copying `m_Elements` rather than re-hashing.

**Iteration.** `FIterator`/`FConstIterator` wrap a `(TSparseArray*, int32 m_Index)` pair. On construction and after every `operator++`, `SkipToAllocated()` advances `m_Index` past any slots where `m_bAllocated` is false, so dereferencing (`operator*`, which calls `(*m_pArray)[m_Index]`) always lands on a live element. `begin()` starts at index 0 (skipping leading holes), `end()` is `FIterator(this, m_HighWater)`. `GetIndex()` exposes the current sparse index — this is exactly what `TSet::Rehash`/`TMap::Rehash` use to relink the hash-bucket chains without touching element storage.

#### Public API surface

| Member | Behavior |
|---|---|
| `Emplace(Args&&...)` / `Add(const T&)` / `Add(T&&)` | insert, reuse free slot or append |
| `RemoveAt(Index)` | O(1) destroy + return to free list |
| `IsAllocated(Index)` | bounds + `m_bAllocated` check |
| `operator[](Index)` | element access (checked) |
| `Num()` | `m_NumAllocated` |
| `GetMaxIndex()` | `m_HighWater` (iterate `[0, GetMaxIndex())` with `IsAllocated`) |
| `IsEmpty()` | `m_NumAllocated == 0` |
| `Reset()` | destroy all elements, keep buffer |
| `Empty()` | destroy all + free buffer |
| `begin()/end()` (`FIterator`/`FConstIterator`) | hole-skipping iteration, `GetIndex()` |

---

## TSet / TMap / TMultiMap

#### Purpose
`TSet<KeyType>` and `TMap<KeyType, ValueType>` are separate-chaining hash containers built on top of `TSparseArray` for element storage plus a power-of-two bucket array of intrusive chain heads; `TMultiMap<KeyType, ValueType>` layers a one-to-many key/value API on top of `TMap<KeyType, TArray<ValueType>>`.

Files: `/home/user/MapleStory_UnrealSource/Engine/Include/Core/Containers/TSet.h`, `TMap.h`, `TMultiMap.h`.

#### Real code excerpts

`FSetElement` / `FMapElement` with the intrusive chain link:

```cpp
// TSet.h
struct FSetElement
{
    KeyType Key;
    int32 m_HashNext;   // next element index in the same bucket chain

    FSetElement(const KeyType& InKey, int32 InHashNext) : Key(InKey), m_HashNext(InHashNext) {}
    FSetElement(KeyType&& InKey, int32 InHashNext) : Key(MoveTemp(InKey)), m_HashNext(InHashNext) {}
};

TSparseArray<FSetElement> m_Elements;
int32* m_pBuckets;      // element index heads, INDEX_NONE-terminated chains
int32 m_NumBuckets;     // always a power of two
```

```cpp
// TMap.h
struct FMapElement
{
    KeyType Key;
    ValueType Value;
    int32 m_HashNext;   // next element index in the same bucket chain

    FMapElement(const KeyType& InKey, const ValueType& InValue, int32 InHashNext)
        : Key(InKey), Value(InValue), m_HashNext(InHashNext) {}
    FMapElement(KeyType&& InKey, ValueType&& InValue, int32 InHashNext)
        : Key(MoveTemp(InKey)), Value(MoveTemp(InValue)), m_HashNext(InHashNext) {}
    FMapElement(const KeyType& InKey, int32 InHashNext)
        : Key(InKey), Value(), m_HashNext(InHashNext) {}
};

TSparseArray<FMapElement> m_Elements;
int32* m_pBuckets;
int32 m_NumBuckets;
```

Bucket indexing via mask (power-of-two bucket count):

```cpp
static const int32 INITIAL_BUCKETS = 16;

int32 BucketIndex(uint32 Hash) const
{
    return (int32)(Hash & (uint32)(m_NumBuckets - 1));
}
```

`ConditionalRehash` and `Rehash` (relink without moving elements):

```cpp
void Rehash(int32 NewNumBuckets)
{
    if (m_pBuckets) { FMemory::Free(m_pBuckets); }

    AllocateBuckets(NewNumBuckets);

    for (auto It = m_Elements.begin(); It != m_Elements.end(); ++It)
    {
        int32& Bucket = m_pBuckets[BucketIndex(GetTypeHash(It->Key))];
        It->m_HashNext = Bucket;
        Bucket = It.GetIndex();
    }
}

void ConditionalRehash(int32 NumElements)
{
    if (!m_pBuckets)
    {
        AllocateBuckets(INITIAL_BUCKETS);
        return;
    }

    if (NumElements > m_NumBuckets)
    {
        int32 NewNumBuckets = m_NumBuckets;
        while (NumElements > NewNumBuckets) { NewNumBuckets *= 2; }
        Rehash(NewNumBuckets);
    }
}
```

`TSet::Add` (insert, chaining onto the bucket head):

```cpp
bool Add(const KeyType& Key)
{
    ConditionalRehash(m_Elements.Num() + 1);

    int32& Bucket = m_pBuckets[BucketIndex(GetTypeHash(Key))];

    for (int32 i = Bucket; i != INDEX_NONE; i = m_Elements[i].m_HashNext)
    {
        if (m_Elements[i].Key == Key) { return false; } // already exists
    }

    Bucket = m_Elements.Emplace(Key, Bucket);

    return true;
}
```

`TMap::Add` / `FindOrAdd` (update-in-place vs insert):

```cpp
void Add(const KeyType& Key, const ValueType& Value)
{
    ConditionalRehash(m_Elements.Num() + 1);

    int32& Bucket = m_pBuckets[BucketIndex(GetTypeHash(Key))];

    for (int32 i = Bucket; i != INDEX_NONE; i = m_Elements[i].m_HashNext)
    {
        if (m_Elements[i].Key == Key) { m_Elements[i].Value = Value; return; }
    }

    Bucket = m_Elements.Emplace(Key, Value, Bucket);
}

ValueType& FindOrAdd(const KeyType& Key)
{
    ConditionalRehash(m_Elements.Num() + 1);

    int32& Bucket = m_pBuckets[BucketIndex(GetTypeHash(Key))];

    for (int32 i = Bucket; i != INDEX_NONE; i = m_Elements[i].m_HashNext)
    {
        if (m_Elements[i].Key == Key) { return m_Elements[i].Value; }
    }

    Bucket = m_Elements.Emplace(Key, Bucket);

    return m_Elements[Bucket].Value;
}
```

Remove (unlink from chain, then free the `TSparseArray` slot):

```cpp
bool Remove(const KeyType& Key)
{
    if (!m_pBuckets || m_Elements.IsEmpty()) { return false; }

    int32* pLink = &m_pBuckets[BucketIndex(GetTypeHash(Key))];

    while (*pLink != INDEX_NONE)
    {
        const int32 Index = *pLink;

        if (m_Elements[Index].Key == Key)
        {
            *pLink = m_Elements[Index].m_HashNext;
            m_Elements.RemoveAt(Index);
            return true;
        }

        pLink = &m_Elements[Index].m_HashNext;
    }

    return false;
}
```

Iterators forwarding to `TSparseArray`'s iterator:

```cpp
// TSet.h
struct FIterator
{
    typename TSparseArray<FSetElement>::FIterator m_It;

    FIterator(typename TSparseArray<FSetElement>::FIterator It) : m_It(It) {}
    FIterator& operator++() { ++m_It; return *this; }
    bool operator!=(const FIterator& Other) const { return m_It != Other.m_It; }
    KeyType& operator*() const { return (*m_It).Key; }
};
```

```cpp
// TMap.h
using FIterator = typename TSparseArray<FMapElement>::FIterator;
using FConstIterator = typename TSparseArray<FMapElement>::FConstIterator;

FIterator begin() { return m_Elements.begin(); }
FIterator end() { return m_Elements.end(); }
```

`TMultiMap` wrapping `TMap<K, TArray<V>>`:

```cpp
template<typename KeyType, typename ValueType>
class TMultiMap
{
private:
    TMap<KeyType, TArray<ValueType>> m_Map;

public:
    void Add(const KeyType& Key, const ValueType& Value)
    {
        m_Map.FindOrAdd(Key).Add(Value);
    }

    bool AddUnique(const KeyType& Key, const ValueType& Value)
    {
        TArray<ValueType>& Values = m_Map.FindOrAdd(Key);
        if (Values.Contains(Value)) { return false; }
        Values.Add(Value);
        return true;
    }

    const TArray<ValueType>* MultiFind(const KeyType& Key) const
    {
        return m_Map.Find(Key);
    }
    ...
};
```

#### Step-by-step runtime behavior

**Element layout and chaining.** `TSet` stores `FSetElement { KeyType Key; int32 m_HashNext; }`; `TMap` stores `FMapElement { KeyType Key; ValueType Value; int32 m_HashNext; }`. Both are held inside a `TSparseArray<FSetElement>`/`TSparseArray<FMapElement>` (`m_Elements`), so each element gets a stable sparse index for as long as it lives, and `m_HashNext` is an intrusive singly-linked-list pointer (by sparse index, `INDEX_NONE`-terminated) chaining together every element that hashes to the same bucket — this is classic separate chaining, not open addressing.

**Bucket array.** `m_pBuckets` is a flat `int32*` array of `m_NumBuckets` chain-head indices (`INDEX_NONE` = empty), and `m_NumBuckets` is always kept a power of two so `BucketIndex(Hash)` can compute `Hash & (m_NumBuckets - 1)` (a mask) instead of a modulo. `AllocateBuckets` mallocs the array and initializes every slot to `INDEX_NONE`. Both `TSet` and `TMap` start with `m_pBuckets == nullptr, m_NumBuckets == 0` and lazily allocate `INITIAL_BUCKETS == 16` buckets on first insert via `ConditionalRehash`.

**Add / Find.** Both containers compute `BucketIndex(GetTypeHash(Key))` to get a reference to the bucket's head slot, then walk the `m_HashNext` chain comparing keys with `==`. For `TSet::Add`, if a matching key is found the call returns `false` (no duplicate insert); otherwise a new `FSetElement(Key, Bucket)` is emplaced into `m_Elements` — note the new element's `m_HashNext` is initialized to the *previous* bucket head (`Bucket`) before `Bucket` is reassigned to the new element's index, so insertion is O(1) push-to-front of the chain. `TMap::Add` does the analogous walk but instead of failing on a match, it overwrites `m_Elements[i].Value` in place (update semantics). `FindOrAdd`/`operator[]` do the same walk, returning a reference to the existing value or inserting a default-constructed one (`FMapElement(Key, Bucket)` — the value-less constructor default-constructs `Value`) if absent. `Find`/`FindIndex`/`Contains` are read-only walks of the same shape and return `nullptr`/`INDEX_NONE` on a miss.

**Remove.** Rather than iterating with a simple index, `Remove` walks the chain holding a pointer-to-pointer (`int32* pLink`) starting at the bucket head; when it finds the matching key it splices the element out of the chain by writing `*pLink = m_Elements[Index].m_HashNext` (works uniformly whether the match is the bucket head or a middle/tail link, since `pLink` was previously either `&m_pBuckets[...]` or `&m_Elements[Index].m_HashNext` of the prior link), then calls `m_Elements.RemoveAt(Index)` to return the sparse slot to `TSparseArray`'s free list.

**ConditionalRehash / Rehash.** Every `Add`/`FindOrAdd` first calls `ConditionalRehash(m_Elements.Num() + 1)`. If buckets haven't been allocated yet, it allocates the initial 16 and returns (no rehash needed for an empty container). Otherwise, if the prospective element count would exceed `m_NumBuckets` (i.e., load factor > 1.0), it doubles `NewNumBuckets` until it's `>= NumElements`, then calls `Rehash(NewNumBuckets)`. Critically, `Rehash` does **not** move or reconstruct any elements in `m_Elements` — it only frees and reallocates the (much smaller) `int32` bucket array, then re-links every existing element by iterating `m_Elements.begin()..end()` (a `TSparseArray` iterator that already skips holes) and, for each, recomputing `BucketIndex(GetTypeHash(It->Key))` and pushing `It.GetIndex()` onto the new bucket's chain head. Because `TSparseArray` indices are stable, no `T` object is ever copied/moved/destroyed during a rehash — only the tiny bucket-head/`m_HashNext` integers change.

**Iterators forward to TSparseArray's iterator.** `TSet::FIterator`/`FConstIterator` are thin wrappers holding a `TSparseArray<FSetElement>::FIterator m_It`; `operator++` forwards to `++m_It`, `operator!=` forwards to `m_It != Other.m_It`, and `operator*` returns `(*m_It).Key` (i.e., it un-wraps the `FSetElement` down to just the `KeyType&`). `TMap` goes one step further and doesn't even wrap — it directly aliases `FIterator = typename TSparseArray<FMapElement>::FIterator` and its `begin()/end()` simply return `m_Elements.begin()/end()`, so dereferencing a `TMap` iterator yields an `FMapElement&` with public `.Key`/`.Value` members directly (as seen used in `TMultiMap::Num()`: `for (const auto& Bucket : m_Map) { Total += Bucket.Value.Num(); }`). In both cases, hole-skipping (only visiting allocated slots) is entirely inherited from `TSparseArray::FIterator::SkipToAllocated()`.

**Copy semantics.** Both `TSet`'s and `TMap`'s copy constructor/assignment copy `m_Elements` via `TSparseArray`'s structural `CopyFrom` (which preserves sparse indices), and then simply `Memcpy` the bucket array (`FMemory::Memcpy(m_pBuckets, Other.m_pBuckets, sizeof(int32) * m_NumBuckets)`) rather than re-hashing — valid precisely because the structural copy guarantees every element keeps the same index in the copy as in the original, so the old bucket-chain indices remain correct.

**TMultiMap.** It holds a single private member `TMap<KeyType, TArray<ValueType>> m_Map` and has no hashing/bucket logic of its own — `Add` calls `m_Map.FindOrAdd(Key).Add(Value)` (get-or-create the value array, then append); `AddUnique` additionally checks `Values.Contains(Value)` before appending; `MultiFind`/`Contains` delegate to `m_Map.Find`; `RemoveSingle` finds the array, removes one matching value via `TArray::Remove`, and if the array becomes empty also calls `m_Map.Remove(Key)` to drop the key entirely; `RemoveAll` just calls `m_Map.Remove(Key)`. `NumKeys()` is `m_Map.Num()`; `Num()` (total values) iterates every bucket and sums `Bucket.Value.Num()`.

#### Public API surface

| Container | Member | Behavior |
|---|---|---|
| TSet | `Add(const K&)/Add(K&&)` | insert if absent, returns bool |
| TSet | `Contains(const K&)` | membership test |
| TSet | `Remove(const K&)` | unlink + free slot |
| TSet | `Reset()`/`Empty()` | clear elements (keep/free buckets) |
| TSet | `Num()`/`IsEmpty()` | delegate to `m_Elements` |
| TSet | `begin()/end()` | `FIterator`/`FConstIterator` yielding `KeyType&` |
| TMap | `Add(K,V)` | insert or overwrite value |
| TMap | `FindOrAdd(const K&)` / `operator[]` | get-or-default-insert reference |
| TMap | `Find(const K&)` | returns `ValueType*` or `nullptr` |
| TMap | `FindRef(const K&)` | returns reference, `check()`s existence |
| TMap | `Contains(const K&)` | membership test |
| TMap | `Remove(const K&)` | unlink + free slot |
| TMap | `Reset()`/`Empty()` | clear |
| TMap | `Num()`/`IsEmpty()` | delegate to `m_Elements` |
| TMap | `begin()/end()` | `TSparseArray<FMapElement>::FIterator` directly, yields `.Key`/`.Value` |
| TMultiMap | `Add(K,V)` | append value under key |
| TMultiMap | `AddUnique(K,V)` | append only if pair absent |
| TMultiMap | `MultiFind(const K&)` | returns `const TArray<V>*` |
| TMultiMap | `Contains(K)` / `Contains(K,V)` | key-only or key+value test |
| TMultiMap | `RemoveSingle(K,V)` | remove one value, drop key if array empties |
| TMultiMap | `RemoveAll(K)` | drop key entirely |
| TMultiMap | `Reset()`/`Empty()` | delegate to `m_Map` |
| TMultiMap | `NumKeys()`/`Num()`/`IsEmpty()` | key count / total value count / emptiness |

---

## HashFunctions

#### Purpose
`HashFunctions.h` (`/home/user/MapleStory_UnrealSource/Engine/Include/Core/Containers/HashFunctions.h`) defines `GetTypeHash`, the free-function hashing hook that `TSet`/`TMap` call via `BucketIndex(GetTypeHash(Key))`, along with a generic byte-scanning fallback and explicit specializations for common scalar types.

#### Real code excerpts

Generic fallback (byte-level reinterpret + Murmur-inspired finalizer per byte):

```cpp
template<typename T>
inline uint32 GetTypeHash(const T& Value)
{
    uint32 Hash = 0;
    const uint8* Bytes = reinterpret_cast<const uint8*>(&Value);

    for (int32 i = 0; i < (int32)sizeof(T); i++)
    {
        Hash ^= (uint32)Bytes[i] << ((i & 3) * 8);
        Hash ^= Hash >> 16;
        Hash *= 0x45d9f3bU;
        Hash ^= Hash >> 16;
    }

    return Hash;
}
```

`int32`/`uint32` specialization:

```cpp
template<>
inline uint32 GetTypeHash(const int32& Value)
{
    uint32 H = (uint32)Value;
    H ^= H >> 16;
    H *= 0x45d9f3bU;
    H ^= H >> 16;
    return H;
}

template<>
inline uint32 GetTypeHash(const uint32& Value)
{
    uint32 H = Value;
    H ^= H >> 16;
    H *= 0x45d9f3bU;
    H ^= H >> 16;
    return H;
}
```

`int64`/`uint64` specialization (MurmurHash3 64-bit finalizer, folded down to 32 bits):

```cpp
template<>
inline uint32 GetTypeHash(const int64& Value)
{
    uint64 H = (uint64)Value;
    H ^= H >> 33;
    H *= 0xff51afd7ed558ccdULL;
    H ^= H >> 33;
    H *= 0xc4ceb9fe1a85ec53ULL;
    H ^= H >> 33;
    return (uint32)(H ^ (H >> 32));
}
```
(`uint64` has an identical specialization.)

`float` specialization (canonicalizes `-0.0f` to `0.0f` before hashing):

```cpp
template<>
inline uint32 GetTypeHash(const float& Value)
{
    // Treat -0.0f and 0.0f as equal
    float Canonical = (Value == 0.f) ? 0.f : Value;
    uint32 Bits;
    memcpy(&Bits, &Canonical, sizeof(uint32));
    Bits ^= Bits >> 16;
    Bits *= 0x45d9f3bU;
    Bits ^= Bits >> 16;
    return Bits;
}
```

`bool` specialization:

```cpp
template<>
inline uint32 GetTypeHash(const bool& Value)
{
    return Value ? 1u : 0u;
}
```

Pointer overload (not a specialization — a separate templated overload taking `T*`):

```cpp
template<typename T>
inline uint32 GetTypeHash(T* Ptr)
{
    uint64 Addr = (uint64)(uintptr_t)Ptr;
    return GetTypeHash(Addr);
}
```

#### Step-by-step runtime behavior

**Generic fallback.** For any type `T` without an explicit specialization, `GetTypeHash` reinterprets `&Value` as a `const uint8*` and folds each byte into a running 32-bit hash using a per-byte Murmur-style mix (`Hash ^= byte << ((i&3)*8); Hash ^= Hash>>16; Hash *= 0x45d9f3bU; Hash ^= Hash>>16`). This makes `GetTypeHash` work out of the box for any trivially-hashable POD struct passed by value/reference, at the cost of being a pure bitwise hash of the object representation (padding bytes included, and it will treat any two objects with identical bit patterns as equal-hashing regardless of semantic equality).

**int32 / uint32.** A single-pass integer finalizer: XOR-shift-right-16, multiply by the odd 32-bit constant `0x45d9f3bU`, XOR-shift-right-16 again — a cheap, well-mixing avalanche finalizer (this same `0x45d9f3bU` constant is reused as the byte-mixing multiplier in the generic fallback and the float hash).

**int64 / uint64.** Uses the classic MurmurHash3 64-bit finalizer (`fmix64`): three rounds of `H ^= H >> 33; H *= <64-bit odd constant>;` with constants `0xff51afd7ed558ccdULL` and `0xc4ceb9fe1a85ec53ULL`, then folds the 64-bit result down to 32 bits by XOR-ing the high and low halves (`H ^ (H >> 32)`).

**float.** First canonicalizes so that `-0.0f` and `0.0f` hash identically (`(Value == 0.f) ? 0.f : Value`, since floating point `-0.0f == 0.0f` evaluates true), reinterprets the bits via `memcpy` into a `uint32` (avoiding strict-aliasing UB), then runs the same one-pass integer finalizer as `int32`/`uint32`.

**bool.** Trivial: `1u` for `true`, `0u` for `false` — no mixing needed since there are only two possible hash values.

**Pointer overload.** Templated on `T*` (any pointer type), it casts the pointer to a `uint64` address (`(uint64)(uintptr_t)Ptr`) and forwards to the `int64`/`uint64` `GetTypeHash` specialization for the actual mixing, so pointer keys in a `TSet`/`TMap` get the full MurmurHash3 finalizer treatment rather than raw-address-modulo-buckets behavior.

#### Public API surface

| Function | Specialization for | Algorithm |
|---|---|---|
| `GetTypeHash(const T&)` | generic/any type | per-byte Murmur-style mix over raw bytes |
| `GetTypeHash(const int32&)` | `int32` | one-pass xorshift/multiply finalizer |
| `GetTypeHash(const uint32&)` | `uint32` | one-pass xorshift/multiply finalizer |
| `GetTypeHash(const int64&)` | `int64` | MurmurHash3 64-bit `fmix64`, folded to 32 bits |
| `GetTypeHash(const uint64&)` | `uint64` | MurmurHash3 64-bit `fmix64`, folded to 32 bits |
| `GetTypeHash(const float&)` | `float` | `-0.0`/`0.0` canonicalized, bit-cast + finalizer |
| `GetTypeHash(const bool&)` | `bool` | `1`/`0` constant |
| `GetTypeHash(T*)` | any pointer type | address cast to `uint64`, delegates to int64 hash |
---

## Math 라이브러리

`Engine/Include/Core/Math/`에 위치한 순수 값 타입(POD에 가까운 struct-like class) 모음으로, STL `<cmath>`를 얇게 감싼 스칼라 유틸리티(`FMath`)부터 2D 벡터/사각형/색상/변환/행렬, 그리고 드롭률·스폰에 쓰는 결정론적 시드 RNG(`FRandomStream`)까지 엔진 전체에서 좌표·충돌·렌더링·게임플레이 확률 계산에 공통으로 쓰이는 기반 수학 타입을 제공한다.

### FMath

상수(`PI`, `KINDA_SMALL_NUMBER = 1.e-4f`, `SMALL_NUMBER = 1.e-8f`)와 `Abs`/`Min`/`Max`/`Clamp`/`Lerp`는 템플릿으로 구현되어 `int32`, `float`, `FVector2D`(스칼라 연산자를 갖는 타입이면) 등 여러 타입에 재사용된다.

```cpp
// FMath.h
template<typename T>
static T Clamp(T Value, T MinVal, T MaxVal)
{
    return Min(Max(Value, MinVal), MaxVal);
}

template<typename T>
static T Lerp(T A, T B, float Alpha)
{
    return A + (T)((B - A) * Alpha);
}
```

삼각함수·제곱근·나머지·반올림 등(`Sin`, `Cos`, `Tan`, `Atan2`, `Sqrt`, `Pow`, `FMod`, `Floor`, `Ceil`, `Round`)은 `.cpp`에서 `::sinf`, `::atan2f` 등 C 표준 `<cmath>` 함수를 그대로 위임 호출하는 래퍼일 뿐이다. `IsNearlyZero`/`IsNearlyEqual`은 허용 오차 비교에 쓰이며, 다른 모든 타입의 `operator==`가 float 비교 시 이 함수를 사용한다(예: `FVector2D::operator==`, `FRect::operator==`).

### FVector2D

`m_X`, `m_Y` 두 float 멤버에 사칙연산자, 내적(`Dot`), `Size`/`SizeSquared`, `GetNormalized`/`Normalize`, `IsNormalized`, `IsNearlyZero`, 정적 `Distance`/`DistanceSquared`를 제공하는 기본 2D 벡터다. `Zero`/`One`/`Up`/`Right` 정적 상수를 갖는다. 정규화는 0-division을 막기 위해 `SMALL_NUMBER` 임계값을 확인한다.

```cpp
// FVector2D.cpp
FVector2D FVector2D::GetNormalized() const
{
    float Sz = Size();
    if (Sz < FMath::SMALL_NUMBER)
    {
        return FVector2D::Zero;
    }
    return FVector2D(m_X / Sz, m_Y / Sz);
}
```

캐릭터 위치, 속도, 방향, 발판/타일 좌표 등 2D 게임 전반의 위치·이동 벡터로 사용되는 기본 단위 타입이다.

### FColor / FLinearColor

`FColor`는 `uint8 m_R/m_G/m_B/m_A`로 구성된 8비트 정수 색상으로, 패킹된 `uint32`와 상호 변환(`ToPackedRGBA`/`ToPackedARGB`, `uint32` 생성자)이 가능하며 `White`/`Black`/`Red`/`Green`/`Blue`/`Transparent` 정적 상수를 가진다. `FLinearColor`는 `float m_R/m_G/m_B/m_A`(0~1 범위) 기반으로 덧셈/스칼라곱/성분곱(`operator*`)과 `Lerp`(색 보간, `FMath::Lerp` 재사용)를 제공한다. 두 타입은 상호 변환 함수를 갖고, `ToFColor`에서 반올림 오차 보정을 명시적으로 처리한다.

```cpp
// FLinearColor.cpp
FColor FLinearColor::ToFColor() const
{
    // +0.5f 로 반올림 — 없으면 1.0f*255 = 254.9999... → 254로 잘림
    return FColor((uint8)(FMath::Clamp(m_R, 0.f, 1.f) * 255.f + 0.5f), ...);
}
```

`FColor`는 GPU에 올릴 픽셀/버텍스 컬러(스프라이트 틴트, 피격 깜빡임)에, `FLinearColor`는 보간·감쇠 같은 연산이 필요한 색상 연산(페이드인아웃, 버프 이펙트 색 보간)에 쓰인다.

### FIntPoint / FIntRect

`FIntPoint`는 `int32 m_X/m_Y`의 정수 좌표(사칙연산자 일부, `Zero` 상수)로 타일맵·그리드 인덱스에 쓰인다. `FIntRect`는 `m_Left/m_Top/m_Right/m_Bottom`(int32) 4개로 구성되며, 반개방 구간 `[L,R) × [T,B)` 규칙으로 `Contains`/`Overlaps`/`Intersect`를 구현해 타일 경계가 겹치지 않도록 설계했다.

```cpp
// FIntRect.h
// 반개방 구간 [L,R) × [T,B) — 타일 겹침 방지
bool Contains(FIntPoint Point) const;
```

```cpp
// FIntRect.cpp
bool FIntRect::Contains(FIntPoint Point) const
{
    return Point.m_X >= m_Left && Point.m_X < m_Right && Point.m_Y >= m_Top && Point.m_Y < m_Bottom;
}
```

타일맵/NavGrid의 정수 좌표 및 타일 영역 계산 전용이다.

### FRect

`m_Left/m_Top/m_Right/m_Bottom`(float) 4개로 이루어진 AABB 사각형으로, `FIntRect`와 달리 **닫힌 구간** `[L,R] × [T,B]`을 사용해 float 경계 접촉도 겹침으로 포함한다.

```cpp
// FRect.h
// 닫힌 구간 [L,R] × [T,B] — float 엣지 접촉도 포함
bool Contains(FVector2D Point)   const;
bool Overlaps(const FRect& Other) const;
```

`Width`/`Height`/`Area`/`Center`, `Overlaps`(SAT 스타일 4방향 배제 검사), `Intersect`(겹치는 영역 사각형 계산)를 제공하며, `operator==`는 `FMath::IsNearlyEqual`로 비교한다. 이름 그대로 AABB 충돌 판정(캐릭터-몬스터, 히트박스, 화면/카메라 프러스텀 컬링) 전용 타입이다.

### FTransform2D

`m_Location`(FVector2D), `m_Rotation`(float, 라디안), `m_Scale`(FVector2D)로 구성된 SRT(Scale-Rotate-Translate) 2D 트랜스폼이다. `TransformPoint`/`TransformDirection`은 스케일→회전→이동 순서로 직접 계산하며(행렬을 거치지 않고 sin/cos을 인라인으로 사용), `Inverse`는 역스케일·역회전·역이동을 재계산해 만든다.

```cpp
// FTransform2D.cpp
FVector2D FTransform2D::TransformPoint(FVector2D Point) const
{
    // 1. Scale
    float SX = Point.m_X * m_Scale.m_X;
    float SY = Point.m_Y * m_Scale.m_Y;
    // 2. Rotate
    float C = FMath::Cos(m_Rotation);
    float S = FMath::Sin(m_Rotation);
    float RX = SX * C - SY * S;
    float RY = SX * S + SY * C;
    // 3. Translate
    return FVector2D(RX + m_Location.m_X, RY + m_Location.m_Y);
}
```

`USceneComponent`류가 보유하는 게임플레이 레벨 트랜스폼(오브젝트의 위치/회전/스케일)으로, 씬 그래프 상의 Location/Rotation/Scale 표현에 쓰인다.

### FMatrix3x3 / FMatrix4x4

`FMatrix3x3`은 `float m_M[3][3]` 동차좌표 행렬로 `Identity`/`Translation`/`Rotation`/`Scale` 정적 생성자, `operator*`(행렬곱), `TransformPoint`(w로 나누는 원근분할 포함, `[x,y,1]` 변환 — 이동 포함), `TransformVector`(`[x,y,0]`, 이동 미포함), `Transpose`/`Determinant`/`Inverse`(여인수·수반행렬 방식)를 갖는다.

```cpp
// FMatrix3x3.h
// 동차 좌표 [x, y, 1] 변환 (이동 포함)
FVector2D TransformPoint(FVector2D V)  const;
// 동차 벡터 [x, y, 0] 변환 (이동 미포함)
FVector2D TransformVector(FVector2D V) const;
```

`FMatrix4x4`은 `float m_M[4][4]`로 `Translation`/`RotationX·Y·Z`/`ScaleMatrix`와 렌더러용 `OrthoLH`(DirectX 왼손 좌표계 직교투영 행렬)를 제공하고, `Determinant`/`Inverse`는 4x4 라플라스 전개(3x3 소행렬식, `Det3` 람다)로 구현되어 있다.

```cpp
// FMatrix4x4.h
// DirectX LH 정투영 행렬 (2D 렌더러용)
static FMatrix4x4 OrthoLH(float Width, float Height, float NearZ, float FarZ);
```

두 행렬 타입의 역할은 명확히 나뉜다 — `FMatrix3x3`은 2D 게임플레이/UI 레이아웃 공간의 트랜스폼 합성(부모-자식 씬 노드 누적 변환)에, `FMatrix4x4`은 DX11 렌더러의 월드·뷰·정투영(Ortho) 변환 파이프라인에 쓰인다(둘 다 회전 공식은 동일한 sin/cos 기반이며 서로 독립적으로 구현되어 있다).

### FRandomStream

시드 기반의 선형합동생성기(LCG)형 결정론적 RNG. `m_InitialSeed`와 `m_Seed`(둘 다 `int32`) 두 상태만 가지며 `NextSeed()`가 매 호출마다 시드를 갱신한다.

```cpp
// FRandomStream.cpp
int32 FRandomStream::NextSeed()
{
    m_Seed = (int32)(((uint32)m_Seed) * 196314165u + 907633515u);
    return m_Seed;
}
```

이 시드를 바탕으로 `RandHelper(A)`(`[0,A)`), `RandRange(Min,Max)`(`[Min,Max]` 닫힌 구간), `FRand()`(`[0,1)`, 23비트 가수부 마스킹으로 float 생성), `FRandRange`, `RandBool`을 제공한다. `Reset()`은 `m_InitialSeed`로 되돌려 같은 시퀀스를 재현할 수 있게 한다. 같은 시드로 결정론적 재현이 가능하다는 특성 때문에 드롭 확률 계산, 몬스터 스폰, 리플레이/디버깅처럼 재현성이 필요한 게임플레이 난수에 사용하도록 설계되어 있다(CLAUDE.md 로드맵에도 "드롭 확률·몬스터 스폰" 용도로 명시됨).
---

## FString

**Purpose.** `FString` is the engine's dynamic, heap-owning wide string type (`Engine/Include/Core/String/FString.h`, `FString.cpp`). It is the analogue of Unreal's `FString`, backed by `wchar_t` rather than any STL container, allocated exclusively through `FMemory::Malloc`/`FMemory::Free`.

**Buffer layout.** The whole state is three members:

```cpp
// FString.h
wchar_t* m_pData;
int32 m_Length;
int32 m_Capacity;
```

`m_pData` is `nullptr` for an empty/default string (no allocation happens for `FString()`), `m_Length` is the character count excluding the null terminator, and `m_Capacity` is the number of `wchar_t` slots actually allocated (always `>= m_Length + 1` so there is always room for the terminator). `GetData()` compensates for the null-buffer case by returning `L""` instead of `nullptr`:

```cpp
const wchar_t* GetData() const
{
    return m_pData ? m_pData : L"";
}
```

Growth is handled by a private `Grow(int32 NewCapacity)` (`FString.cpp`) which mallocs a new buffer, copies `m_Length + 1` wide chars (including the terminator) from the old buffer if one exists, or writes a lone `L'\0'` if there wasn't one, then frees the old buffer. Callers (`operator+=`) compute the new capacity as `max(m_Capacity * 2, NewLength + 1)` — classic doubling growth with a floor at the exact requirement.

**Construction.**
- `FString()` — zero-inits, no allocation.
- `FString(const wchar_t* Str)` — allocates exactly `wcslen(Str)+1` slots and `Memcpy`s the source including its terminator; guarded by `check(m_pData != nullptr)`. A `nullptr` or empty input leaves the string in the default (unallocated) state.
- `FString(const FString& Other)` — delegates to `*this = Other` (copy assignment), so it's a real deep copy with a fresh allocation sized to `Other.m_Length + 1`.
- `FString(FString&& Other) noexcept` — steals `Other`'s pointer/length/capacity directly and resets `Other` to the empty state; no allocation, no copy.
- Destructor frees `m_pData` if non-null.

**Assignment.** `operator=(const FString&)` self-assign-checks, frees any existing buffer, then reallocates and copies (mirrors the copy constructor). `operator=(FString&&) noexcept` self-assign-checks, frees the current buffer, steals the source's members, and resets the source. `operator=(const wchar_t*)` is implemented by constructing a temporary `FString(Str)` and move-assigning it — i.e. it reuses the move path rather than duplicating logic.

**Operator overloads present:**
- `operator=` — copy, move, and `const wchar_t*`.
- `operator+=` — `const FString&`, `const wchar_t*`, and single `wchar_t` (each grows the buffer as needed; the `wchar_t` overload manually writes the char plus a fresh null terminator and bumps `m_Length`).
- `operator+` — `const FString&` and `const wchar_t*`, both implemented as "copy `*this`, then `+=`".
- `operator==` / `operator!=` — length check first, then `wcscmp` (only called when both lengths are equal and nonzero; two empty strings compare equal without touching `m_pData`).
- `operator<` — handles null-buffer cases explicitly (`nullptr < nullptr` is `false`, `nullptr < non-null` is `true`) before falling back to `wcscmp(...) < 0`.
- `operator[]` (mutable and const) — bounds-checked via `check(m_pData && Index >= 0 && Index < m_Length)`.

There is no `operator*` despite the CLAUDE.md roadmap mentioning one — the actual header only declares `+`, `+=`, `==`, `!=`, `<`, `[]`, and the assignment operators.

**State queries.** `Len()`, `IsEmpty()`, `GetData()` — all trivial inline accessors.

**Search methods** (`FString.cpp`):
```cpp
bool FString::Contains(const FString& Sub) const
{
    if (Sub.m_Length == 0 || m_Length < Sub.m_Length)
    {
        return Sub.m_Length == 0;
    }
    return wcsstr(m_pData, Sub.m_pData) != nullptr;
}
```
`StartsWith` uses `wcsncmp` against the prefix length; `EndsWith` uses `wcscmp` starting at `m_pData + (m_Length - Suffix.m_Length)`. All three treat an empty needle as trivially matching.

**Transform methods:**
- `ToUpper()` / `ToLower()` — copy `*this`, then walk the copy's buffer doing manual ASCII range remapping (`'a'..'z'` <-> `'A'..'Z'`); no locale/Unicode case folding.
- `Substring(int32 Start, int32 Length) const` — validates bounds (`Start` in range, `Length > 0`), clamps `ActualLen` to not run past the end, allocates a fresh buffer, `Memcpy`s the slice, and null-terminates.
- `Split(wchar_t Delim, TArray<FString>& OutParts) const` — resets `OutParts`, then does a single forward scan (`i` from `0` to `m_Length` inclusive) calling `Substring(Start, i - Start)` and pushing it into `OutParts` whenever it hits the delimiter or the end of string; returns `OutParts.Num()`.

**Parsing methods:**
```cpp
int32 FString::ToInt() const { return !m_pData ? 0 : (int32)wcstol(m_pData, nullptr, 10); }
float FString::ToFloat() const { return !m_pData ? 0.f : wcstof(m_pData, nullptr); }
```
Both are thin wrappers over the C runtime's wide-string parsers, with a null-buffer guard returning a zero value.

**Printf.** There is no separate `Format` function — only `Printf`:
```cpp
FString FString::Printf(const wchar_t* Fmt, ...)
{
    va_list Args;
    va_start(Args, Fmt);
    wchar_t Buffer[4096];
    int32 Len = (int32)vswprintf(Buffer, 4096, Fmt, Args);
    va_end(Args);
    if (Len <= 0) { return FString(); }
    return FString(Buffer);
}
```
It uses a fixed 4096-`wchar_t` stack buffer and `vswprintf`; if formatting fails or produces nothing (`vswprintf` returns negative on truncation/error), it returns a default (empty) `FString` rather than a truncated result. The result is built by copy-constructing `FString(Buffer)`, so the final string is a freshly right-sized heap allocation, not the stack buffer itself.

## FName / FNamePool

**Purpose.** `FName` is a cheap-to-copy, cheap-to-compare handle for interned strings, mirroring Unreal's `FName`: identical strings collapse to the same underlying entry, and comparisons/copies become `uint32` operations instead of string operations. All the actual storage and interning logic lives in `FNamePool` (`Engine/Include/Core/String/FNamePool.h/.cpp`); `FName` itself (`Engine/Include/Core/String/FName.h`) is just a wrapper around an index into that pool. Note: there is no `FName.cpp` in this tree — every `FName` member, including `ToString()`, is defined inline in `FName.h`.

**FNameEntry.** Fixed-size inline storage, no separate heap allocation per entry:
```cpp
// FNamePool.h
struct FNameEntry
{
    static const int32 NAME_SIZE = 64;
    wchar_t m_Name[NAME_SIZE];
};
```
Because the array is inline inside the struct, `TArray<FNameEntry> m_Entries` in `FNamePool` stores names contiguously with no per-entry pointer chasing/allocation — the tradeoff is a hard cap of 63 characters plus terminator per name (enforced at registration time, see below).

**FNamePool singleton.** Declared with a private constructor and deleted copy operations, and exposed only through a function-local static:
```cpp
FNamePool& FNamePool::Get()
{
    static FNamePool Instance;
    return Instance;
}
```
This is the classic Meyers-singleton pattern, giving thread-safe (in C++11+) lazy initialization on first use, and guaranteeing construction-before-use regardless of static-init order across translation units. The constructor pre-registers the sentinel name:
```cpp
FNamePool::FNamePool()
{
    FindOrRegister(L"None");
}
```
This is why index `0` is reserved for `"None"` — `FName()`'s default `m_Index(0)` and `IsNone()`'s `m_Index == 0` check both rely on `"None"` always being the first entry ever registered.

**Hashing.** `HashString` is a djb2 variant, using XOR-combine instead of the more common additive combine:
```cpp
uint32 FNamePool::HashString(const wchar_t* Str)
{
    uint32 Hash = 5381;
    while (*Str)
    {
        Hash = ((Hash << 5) + Hash) ^ (uint32)(*Str);
        Str++;
    }
    return Hash;
}
```
(`(Hash << 5) + Hash` is `Hash * 33`, the classic djb2 multiplier; XOR-ing in the character is the "djb2a" variant.)

**FindOrRegister — the actual interning path.** Storage is `TArray<FNameEntry> m_Entries` (dense, index-addressable) plus `TMultiMap<uint32, uint32> m_HashToIndex` mapping hash -> candidate entry indices, used to resolve hash collisions by verifying full string equality:
```cpp
uint32 FNamePool::FindOrRegister(const wchar_t* Name)
{
    check(Name != nullptr);

    const uint32 Hash = HashString(Name);

    const TArray<uint32>* pCandidates = m_HashToIndex.MultiFind(Hash);
    if (pCandidates)
    {
        for (int32 i = 0; i < pCandidates->Num(); i++)
        {
            const uint32 Index = (*pCandidates)[i];

            if (wcscmp(m_Entries[(int32)Index].m_Name, Name) == 0)
            {
                return Index;
            }
        }
    }

    const int32 Length = (int32)wcslen(Name);
    check(Length < FNameEntry::NAME_SIZE);

    FNameEntry Entry;
    FMemory::Memcpy(Entry.m_Name, Name, sizeof(wchar_t) * (Length + 1));

    const uint32 NewIndex = (uint32)m_Entries.Num();
    m_Entries.Add(Entry);
    m_HashToIndex.Add(Hash, NewIndex);

    return NewIndex;
}
```
Step by step, on every `FName` construction from a string:
1. Hash the input string with `HashString` (djb2a).
2. Ask `m_HashToIndex.MultiFind(Hash)` for the bucket of every existing entry index that hashed to the same value. This is `O(1)` amortized to reach the bucket, not `O(n)` over all names.
3. Because different strings can collide on the same 32-bit hash, every candidate in that bucket is verified with a full `wcscmp` against the stored inline buffer (`m_Entries[Index].m_Name`) before being accepted — this is the collision-safety net. If a candidate's string matches exactly, its existing index is returned and nothing new is registered.
4. If no candidate matches (empty bucket, or all `wcscmp`s failed), the name is new: its length is checked against the 64-`wchar_t` capacity (`check(Length < FNameEntry::NAME_SIZE)` — this is where the earlier-mentioned 63-character cap is enforced, and it's a hard `assert`-style `check`, not a graceful failure), the string (including its null terminator) is `Memcpy`'d into a stack-local `FNameEntry`, that entry is appended to `m_Entries` (its index becomes `NewIndex = m_Entries.Num()` *before* the `Add`, i.e. the index it will occupy), and the same `(Hash, NewIndex)` pair is inserted into `m_HashToIndex` so future lookups find it.
5. The new (or found) index is returned.

`GetEntryName` is the reverse lookup, bounds-checked and returning a pointer straight into the pool's inline storage (valid for the pool's lifetime, since `m_Entries` is append-only and never reallocated entries away — though note a `TArray` growth/reallocation on `Add` could in principle relocate the whole array in memory; callers are expected to re-fetch rather than cache the raw pointer across registrations):
```cpp
const wchar_t* FNamePool::GetEntryName(uint32 Index) const
{
    check(Index < (uint32)m_Entries.Num());
    return m_Entries[(int32)Index].m_Name;
}
```
`Num()` just forwards to `m_Entries.Num()`.

**FName itself.** The entire object is one `uint32`:
```cpp
// FName.h
private:
    uint32 m_Index;
```
Construction from `const wchar_t*` or `const FString&` guards against null/empty input (leaving `m_Index == 0`, i.e. `"None"`) and otherwise calls `FNamePool::Get().FindOrRegister(...)`:
```cpp
FName(const wchar_t* Name) : m_Index(0)
{
    if (Name && Name[0] != L'\0')
    {
        m_Index = FNamePool::Get().FindOrRegister(Name);
    }
}
```
Comparisons are pure index comparisons — this is the entire point of interning:
```cpp
bool operator==(const FName& Other) const { return m_Index == Other.m_Index; }
bool operator!=(const FName& Other) const { return m_Index != Other.m_Index; }
bool operator<(const FName& Other) const { return m_Index < Other.m_Index; }
```
`ToString()` converts back to a real, independently-owned `FString` by looking up the pooled buffer and copy-constructing an `FString` from it:
```cpp
FString ToString() const
{
    const wchar_t* pEntryName = FNamePool::Get().GetEntryName(m_Index);
    return FString(pEntryName);
}
```
This is defined **inline in the header**, not out-of-line in a `.cpp` file, and there is no `ENGINE_NOINLINE`/`noinline` attribute anywhere in the `Engine/Include/Core/String/` tree (confirmed by search — no matches for `NOINLINE` in the repo, and no `FName.cpp` exists at all). This contradicts the roadmap note in this repo's own `CLAUDE.md`, which describes `FName::ToString()` as being deliberately moved out-of-line into `FName.cpp` with `noinline` to work around an "MSVC Debug inline codegen bug." Based on the actual source present, that migration either was never carried out or was reverted — the real code has `ToString()` as an ordinary inline header member.

Also present: `IsNone()` (`m_Index == 0`), `GetIndex()` (raw index accessor), and a free-function hash specialization in `FName.h`:
```cpp
inline uint32 GetTypeHash(const FName& Name)
{
    return GetTypeHash(Name.GetIndex());
}
```
which lets `FName` be used as a key in the engine's `TMap`/`TSet`/`TMultiMap` by hashing the index (an existing `GetTypeHash(uint32)` overload from `HashFunctions.h`) rather than re-hashing the string — reinforcing that once interned, an `FName` never needs to touch its string content again for hashing or comparison.

## FText

**What it actually is.** `FText` (`Engine/Include/Core/String/FText.h/.cpp`) is a thin value-type wrapper around a single `FString` member — there is no localization table, no culture/locale key, no formatting-args support, and no separate "source string vs. displayed string" concept in this implementation. The entire private state is:
```cpp
private:
    FString m_String;
```

**Construction/assignment.** All six special members are declared and defined out-of-line in `FText.cpp`, and every one of them simply forwards to the corresponding `FString` operation:
```cpp
FText::FText() {}
FText::FText(const wchar_t* Str) : m_String(Str) {}
FText::FText(const FString& Str) : m_String(Str) {}
FText::FText(FString&& Str) : m_String(static_cast<FString&&>(Str)) {}
FText::FText(const FText& Other) : m_String(Other.m_String) {}
FText::FText(FText&& Other) noexcept : m_String(static_cast<FString&&>(Other.m_String)) {}

FText& FText::operator=(const FText& Other) { m_String = Other.m_String; return *this; }
FText& FText::operator=(FText&& Other) noexcept { m_String = static_cast<FString&&>(Other.m_String); return *this; }
```
Note the move constructor is *not* marked `noexcept` in its own signature comment context inconsistency-wise — actually checking: `FText(FText&& Other) noexcept` in the header is `noexcept`, but the corresponding definition in `FText.cpp` (`FText::FText(FText&& Other) noexcept`) matches it correctly, so both declaration and definition agree.

**Everything else is inline in the header:**
```cpp
bool operator==(const FText& Other) const { return m_String == Other.m_String; }
bool operator!=(const FText& Other) const { return m_String != Other.m_String; }
const FString& ToString() const { return m_String; }
bool IsEmpty() const { return m_String.IsEmpty(); }
```
`operator==`/`operator!=` defer directly to `FString`'s own comparison operators (length check + `wcscmp`, as detailed above). `ToString()` returns a `const FString&` — a reference to the internal buffer, not a copy (unlike `FName::ToString()`, which must materialize a new `FString` from pooled storage). `IsEmpty()` defers to `FString::IsEmpty()`.

In short, despite the "3-string-type / localization wrapper" framing suggested elsewhere in this project's planning notes, the code that actually exists implements `FText` as nothing more than an `FString` with a distinct type identity and a restricted interface (no direct `[]` indexing, no `+=`, no `Split`/`Contains`/etc. — none of `FString`'s search/transform/parse methods are exposed through `FText`). There is no localization key, no culture, and no runtime re-resolution of text by locale anywhere in `FText.h`/`FText.cpp`.
---

I have all the content needed. Here is the Markdown section.

## Logging / Assert 시스템

### 1. 로그 카테고리 — `FLogCategoryBase` / `DECLARE_LOG_CATEGORY_EXTERN` / `DEFINE_LOG_CATEGORY`

로그 카테고리는 이름 하나만 들고 있는 아주 얇은 구조체다. `Engine/Include/Core/Logging/LogMacros.h`:

```cpp
struct FLogCategoryBase
{
    const wchar_t* m_Name;
    constexpr FLogCategoryBase(const wchar_t* Name) : m_Name(Name) {}
};

#define DECLARE_LOG_CATEGORY_EXTERN(CategoryName) \
    extern FLogCategoryBase CategoryName

#define DEFINE_LOG_CATEGORY(CategoryName) \
    FLogCategoryBase CategoryName(L ## #CategoryName)
```

`DECLARE_LOG_CATEGORY_EXTERN`은 헤더에서 전역 변수를 `extern`으로 선언만 하는 매크로이고, `DEFINE_LOG_CATEGORY`는 `.cpp`에서 실제 정의를 만드는 매크로다. `L ## #CategoryName`은 매크로 인자를 문자열화(`#CategoryName`)한 뒤 와이드 문자열 접두사(`L`)를 토큰 결합(`##`)으로 붙이는 방식이라, `DEFINE_LOG_CATEGORY(LogCore)`는 `FLogCategoryBase LogCore(L"LogCore")`로 전개된다. 즉 카테고리 이름 문자열은 매크로 호출 시점에 소스 코드 텍스트로부터 자동 생성되며 별도로 손으로 타이핑하지 않는다.

`LogMacros.h`는 다음 다섯 카테고리를 선언한다.

```cpp
DECLARE_LOG_CATEGORY_EXTERN(LogCore);
DECLARE_LOG_CATEGORY_EXTERN(LogRenderer);
DECLARE_LOG_CATEGORY_EXTERN(LogPhysics);
DECLARE_LOG_CATEGORY_EXTERN(LogAI);
DECLARE_LOG_CATEGORY_EXTERN(LogUI);
```

그리고 이 다섯 개의 실제 정의는 전부 `Engine/Include/Core/Logging/FLogger.cpp` 한 곳에 모여있다.

```cpp
DEFINE_LOG_CATEGORY(LogCore);
DEFINE_LOG_CATEGORY(LogRenderer);
DEFINE_LOG_CATEGORY(LogPhysics);
DEFINE_LOG_CATEGORY(LogAI);
DEFINE_LOG_CATEGORY(LogUI);
```

카테고리별로 별도의 필터링/verbosity 오버라이드 같은 부가 정보는 없다 — `FLogCategoryBase`는 이름 문자열 하나만 가지고 있으며, 실제 verbosity 판단은 카테고리가 아니라 매 `Log()` 호출에 전달되는 `ELogVerbosity` 값으로 처리된다.

### 2. `UE_LOG` 매크로와 `FLogger::Log` 실행 흐름

`UE_LOG`는 `Category`, `Verbosity`, `Format`, 가변 인자를 받아 `FLogger::Log`를 호출하는 얇은 래퍼다 (`LogMacros.h`):

```cpp
#define UE_LOG(Category, Verbosity, Format, ...) \
    FLogger::Log(Category, ELogVerbosity::Verbosity, Format, ##__VA_ARGS__)
```

주목할 점은 `Verbosity`가 문자열이 아니라 토큰 그대로 `ELogVerbosity::` 뒤에 붙는다는 것이다. 즉 `UE_LOG(LogCore, Warning, L"test %d", 42)`는 컴파일 타임에

```cpp
FLogger::Log(LogCore, ELogVerbosity::Warning, L"test %d", 42);
```

로 치환된다 (`ELogVerbosity::Warning`이 존재하지 않으면 컴파일 에러가 난다 — 이것이 이 시스템의 "verbosity 오타 방지" 장치다).

`FLogger::Log`의 시그니처와 구현은 `Engine/Include/Core/Logging/FLogger.h` / `.cpp`에 있다.

```cpp
static void Log(const FLogCategoryBase& Category, ELogVerbosity Verbosity, const wchar_t* Format, ...);
```

실제 본문(`FLogger.cpp`):

```cpp
void FLogger::Log(const FLogCategoryBase& Category, ELogVerbosity Verbosity, const wchar_t* Format, ...)
{
    wchar_t MsgBuf[2048];
    va_list Args;
    va_start(Args, Format);
    vswprintf(MsgBuf, 2048, Format, Args);
    va_end(Args);

    wchar_t FullBuf[2176];
    swprintf(FullBuf, 2176, L"[%s] [%s] %s\n", VerbosityToString(Verbosity), Category.m_Name, MsgBuf);

    WriteToOutputs(m_pFile, FullBuf);

    if (Verbosity == ELogVerbosity::Fatal)
    {
        __debugbreak();
        ExitProcess(1);
    }
}
```

실행 흐름을 그대로 따라가면:

1. 가변 인자 목록(`va_list`)을 `vswprintf`로 고정 크기 스택 버퍼 `MsgBuf[2048]`에 포맷팅한다. 버퍼 크기 체크는 `vswprintf`의 `count` 인자(2048)로만 이루어지며, 그 이상으로 넘치는 포맷 결과는 안전하게 잘린다(내부적으로 truncate).
2. `VerbosityToString(Verbosity)`로 verbosity를 사람이 읽을 수 있는 와이드 문자열로 바꾸고, `[Verbosity] [CategoryName] Message\n` 형태로 `FullBuf[2176]`에 다시 조립한다.
3. `WriteToOutputs(m_pFile, FullBuf)`를 호출해 세 곳에 동시에 쓴다:

```cpp
static void WriteToOutputs(FILE* pFile, const wchar_t* FullBuf)
{
    wprintf(L"%s", FullBuf);
    OutputDebugStringW(FullBuf);

    if (pFile)
    {
        fwprintf(pFile, L"%s", FullBuf);
        fflush(pFile);
    }
}
```

- `wprintf`로 콘솔(표준 출력)에 출력.
- `OutputDebugStringW`로 디버거(Visual Studio 출력 창)에 출력.
- `m_pFile`(= `FLogger::Init`에서 열어둔 로그 파일)이 유효하면 `fwprintf`로 파일에도 쓰고 즉시 `fflush`한다. `fflush`를 매 로그마다 호출하므로 크래시 직전 로그도 파일에 남는다(버퍼링으로 유실되지 않음).

4. 마지막으로 `Verbosity == ELogVerbosity::Fatal`이면 `__debugbreak()`로 디버거를 즉시 정지시키고 (디버거가 붙어있지 않으면 OS의 예외 처리로 이어짐), 그 다음 `ExitProcess(1)`로 프로세스를 강제 종료한다. 이 종료 코드 경로는 예외를 던지지 않고 즉시 프로세스를 죽이는 방식으로, 이 엔진의 "예외 처리 금지(no exceptions)" 원칙과 일치한다.

`LogRaw`도 카테고리 태그만 빠졌을 뿐 동일한 패턴(포맷 → `WriteToOutputs` → Fatal 시 `__debugbreak`+`ExitProcess`)을 그대로 반복하며, `ensure()` 매크로가 이 함수를 사용한다(아래 4번 참고).

### 3. `FLogger::Init` / `Shutdown` — 로그 파일 경로 구성

```cpp
void FLogger::Init()
{
    if (m_bInitialized)
    {
        return;
    }

    m_bInitialized = true;

    wchar_t ExePath[MAX_PATH];
    GetModuleFileNameW(nullptr, ExePath, MAX_PATH);
    wchar_t* LastSlash = wcsrchr(ExePath, L'\\');

    if (LastSlash)
    {
        *(LastSlash + 1) = L'\0';
    }

    wchar_t LogDir[MAX_PATH];
    swprintf(LogDir, MAX_PATH, L"%slogs", ExePath);
    CreateDirectoryW(LogDir, nullptr);

    // filename: engine_YYYY-MM-DD_HH-MM-SS.log
    SYSTEMTIME st;
    GetLocalTime(&st);

    wchar_t FullPath[MAX_PATH];
    swprintf(FullPath, MAX_PATH, L"%s\\engine_%04d-%02d-%02d_%02d-%02d-%02d.log", LogDir, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    _wfopen_s(&m_pFile, FullPath, L"w");
}
```

경로 구성 로직:

1. `GetModuleFileNameW(nullptr, ...)`로 실행 파일(exe)의 전체 경로를 얻는다.
2. `wcsrchr(ExePath, L'\\')`로 마지막 `\` 위치를 찾고, 그 다음 문자를 `L'\0'`으로 잘라서 exe 파일명은 버리고 **디렉토리 경로만** 남긴다.
3. `swprintf(LogDir, ..., L"%slogs", ExePath)`로 exe 디렉토리 바로 밑에 `logs`라는 하위 폴더 경로 문자열을 만들고(예: `C:\Build\logs`), `CreateDirectoryW`로 실제 생성한다(이미 존재하면 실패해도 무시 — 반환값을 검사하지 않음).
4. `GetLocalTime(&st)`로 로컬 시각을 얻고, `engine_YYYY-MM-DD_HH-MM-SS.log` 형식의 타임스탬프 파일명을 만든다. 즉 실행할 때마다 새로운 로그 파일이 생성되며(초 단위까지 포함되어 있어 실질적으로 파일명 충돌은 거의 없음), 과거 로그가 덮어써지지 않는다.
5. `_wfopen_s(&m_pFile, FullPath, L"w")`로 쓰기 모드(텍스트, 매번 새로 생성/truncate)로 파일을 연다.

`Init`은 `m_bInitialized` 플래그로 중복 초기화를 막는 가드가 있고(이미 초기화됐으면 즉시 리턴), `Shutdown`은 파일 핸들이 유효하면 닫고 `m_pFile`을 `nullptr`로, `m_bInitialized`를 `false`로 되돌린다.

```cpp
void FLogger::Shutdown()
{
    if (m_pFile)
    {
        fclose(m_pFile);
        m_pFile = nullptr;
    }

    m_bInitialized = false;
}
```

### 4. `ensure(expr)` — 한 번만 발동하는 소프트 assert

`LogMacros.h`의 정의를 그대로 옮기면:

```cpp
#define ensure(expr) \
    ([&]() -> bool { \
        if (!(expr)) \
        { \
            static bool s_bFired = false; \
            if (!s_bFired) \
            { \
                s_bFired = true; \
                FLogger::LogRaw(ELogVerbosity::Warning, L"ensure() failed: " WSTR(expr)); \
                __debugbreak(); \
            } \
            return false; \
        } \
        return true; \
    }())
```

구조를 뜯어보면:

- 매크로 전체가 즉시 실행되는 람다(IIFE, immediately-invoked lambda)로 감싸여 있고, `-> bool`을 명시하므로 `ensure(expr)`는 **불리언 표현식으로 사용 가능**하다. 즉 `if (ensure(Ptr != nullptr)) { ... }`처럼 조건문 안에 직접 쓸 수 있다.
- `expr`이 참이면 그냥 `true`를 반환하고 아무 부수효과가 없다.
- `expr`이 거짓이면 함수-로컬 `static bool s_bFired`를 검사한다. 이 정적 변수는 `ensure`가 호출되는 각 소스 위치(람다 인스턴스)마다 독립적으로 존재하므로, **같은 코드 라인에 있는 `ensure` 호출은 그 최초 1회만 로그+`__debugbreak()`를 발동하고, 이후로는 계속 `false`만 반환하며 아무 것도 로그하지 않는다.** ("fires once" 패턴 — 매 프레임 호출되는 코드에 넣어도 로그가 도배되지 않게 하기 위함.)
- 실패 시 `FLogger::LogRaw(ELogVerbosity::Warning, ...)`로 경고를 남기고, 메시지 안에는 `WSTR(expr)` — 즉 `WIDEN(#expr)`을 통해 실패한 표현식 텍스트 자체가 와이드 문자열로 박혀 들어간다(`WSTR`/`WIDEN`/`WIDEN2` 매크로는 `LogMacros.h` 상단에 정의되어 있다).
- `Warning` verbosity로 로그하므로 `LogRaw` 내부에서 `Fatal`이 아니라 프로세스가 죽지는 않는다. 대신 `__debugbreak()`를 직접 호출해 디버거가 붙어있으면 그 자리에서 멈춘다. 디버거가 없으면 통상 OS 차원의 예외로 이어질 수 있는 지점이다.
- 항상 `return false`로 끝나므로(발동 여부와 무관하게), 실패했다는 사실 자체는 호출부에서 계속 알 수 있다 — 다만 로그/브레이크는 최초 1회로 억제된다.

### 5. `check(expr)` / `verify(expr)` — `EnginePCH.h`

`Engine/Include/EnginePCH.h` 상단, 주석까지 포함해 그대로 옮기면:

```cpp
// check(expr)  — Debug 전용 hard assert. Release(NDEBUG)에서는 표현식 자체가 평가되지 않으므로 부수효과 있는 호출을 넣으면 안 됨.
// verify(expr) — 항상 평가. Debug에서는 실패 시 assert, Release에서는 평가만 수행.
#define check(expr)  assert(expr)

#ifdef NDEBUG
#define verify(expr) ((void)(expr))
#else
#define verify(expr) assert(expr)
#endif
```

`check`와 `verify`는 겉으로는 비슷해 보이지만 정확히는 다르게 동작한다.

**`check(expr)`**: 조건 분기 없이 무조건 `assert(expr)`로 치환된다. `<cassert>`의 `assert`는 표준 라이브러리 매크로로, **`NDEBUG`가 정의되어 있으면(전형적으로 Release 빌드) `assert(expr)` 자체가 `((void)0)`처럼 완전히 사라진다** — 이때 `expr`은 컴파일은 되지만(문법 체크는 이루어짐) **런타임에 전혀 평가되지 않는다**. `check`는 이 표준 `assert`의 동작을 그대로 물려받으므로, 항상 `assert`와 동일하게 Debug에서만 활성화되는 "hard assert"다.

**`verify(expr)`**: `#ifdef NDEBUG` 전처리 분기로 정의 자체가 두 가지로 갈린다.
- `NDEBUG`가 정의된 빌드(Release)에서는 `((void)(expr))`로 치환된다. 이는 `expr`을 **평가는 하되** 그 결과를 버리는 코드다(`(void)` 캐스트로 "미사용 값" 경고만 억제). 즉 실패해도 프로그램이 멈추지 않지만, 부수효과(side effect)가 있는 호출이었다면 그 부수효과는 실제로 일어난다.
- `NDEBUG`가 정의되지 않은 빌드(Debug)에서는 `assert(expr)`로 치환되어, `check`와 마찬가지로 실패 시 어설션이 발동한다.

**실무적 함의**: `check(SomeFunctionWithSideEffect())`처럼 `check` 안에 부수효과가 있는 호출(예: 리스트에서 원소를 제거하고 성공 여부를 bool로 반환하는 함수, 카운터를 증가시키는 함수 등)을 넣으면, Debug 빌드에서는 그 호출이 실행되지만 Release(`NDEBUG`) 빌드에서는 `expr` 자체가 통째로 컴파일 결과물에서 사라져 **그 함수가 아예 호출되지 않는다.** 이는 Debug와 Release 사이에서 프로그램의 동작(상태 변화)이 달라지는 매우 찾기 어려운 버그로 이어진다 — Debug에서는 멀쩡히 동작하다가 Release 빌드로 넘어가는 순간 부수효과가 통째로 빠지면서 로직이 깨진다. 이런 이유로 부수효과가 있는 호출은 반드시 `check` 밖에서 먼저 실행한 뒤 그 결과(bool 등)만 `check`에 넘기거나, 애초에 `verify`(항상 평가는 보장됨)를 사용해야 한다. `EnginePCH.h`의 주석 자체가 이 점을 명시하고 있다: "Release(NDEBUG)에서는 표현식 자체가 평가되지 않으므로 부수효과 있는 호출을 넣으면 안 됨."

참고로 이 프로젝트는 `/EHs-c-`로 예외를 비활성화하고 `assert`/`ExitProcess`/`__debugbreak` 기반의 즉시 종료·중단 방식으로 오류를 처리하는 정책을 쓰고 있으며(`CLAUDE.md`의 "예외 처리 금지 — check() 매크로로 대체" 원칙과 일치), `check`/`verify`/`ensure`/`UE_LOG`의 Fatal 경로 모두 이 원칙 — 예외를 던지지 않고 `assert`, `__debugbreak`, `ExitProcess` 중 하나로 직접 프로세스를 멈추거나 종료시키는 방식 — 을 공유한다.
---

## 스마트 포인터 (TSharedPtr / TWeakPtr / TSharedRef)

경로: `Engine/Include/Core/SmartPointer/{SharedPointerInternals.h, TSharedPtr.h, TWeakPtr.h, TSharedRef.h}`

이 서브시스템은 언리얼의 `TSharedPtr` / `TWeakPtr` / `TSharedRef`를 STL(`std::shared_ptr`) 없이 직접 구현한 것이다. 소유권 공유 스마트 포인터 세 종류가 모두 하나의 공용 제어 블록(`FRefCountBlock`)을 통해 동작하며, `FSmartPtrAtomics`라는 컴파일러 분기 래퍼로 참조 카운트 증감을 원자적으로 수행한다. Phase 6 문서(CLAUDE.md)의 `FReferenceControllerBase → FRefCountBlock` 최적화 항목("멀티스레드 안전 공유 소유권")이 바로 이 구현이다.

### 1. FSmartPtrAtomics — MSVC / GCC 분기 원자 연산 래퍼

`SharedPointerInternals.h`는 표준 라이브러리의 `std::atomic` 대신, 전처리기로 MSVC 인트린식과 GCC/Clang 빌트인을 직접 분기하는 최소한의 원자 연산 래퍼를 정의한다.

```cpp
struct FSmartPtrAtomics
{
    static int32 Increment(volatile int32* pValue)
    {
#if defined(_MSC_VER)
        return (int32)_InterlockedIncrement(reinterpret_cast<volatile long*>(pValue));
#else
        return __atomic_add_fetch(pValue, 1, __ATOMIC_SEQ_CST);
#endif
    }

    static int32 Decrement(volatile int32* pValue)
    {
#if defined(_MSC_VER)
        return (int32)_InterlockedDecrement(reinterpret_cast<volatile long*>(pValue));
#else
        return __atomic_sub_fetch(pValue, 1, __ATOMIC_SEQ_CST);
#endif
    }

    static int32 Load(const volatile int32* pValue)
    {
#if defined(_MSC_VER)
        return *pValue;
#else
        return __atomic_load_n(pValue, __ATOMIC_SEQ_CST);
#endif
    }
};
```

세 함수 모두 `Increment`/`Decrement`가 "감소/증가 후의 결과값"을 리턴하는 fetch-and-add 형태(`_InterlockedIncrement`/`_InterlockedDecrement`는 원래 MSVC 인트린식 자체가 연산 후 값을 반환)라는 점이 중요하다 — 뒤에 나오는 `FRefCountBlock::ReleaseShared`/`ReleaseWeak`가 `Decrement(...) == 0`으로 "내가 마지막으로 카운트를 0으로 만든 스레드인가"를 판별하는 근거가 바로 이 반환값이다. GCC 경로는 `__ATOMIC_SEQ_CST`(순차적 일관성)를 사용해 동일한 의미를 보장한다.

주목할 점은 `Load`가 MSVC 분기에서는 `*pValue`로 단순 역참조만 한다는 것이다. `_InterlockedIncrement`/`_InterlockedDecrement`류의 MSVC 인트린식은 완전한 메모리 펜스를 동반하는 것으로 알려져 있어 x86/x64에서는 단순 읽기로도 실질적으로 안전하다고 간주하고 있는 것으로 보이나, GCC 분기처럼 명시적 `__atomic_load_n`을 쓰지 않는다는 점에서 두 분기가 완전히 대칭적인 원자성 보장을 제공하진 않는다.

### 2. FRefCountBlock — 공유/약 참조 카운트를 함께 관리하는 제어 블록

```cpp
struct FRefCountBlock
{
    volatile int32 m_SharedCount;
    volatile int32 m_WeakCount;
    FSmartPtrDeleter m_Deleter;

    FRefCountBlock(FSmartPtrDeleter InDeleter) : m_SharedCount(1), m_WeakCount(1), m_Deleter(InDeleter)
    {
    }

    void AddShared()
    {
        FSmartPtrAtomics::Increment(&m_SharedCount);
    }

    void AddWeak()
    {
        FSmartPtrAtomics::Increment(&m_WeakCount);
    }

    int32 GetSharedCount() const
    {
        return FSmartPtrAtomics::Load(&m_SharedCount);
    }

    void ReleaseShared(void* pElement)
    {
        if (FSmartPtrAtomics::Decrement(&m_SharedCount) == 0)
        {
            if (m_Deleter)
            {
                m_Deleter(pElement);
            }

            if (FSmartPtrAtomics::Decrement(&m_WeakCount) == 0)
            {
                FMemory::Free(this);
            }
        }
    }

    void ReleaseWeak()
    {
        if (FSmartPtrAtomics::Decrement(&m_WeakCount) == 0)
        {
            FMemory::Free(this);
        }
    }
};
```

`FSmartPtrDeleter`는 `void(*)(void*)` 타입의 순수 함수 포인터(`SharedPointerInternals.h` 상단에 `using FSmartPtrDeleter = void(*)(void*);`)로, RTTI/예외 없이 타입-소거된 소멸 로직을 저장하는 수단이다.

**왜 `m_WeakCount`가 1에서 시작하는가.** 생성자는 `m_SharedCount(1), m_WeakCount(1)`로 두 카운트를 모두 1로 초기화한다. `m_SharedCount = 1`은 직관적이다 — 이 블록을 만든 최초의 `TSharedPtr`가 소유자 하나이기 때문이다. 하지만 이 시점에는 아직 어떤 `TWeakPtr`도 존재하지 않는다. 그럼에도 `m_WeakCount`를 1로 잡는 이유는, "블록 자체의 메모리(`FRefCountBlock` 인스턴스)"를 누가 해제할 권한을 갖는지를 결정하는 신호로 그 슬롯을 쓰기 때문이다. 즉 이 구현은 "모든 `TSharedPtr` 소유자 그룹 전체가 암묵적으로 1개의 약 참조를 쥐고 있다"는 패턴을 채택한 것이며, 이 암묵적 약 참조는 실제 `TWeakPtr`가 하나도 없어도 존재하고, `m_SharedCount`가 0이 될 때(마지막 `TSharedPtr` 소멸 시) `ReleaseShared` 내부에서 명시적으로 반납된다. 이 설계 덕분에 "제어 블록을 실제로 언제 `FMemory::Free`할지"의 판단이 `m_WeakCount == 0` 하나의 조건으로 통일되고, `m_SharedCount`가 0이 되는 순간과 `m_WeakCount`가 0이 되는 순간이 분리되어 다음과 같은 순서를 강제할 수 있다: 객체 소멸(Deleter 호출)은 반드시 마지막 `TSharedPtr`가 사라질 때 일어나지만, 제어 블록 자체의 메모리 해제는 그보다 나중에(살아있는 `TWeakPtr`가 모두 사라진 뒤에) 일어날 수 있다.

**마지막 `TSharedPtr`가 스코프를 벗어날 때 `ReleaseShared`가 하는 일, 단계별 추적:**

1. `FSmartPtrAtomics::Decrement(&m_SharedCount)`로 공유 카운트를 원자적으로 감소시키고, 그 결과가 `0`인지 검사한다. 결과가 0이 아니면(다른 `TSharedPtr`가 아직 남아있으면) 아무 일도 하지 않고 리턴한다.
2. 결과가 0이면(내가 마지막 소유자였다면) `m_Deleter`가 유효할 때 `m_Deleter(pElement)`를 호출한다. `TSharedPtr::DefaultDeleter`가 이 함수로 등록되며(뒤에서 다룸), 이 호출로 관리 대상 객체의 소멸자가 실행되고 객체 메모리가 해제된다. 이 시점에 실제 T 객체는 완전히 파괴된다.
3. 곧바로 `FSmartPtrAtomics::Decrement(&m_WeakCount)`를 호출한다. 이것이 바로 "공유 소유자 그룹이 암묵적으로 쥐고 있던 1개의 약 참조"를 반납하는 코드다. 만약 이 시점에 살아있는 `TWeakPtr`가 하나도 없었다면 `m_WeakCount`는 정확히 1이었으므로 이 감소로 0이 되고, `FMemory::Free(this)`로 `FRefCountBlock` 자신의 메모리가 해제되며 전체 생명주기가 끝난다. 만약 살아있는 `TWeakPtr`가 N개 있었다면 `m_WeakCount`는 `1 + N`이었을 것이므로 이 감소로 `N`이 되어 아직 0이 아니고, 블록은 해제되지 않은 채 `m_SharedCount == 0` 상태로 남는다(이 상태에서 `TWeakPtr::IsValid()`는 `GetSharedCount() > 0`이 거짓이 되어 false를 반환하고, `Pin()`도 실패한다).

**멤버 `TWeakPtr`가 Deleter 체인 안에서 파괴될 때 자기 자신의 `ReleaseWeak` 감소가 왜 블록을 조기에 해제시키지 못하는가.** 만약 파괴되는 객체 T가 자기 자신을 가리키는 `TWeakPtr<T>` 멤버를 갖고 있다면, 2단계의 `m_Deleter(pElement)` 호출(= `~T()` 실행) 도중에 그 멤버 `TWeakPtr`의 소멸자가 실행되어 `TWeakPtr::ReleaseRef()` → `m_pRefCountBlock->ReleaseWeak()`가 호출된다. 이 시점에서 `m_WeakCount`는 여전히 최소 1(공유 그룹의 암묵적 몫)을 포함한 값이며, `ReleaseShared`의 3단계(자기 자신의 `Decrement(&m_WeakCount)`)는 아직 실행되기 전이다. 따라서 멤버 `TWeakPtr`의 `ReleaseWeak`가 감소시키는 것은 "실제로 그 `TWeakPtr` 인스턴스가 갖고 있던 몫"일 뿐이고, 암묵적 몫(1)은 여전히 카운트에 남아 있어 0이 될 수 없다 — 즉 `m_Deleter` 실행이 끝나기 전에는 `m_WeakCount`가 절대 0으로 떨어지지 않도록, 암묵적 1이 "마지막 안전판" 역할을 한다. `m_Deleter(pElement)` 호출이 완전히 반환된 뒤에야 `ReleaseShared`의 3단계가 실행되어 이 암묵적 몫을 반납하므로, 블록 메모리 해제는 항상 객체 소멸이 끝난 이후에만 일어난다. 이것이 `m_WeakCount`를 1로 초기화하는 설계가 존재하는 근본적인 이유다: 만약 초기값이 0이었다면, 소멸자 실행 도중에 다른 `TWeakPtr`가 하나도 남아있지 않은 상태에서 멤버 `TWeakPtr`가 자신을 해제하는 순간 `m_WeakCount`가 음수로 내려가거나(혹은 우연히 다른 참조와 맞물려) 블록이 `~T()` 실행 도중에 `FMemory::Free`되어 use-after-free가 발생할 수 있다.

`ReleaseWeak()` 자체는 훨씬 단순하다 — `TWeakPtr`가 소멸하거나 재대입될 때마다 호출되며, `m_WeakCount`를 감소시켜 0이 되면(즉 공유 그룹의 암묵적 몫 포함 모든 약 참조가 사라졌다면) `FMemory::Free(this)`로 블록을 해제한다. `m_SharedCount`가 아직 0이 아닌 상태(즉 살아있는 `TSharedPtr`가 있는 상태)에서는 애초에 `m_WeakCount`가 최소 1(암묵적 몫) 이상이므로 이 경로로 블록이 해제될 수 없다.

### 3. TSharedPtr — 소유권 공유 포인터

`TSharedPtr.h`는 원시 포인터 생성자, 복사/이동 생성자, 템플릿 업캐스트 생성자, 대입 연산자, 그리고 `MakeShared<T>` 헬퍼로 구성된다.

**원시 포인터 생성자**는 `explicit`이며, 널이 아닌 포인터에 대해서만 `FRefCountBlock`을 힙에 할당하고 placement-new로 생성한다.

```cpp
explicit TSharedPtr(T* InPtr) : m_pElement(InPtr), m_pRefCountBlock(nullptr)
{
    if (InPtr)
    {
        m_pRefCountBlock = static_cast<FRefCountBlock*>(FMemory::Malloc(sizeof(FRefCountBlock)));

        new (m_pRefCountBlock) FRefCountBlock(&TSharedPtr::DefaultDeleter);
    }
}
```

Deleter로는 정적 멤버 함수 `DefaultDeleter`가 함수 포인터로 전달되며, 이 함수가 T의 소멸자를 명시적으로 호출한 뒤 `FMemory::Free`로 메모리를 반환한다.

```cpp
static void DefaultDeleter(void* p)
{
    static_cast<T*>(p)->~T();
    FMemory::Free(p);
}
```

**복사 생성자**는 포인터와 블록을 복사한 뒤 블록이 존재하면 `AddShared()`로 카운트를 올린다. **이동 생성자**는 원본을 `nullptr`로 비우기만 할 뿐 카운트를 건드리지 않는다(소유권이 그대로 이전되므로).

```cpp
TSharedPtr(const TSharedPtr& Other) noexcept : m_pElement(Other.m_pElement), m_pRefCountBlock(Other.m_pRefCountBlock)
{
    if (m_pRefCountBlock)
    {
        m_pRefCountBlock->AddShared();
    }
}

TSharedPtr(TSharedPtr&& Other) noexcept : m_pElement(Other.m_pElement), m_pRefCountBlock(Other.m_pRefCountBlock)
{
    Other.m_pElement = nullptr;
    Other.m_pRefCountBlock = nullptr;
}
```

**템플릿 업캐스트 생성자**는 `TSharedPtr<U>`에서 `TSharedPtr<T>`로(예: 파생 클래스 → 베이스 클래스) 변환을 허용한다. `Other.m_pElement`가 `U*`이므로 `static_cast<T*>`로 포인터 타입만 바꾸고, 제어 블록은 그대로 공유하며 `AddShared()`를 호출한다. 이 접근이 가능한 이유는 클래스 하단의 `template<typename U> friend class TSharedPtr;` 선언으로 서로 다른 인스턴스화 간의 private 멤버 접근이 허용되기 때문이다.

```cpp
template<typename U>
TSharedPtr(const TSharedPtr<U>& Other) noexcept : m_pElement(static_cast<T*>(Other.m_pElement)), m_pRefCountBlock(Other.m_pRefCountBlock)
{
    if (m_pRefCountBlock)
    {
        m_pRefCountBlock->AddShared();
    }
}
```

**대입 연산자**(복사/이동 모두)는 공통적으로 먼저 `ReleaseRef()`로 현재 소유하고 있던 참조를 내려놓은 뒤, 새 상태를 대입하는 순서를 취한다.

```cpp
TSharedPtr& operator=(const TSharedPtr& Other) noexcept
{
    if (this != &Other)
    {
        ReleaseRef();
        m_pElement = Other.m_pElement;
        m_pRefCountBlock = Other.m_pRefCountBlock;

        if (m_pRefCountBlock)
        {
            m_pRefCountBlock->AddShared();
        }
    }

    return *this;
}
```

**`ReleaseRef`**는 블록이 있을 때만 `FRefCountBlock::ReleaseShared(m_pElement)`에 위임하는 얇은 래퍼다. 소멸자(`~TSharedPtr() { ReleaseRef(); }`), `Reset()`, 대입 연산자 모두 이 경로 하나로 귀결된다.

```cpp
void ReleaseRef()
{
    if (!m_pRefCountBlock)
    {
        return;
    }

    m_pRefCountBlock->ReleaseShared(m_pElement);
}
```

**`GetRefCount()`**는 블록의 `GetSharedCount()`를 그대로 노출한다:

```cpp
int32 GetRefCount() const
{
    return m_pRefCountBlock ? m_pRefCountBlock->GetSharedCount() : 0;
}
```

또한 `private` 영역에는 `TSharedPtr(T* InPtr, FRefCountBlock* InBlock)`라는 2-인자 생성자가 별도로 존재한다. 이는 카운트를 새로 만들지 않고 "이미 존재하는 블록"을 그대로 받아 붙이기 위한 내부 전용 생성자로, `TWeakPtr::Pin()`이 `AddShared()`로 미리 카운트를 올린 뒤 이 생성자로 `TSharedPtr`를 조립하는 데 사용한다(아래 4절).

**`MakeShared<T>`** — 객체와 제어 블록을 하나로 합친 단일 할당(언리얼 실제 `MakeShared`나 `std::make_shared`가 하는 최적화) 대신, 이 구현은 **별도의 두 번 할당**을 수행한다: 먼저 `FMemory::Malloc(sizeof(T), alignof(T))`로 T용 메모리를 할당하고 placement-new로 생성한 뒤, 그 결과 포인터를 `TSharedPtr<T>(Ptr)` 생성자에 넘기는데 이 생성자가 내부적으로 다시 `FMemory::Malloc(sizeof(FRefCountBlock))`을 호출해 제어 블록을 별도로 할당한다.

```cpp
template<typename T, typename... Args>
TSharedPtr<T> MakeShared(Args&&... InArgs)
{
    T* Ptr = static_cast<T*>(FMemory::Malloc(sizeof(T), alignof(T)));
    new (Ptr) T(Forward<Args>(InArgs)...);
    return TSharedPtr<T>(Ptr);
}
```

즉 이 엔진의 `MakeShared<T>`는 "단일 할당으로 캐시 지역성과 할당 횟수를 줄인다"는 표준 라이브러리식 이점을 제공하지 않는다 — 순수하게 원시 포인터 생성자를 재사용하는 편의 함수이며, 객체용 할당 1회 + `TSharedPtr(T*)` 생성자 내부의 제어 블록용 할당 1회, 총 두 번의 `FMemory::Malloc` 호출이 발생한다.

### 4. TWeakPtr — 소유권 없이 관찰만 하는 약한 참조

`TWeakPtr.h`는 `TSharedPtr`와 동일한 두 멤버(`T* m_pElement`, `FRefCountBlock* m_pRefCountBlock`)를 갖되, 자신의 존재는 오직 `m_WeakCount`에만 반영한다.

**`TSharedPtr`로부터의 생성**은 포인터와 블록을 그대로 복사한 뒤 `AddWeak()`를 호출한다.

```cpp
TWeakPtr(const TSharedPtr<T>& InSharedPtr) noexcept : m_pElement(InSharedPtr.m_pElement), m_pRefCountBlock(InSharedPtr.m_pRefCountBlock)
{
    if (m_pRefCountBlock)
    {
        m_pRefCountBlock->AddWeak();
    }
}
```

(`TSharedPtr`의 `m_pElement`/`m_pRefCountBlock`은 private이지만, `TSharedPtr.h` 하단의 `template<typename U> friend class TWeakPtr;` 선언 덕분에 `TWeakPtr`가 접근할 수 있다.)

**`IsValid()`**는 블록이 존재하고, 블록의 `GetSharedCount()`가 0보다 큰지만 검사한다 — 즉 대상 객체가 아직 파괴되지 않았는지(살아있는 `TSharedPtr`가 하나라도 있는지)를 판별한다.

```cpp
bool IsValid() const
{
    return m_pRefCountBlock != nullptr && m_pRefCountBlock->GetSharedCount() > 0;
}
```

**`Pin()`**은 약한 참조를 다시 강한 소유권(`TSharedPtr`)으로 승격시키는 연산이다. 먼저 `IsValid()`로 대상이 아직 살아있는지 확인하고(죽어있으면 빈 `TSharedPtr()`를 반환), 살아있으면 `AddShared()`로 공유 카운트를 직접 올린 뒤, `TSharedPtr`의 private 2-인자 생성자(`TSharedPtr(T*, FRefCountBlock*)`)를 이용해 카운트를 다시 올리지 않고 그대로 결과 객체를 조립한다.

```cpp
TSharedPtr<T> Pin() const
{
    if (!IsValid())
    {
        return TSharedPtr<T>();
    }

    m_pRefCountBlock->AddShared();
    return TSharedPtr<T>(m_pElement, m_pRefCountBlock);
}
```

여기서 `IsValid()` 검사와 `AddShared()` 호출 사이에는 원자적 결합(compare-and-swap 등)이 없다 — `GetSharedCount() > 0` 확인과 이어지는 `AddShared()`가 두 개의 별도 원자 연산으로 이루어져 있으므로, 진짜 멀티스레드 환경에서 그 사이에 다른 스레드가 마지막 `TSharedPtr`를 해제하면 이론적으로 TOCTOU(check-then-act) 경쟁이 존재할 수 있는 구조다. 각 개별 연산(`Load`, `Increment`, `Decrement`) 자체는 원자적이지만 그것들의 조합은 원자적이지 않다는 점에서, `FSmartPtrAtomics`가 표방하는 "원자적 참조 카운트"는 카운터 값 자체의 정합성만 보장할 뿐, `Pin()` 같은 복합 연산의 논리적 원자성까지 보장하진 않는다.

**소멸/재대입** 시 `TWeakPtr`는 항상 `ReleaseWeak()`만 호출한다(`ReleaseShared`가 아님) — 자신이 보유한 것이 약 참조뿐이기 때문이다.

```cpp
void ReleaseRef()
{
    if (!m_pRefCountBlock)
    {
        return;
    }

    m_pRefCountBlock->ReleaseWeak();
}
```

### 5. TSharedRef — 널이 될 수 없는 공유 참조

`TSharedRef.h`는 완전히 새로운 참조 카운팅 로직을 갖지 않고, 내부에 `TSharedPtr<T> m_SharedPtr` 하나만 멤버로 갖는 얇은 래퍼다. 모든 연산(`operator*`, `operator->`, `Get()`, `GetRefCount()`, 대입, 이동)은 이 내부 `TSharedPtr`에 그대로 위임된다.

```cpp
template<typename T>
class TSharedRef
{
public:
    explicit TSharedRef(T* InPtr) : m_SharedPtr(InPtr)
    {
        check(InPtr != nullptr);
    }
    ...
private:
    TSharedPtr<T> m_SharedPtr;
};
```

널 불가라는 계약은 오직 생성 시점의 `check(InPtr != nullptr)` 한 곳에서만 강제되며(CLAUDE.md 정의상 `check`는 `assert`로 치환됨), 그 이후로는 `TSharedRef`가 `TSharedPtr`처럼 자유롭게 복사·이동될 뿐 별도의 널 재검증 로직은 없다 — 즉 "생성 시 널이 아니었다"는 사실이 유지된다고 가정하고 이후 모든 `operator*`/`operator->`가 무조건 역참조 가능하다고 취급한다. `ToSharedPtr()`는 내부 `m_SharedPtr`를 값으로 복사해 반환하여, 필요할 때 다시 `TSharedPtr`(널 허용) 세계로 되돌아갈 수 있는 탈출구를 제공한다.

```cpp
TSharedPtr<T> ToSharedPtr() const 
{ 
    return m_SharedPtr; 
}
```

`GetRefCount()` 역시 `m_SharedPtr.GetRefCount()`를 그대로 반환하므로, `TSharedRef`가 참조하는 대상의 참조 카운트는 그 대상을 가리키는 모든 `TSharedPtr`/`TSharedRef` 인스턴스가 공유하는 동일한 `FRefCountBlock::m_SharedCount` 값이다 — `TSharedRef`는 카운팅 의미론에 있어 완전히 `TSharedPtr` 위에 얹힌 타입 레벨의 제약(널 금지)일 뿐, 별도의 소유권 체계를 갖지 않는다.
---

## Object 시스템 (UClass / Cast / UObject / AActor)

### 1. UClass — FName + SuperClass* + IsChildOf

`Engine/Include/Object/UClass.h`는 언리얼의 리플렉션 시스템을 흉내 내는 최소 단위 "타입 정보" 객체다. RTTI(`/GR-`)를 쓸 수 없기 때문에, 클래스 하나당 정확히 하나씩 존재하는 `UClass` 인스턴스가 타입 식별자 역할을 한다.

```cpp
// Engine/Include/Object/UClass.h
class UClass
{
public:
    UClass(const wchar_t* ClassName, UClass* InSuperClass);

    const FName& GetFName() const 
    { 
        return m_ClassName; 
    }

    UClass* GetSuperClass() const 
    { 
        return m_SuperClass; 
    }

    bool IsChildOf(const UClass* TestClass) const;

    bool IsExactClass(const UClass* TestClass) const 
    { 
        return this == TestClass; 
    }

private:
    FName   m_ClassName;
    UClass* m_SuperClass;
};
```

필드는 딱 두 개뿐이다 — `FName m_ClassName`(클래스 이름, O(1) 비교 가능한 인덱스 기반 문자열)과 `UClass* m_SuperClass`(부모 클래스의 `UClass*`, 루트 클래스는 `nullptr`). 이 `m_SuperClass` 포인터 체인이 곧 클래스 계층 트리 그 자체다.

`IsChildOf`는 `Engine/Include/Object/UClass.cpp`에 구현되어 있다.

```cpp
// Engine/Include/Object/UClass.cpp
bool UClass::IsChildOf(const UClass* TestClass) const
{
    if (!TestClass) 
    {
        return false;
    }

    const UClass* Current = this;

    while (Current)
    {
        if (Current == TestClass) 
        {
            return true;
        }

        Current = Current->m_SuperClass;
    }

    return false;
}
```

동작은 단순한 연결 리스트 순회다. `this`(예: `AActor::StaticClass()`)에서 시작해 `m_SuperClass` 포인터를 따라 루트(`UObject`, `m_SuperClass == nullptr`)까지 올라가며, 그 경로 위에 `TestClass`와 포인터가 정확히 일치하는 노드가 있는지 확인한다. `UClass` 인스턴스는 클래스마다 유일하게 하나만 존재하므로(아래 4번 항목 참고) 이 비교는 포인터 동일성 비교만으로 충분하며, 별도의 이름 비교나 RTTI가 전혀 필요 없다. `IsExactClass`는 부모 체인을 타지 않고 `this == TestClass`만 검사하는, 즉 서브클래스는 걸러내고 정확히 그 타입만 인정하는 더 엄격한 버전이다.

### 2. ObjectMacros.h — DECLARE_CLASS_ROOT / DECLARE_CLASS와 매직 스태틱

`Engine/Include/Object/ObjectMacros.h`는 위 `UClass` 인스턴스를 실제로 "생성하고 연결하는" 매크로 두 개를 정의한다.

```cpp
// Engine/Include/Object/ObjectMacros.h
#define UOBJ_WIDEN2(x)  L ## x
#define UOBJ_WIDEN(x)   UOBJ_WIDEN2(x)
#define UOBJ_WSTR(x)    UOBJ_WIDEN(#x)

class UClass;

#define DECLARE_CLASS_ROOT(TClass)                                \
public:                                                           \
    static UClass* StaticClass()                                  \
    {                                                             \
        static UClass s_ClassInfo(UOBJ_WSTR(TClass), nullptr);   \
        return &s_ClassInfo;                                      \
    }                                                             \
    virtual UClass* GetClass() const                              \
    {                                                             \
        return TClass::StaticClass();                             \
    }                                                             \
private:

#define DECLARE_CLASS(TClass, TSuperClass)                                 \
public:                                                                    \
    using Super = TSuperClass;                                             \
    static UClass* StaticClass()                                           \
    {                                                                      \
        static UClass s_ClassInfo(UOBJ_WSTR(TClass),                      \
                                  TSuperClass::StaticClass());             \
        return &s_ClassInfo;                                               \
    }                                                                      \
    virtual UClass* GetClass() const override                              \
    {                                                                      \
        return TClass::StaticClass();                                      \
    }                                                                      \
private:

#define UCLASS(...)
#define UPROPERTY(...)
#define UFUNCTION(...)
```

- `UOBJ_WSTR(x)`는 토큰을 `L"..."` 형태의 와이드 문자열 리터럴로 스트링화하는 전처리기 트릭이다 (`AActor` → `L"AActor"`). 즉 `FName` 생성자에 넘길 클래스 이름을 소스 코드의 클래스 토큰 그 자체로부터 자동 생성한다.
- `DECLARE_CLASS_ROOT(TClass)`는 상속 계층의 루트(이 코드베이스에서는 `UObject`)에만 쓰인다. `StaticClass()` 안에서 `static UClass s_ClassInfo(...)`를 함수-로컬 정적 변수로 선언하는데, 이것이 이른바 "매직 스태틱(magic static)" 패턴이다. C++11 이후 함수 지역 `static` 변수의 초기화는 스레드 안전하게 정확히 한 번만 수행되도록 표준에서 보장하며, 이 코드베이스가 전역 정적 객체(그 초기화 순서가 번역 단위 간에 미정의인 static initialization order fiasco 문제)를 쓰지 않고도 "클래스당 정확히 하나의 `UClass` 인스턴스"를 안전하게 만들어 낼 수 있는 이유다. `SuperClass`는 `nullptr`.
- `DECLARE_CLASS(TClass, TSuperClass)`는 그 외 모든 클래스에 쓰인다. 두 가지가 `DECLARE_CLASS_ROOT`와 다르다: (1) `using Super = TSuperClass;`로 언리얼 스타일의 `Super::Foo()` 호출 관용구를 가능하게 하는 타입 별칭을 만든다. (2) `s_ClassInfo` 생성 시 `SuperClass`를 `nullptr`이 아니라 `TSuperClass::StaticClass()`로 채워서, 이 클래스의 `UClass`를 부모 클래스의 `UClass`에 연결한다 — 이것이 1번 항목의 `m_SuperClass` 체인이 실제로 만들어지는 지점이다. (3) `GetClass()`가 `override`로 선언되어, 베이스에서 선언한 `virtual UClass* GetClass() const`를 재정의한다.

이 두 매크로가 만들어내는 `GetClass()` 오버라이드 체인이 사실상 이 엔진의 "동적 타입 조회" 메커니즘이다. 어떤 `UObject*`를 통해 `GetClass()`를 가상 호출하면, 실제 런타임 타입(가장 파생된 클래스)의 `StaticClass()`가 리턴되는데, 이는 vtable을 통해서만 결정되며 RTTI 없이 정확한 동적 타입 판별을 제공한다. 예를 들어 `AActor* p = new UActorComponent();`... 아니, 실제로는 `UObject* p = new AActor();`를 가정하면, `p->GetClass()`는 `AActor::StaticClass()`를 반환한다(가상 함수 디스패치 덕분에 `UObject::GetClass()`가 아니라 `AActor`가 오버라이드한 버전이 호출됨).

`UCLASS`, `UPROPERTY`, `UFUNCTION`은 언리얼 헤더 문법을 그대로 옮기고 싶어서 넣은 빈 매크로(`#define UCLASS(...)` 등)로, 실제로는 아무 코드도 생성하지 않는 스텁이다. 코드 생성기(UHT 같은 것)가 없기 때문에 순수히 가독성/문서화용 장식자로만 존재한다.

### 3. CastTemplates.h — Cast / CastChecked / ExactCast

`Engine/Include/Object/CastTemplates.h`는 위 `IsChildOf`/`IsExactClass`를 이용해 다운캐스트를 구현한다.

```cpp
// Engine/Include/Object/CastTemplates.h
template<typename T, typename U>
T* Cast(U* Obj)
{
    if (Obj && Obj->GetClass()->IsChildOf(T::StaticClass()))
    {
        return static_cast<T*>(Obj);
    }

    return nullptr;
}

template<typename T, typename U>
T* CastChecked(U* Obj)
{
    T* Result = Cast<T>(Obj);
    check(Result != nullptr);
    return Result;
}

template<typename T, typename U>
T* ExactCast(U* Obj)
{
    if (Obj && Obj->GetClass()->IsExactClass(T::StaticClass()))
    {
        return static_cast<T*>(Obj);
    }

    return nullptr;
}
```

(각각 `const U*`를 받는 오버로드도 동일한 로직으로 별도 정의되어 있다.)

동작 원리는 다음과 같다.

1. `Obj->GetClass()`는 가상 함수 호출이므로, `Obj`의 정적 타입이 `U*`라도 `Obj`가 가리키는 객체의 **실제** 런타임 클래스에 해당하는 `UClass*`를 반환한다 (2번 항목에서 설명한 vtable 기반 `GetClass()` 오버라이드 체인).
2. 그 `UClass*`에 대해 `IsChildOf(T::StaticClass())`를 호출한다 — 즉 "이 객체의 실제 클래스가 `T` 또는 `T`의 자손인가?"를 부모 체인을 걸어 올라가며 검사한다.
3. 참이면 `static_cast<T*>(Obj)`로 실제 포인터 타입을 바꿔서 반환한다. 이 `static_cast`는 원래 `dynamic_cast`가 하는 안전성 검사(런타임 타입 확인)를 대신 앞의 `IsChildOf` 검사가 이미 수행했기 때문에 안전하다고 간주된다 — RTTI 없이 다운캐스트 안전성을 확보하는 핵심 트릭이다.
4. 실패하면 `nullptr`을 반환한다.

`CastChecked<T>`는 `Cast<T>`를 호출한 뒤 결과가 `nullptr`이면 `check()`(즉 `assert`) 매크로로 즉시 중단시킨다 — "반드시 이 타입일 것"이라는 확신이 있을 때 쓰는 버전.

`ExactCast<T>`는 `IsChildOf` 대신 `IsExactClass`(포인터 동일성만 비교)를 쓰므로, `T`의 서브클래스 인스턴스는 걸러내고 정확히 `T` 타입인 객체만 통과시킨다.

### 4. TSubclassOf<T> — 타입 안전 클래스 레퍼런스

`Engine/Include/Object/TSubclassOf.h`는 `UClass*`를 그냥 들고 다니는 대신, "이 포인터는 반드시 `T` 또는 `T`의 자손 클래스를 가리킨다"는 불변식을 컴파일 타임 템플릿 파라미터와 런타임 `check()`로 함께 강제하는 얇은 래퍼다.

```cpp
// Engine/Include/Object/TSubclassOf.h
template<typename T>
class TSubclassOf
{
public:
    TSubclassOf() : m_pClass(nullptr) { }

    TSubclassOf(UClass* InClass) : m_pClass(nullptr)
    {
        if (InClass)
        {
            check(InClass->IsChildOf(T::StaticClass()));
            m_pClass = InClass;
        }
    }

    template<typename U>
    TSubclassOf(const TSubclassOf<U>& Other) : m_pClass(Other.Get())
    {
        if (m_pClass)
        {
            check(m_pClass->IsChildOf(T::StaticClass()));
        }
    }

    UClass* Get() const { return m_pClass; }
    bool IsValid() const { return m_pClass != nullptr; }
    explicit operator bool() const { return IsValid(); }
    UClass* operator->() const { check(m_pClass); return m_pClass; }

private:
    UClass* m_pClass;
};
```

내부적으로는 그냥 `UClass* m_pClass` 하나만 저장한다 — `T` 타입의 인스턴스를 보유하는 게 아니라 "그 타입 정보"만 보유한다(예: 몬스터/직업 클래스 등록 시 "이 슬롯에는 `AMonster`의 서브클래스만 들어갈 수 있다"는 식). `UClass*`를 받는 생성자와 다른 `TSubclassOf<U>`로부터의 변환 생성자 양쪽 모두에서 `check(...->IsChildOf(T::StaticClass()))`로 즉시 검증하므로, 유효하지 않은(계층상 무관한) 클래스가 대입되면 그 자리에서 assert로 걸린다. `operator->()`는 `UClass*`의 멤버(예: `GetFName()`)에 바로 접근할 수 있게 해준다.

### 5. UObject — 루트 클래스, 그리고 BeginPlay/Tick/EndPlay가 여기 있다는 사실

`Engine/Include/Object/UObject.h`:

```cpp
// Engine/Include/Object/UObject.h
class UObject
{
    DECLARE_CLASS_ROOT(UObject)

public:
    UObject() = default;
    virtual ~UObject() = default;

    virtual void BeginPlay();

    virtual void Tick(float DeltaTime);

    virtual void EndPlay();
};
```

`UObject`는 `DECLARE_CLASS_ROOT(UObject)`를 사용하는, 즉 이 엔진 클래스 계층의 진짜 루트다 (`m_SuperClass == nullptr`). 이 헤더가 선언하는 가상 함수는 딱 세 개뿐이다: `BeginPlay()`, `Tick(float DeltaTime)`, `EndPlay()`. 구현부(`Engine/Include/Object/UObject.cpp`)는 셋 다 빈 스텁이다.

```cpp
// Engine/Include/Object/UObject.cpp
void UObject::BeginPlay() { }
void UObject::Tick(float DeltaTime) { (void)DeltaTime; }
void UObject::EndPlay() { }
```

**중요한 점**: 이 세 라이프사이클 함수는 `AActor`나 `UActorComponent`가 아니라 **`UObject` 자체에** 선언되어 있다. 실제 언리얼 엔진에서는 `UObject`에 `BeginPlay`/`Tick`/`EndPlay` 개념이 존재하지 않는다 — 이것들은 `AActor`(`BeginPlay`, `Tick`은 사실 `AActor::Tick` 및 `UActorComponent::TickComponent`를 통해 `FTickFunction`이라는 별도의 틱 스케줄링 서브시스템으로 위임됨) 및 `UActorComponent` 계층에서만 도입되는 개념이며, 순수 `UObject`(예: 에셋, 데이터 컨테이너 등)는 이런 게임플레이 라이프사이클을 갖지 않는다. 이 코드베이스는 그 구분을 하지 않고 라이프사이클 함수 세 개를 최상위 `UObject`에 몰아넣었다 — 즉, 이는 실제 언리얼 아키텍처로부터 의도적으로 단순화된(그리고 명백히 갈라진) 지점이다. 결과적으로 이 엔진에서는 `UObject`를 상속하는 어떤 클래스든(게임플레이와 무관하더라도) `BeginPlay`/`Tick`/`EndPlay`를 오버라이드할 수 있는 구조가 된다.

### 6. UActorComponent / USceneComponent

`Engine/Include/Object/UActorComponent.h`:

```cpp
// Engine/Include/Object/UActorComponent.h
class UActorComponent : public UObject
{
    DECLARE_CLASS(UActorComponent, UObject)
public:
    UActorComponent();
    virtual ~UActorComponent() override;

    AActor* GetOwner() const { return m_pOwner; }
    void    SetOwner(AActor* Owner) { m_pOwner = Owner; }

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay() override;

private:
    AActor* m_pOwner;
};
```

`UActorComponent`는 `UObject`를 상속하고(`DECLARE_CLASS(UActorComponent, UObject)`로 `Super = UObject` 별칭과 `UClass` 체인을 연결) 필드 하나만 추가한다: `AActor* m_pOwner` — 이 컴포넌트를 소유한 액터에 대한 백포인터. `GetOwner()`/`SetOwner()`로 접근한다. `BeginPlay`/`Tick`/`EndPlay`를 오버라이드하지만, `.cpp`(`Engine/Include/Object/UActorComponent.cpp`)를 보면 셋 다 빈 몸체(`Tick`은 `DeltaTime`을 `(void)`로 캐스팅해 미사용 경고만 억제)로, 파생 클래스가 오버라이드할 확장 지점 역할만 한다.

`Engine/Include/Object/USceneComponent.h`:

```cpp
// Engine/Include/Object/USceneComponent.h
class USceneComponent : public UActorComponent
{
    DECLARE_CLASS(USceneComponent, UActorComponent)
public:
    USceneComponent();
    virtual ~USceneComponent() override;

    const FTransform2D& GetRelativeTransform() const { return m_RelativeTransform; }
    void SetRelativeTransform(const FTransform2D& T) { m_RelativeTransform = T; }

    FTransform2D GetWorldTransform() const;

    void SetAttachParent(USceneComponent* Parent);
    USceneComponent* GetAttachParent() const { return m_pAttachParent; }

private:
    FTransform2D              m_RelativeTransform;
    USceneComponent* m_pAttachParent;
    TArray<USceneComponent*>  m_Children;
};
```

이건 스텁이 아니라 실제 내용이 있다. `USceneComponent`는 `UActorComponent`에 트랜스폼(위치/회전/스케일을 담는 `FTransform2D`, 이 엔진이 2D이므로 `FTransform3D`가 아니라 `FTransform2D`)과 부모-자식 attach 계층을 추가한다:

- `m_RelativeTransform` — 부모(있다면) 기준 상대 트랜스폼.
- `m_pAttachParent` / `m_Children` — 단순 트리 구조. `SetAttachParent(Parent)`는 `Engine/Include/Object/USceneComponent.cpp`에 구현되어 있다:

```cpp
// Engine/Include/Object/USceneComponent.cpp
void USceneComponent::SetAttachParent(USceneComponent* Parent)
{
    m_pAttachParent = Parent;

    if (Parent)
    {
        Parent->m_Children.Add(this);
    }
}
```

  자기 자신의 `m_pAttachParent`를 갱신하고, 부모가 있으면 부모의 `m_Children` 배열에 자신을 등록한다. (참고: `Parent`가 바뀔 때 이전 부모의 `m_Children`에서 자신을 제거하는 로직은 없다 — 재부착 시 이전 부모 리스트에 유령 포인터가 남을 수 있는 단순화된 구현이다.)

- `GetWorldTransform()`은 부모 체인을 재귀적으로 타고 올라가며 계산한다:

```cpp
// Engine/Include/Object/USceneComponent.cpp
FTransform2D USceneComponent::GetWorldTransform() const
{
    if (m_pAttachParent)
    {
        return m_pAttachParent->GetWorldTransform();
    }

    return m_RelativeTransform;
}
```

  다만 이 재귀는 부모의 월드 트랜스폼을 그대로 반환할 뿐, 자기 자신의 `m_RelativeTransform`을 부모의 월드 트랜스폼과 실제로 합성(곱)하지 않는다 — 즉 부모가 있으면 자신의 상대 트랜스폼은 무시되고 루트 부모의 트랜스폼만 그대로 리턴된다. 언리얼의 `GetComponentTransform()`처럼 계층을 따라 트랜스폼을 누적 결합하는 진짜 계층적 변환 합성은 구현되어 있지 않다. 따라서 "SceneComponent에 스텁 이상의 내용이 있는가?"라는 질문에는 예 — 트랜스폼 저장과 attach 트리 자체는 실제 데이터를 갖는 진짜 구현이지만, 월드 트랜스폼 계산 로직은 부모-자식 트랜스폼을 올바르게 합성하지 않는 단순화(사실상 미완성)된 상태라고 명확히 해야 한다.

### 7. AActor — AddComponent<T>() / GetComponent<T>()와 라이프사이클 오버라이드

`Engine/Include/Object/AActor.h`:

```cpp
// Engine/Include/Object/AActor.h
class AActor : public UObject
{
    DECLARE_CLASS(AActor, UObject)
public:
    AActor();
    virtual ~AActor() override;

    template<typename T>
    T* AddComponent()
    {
        static_assert(TIsBaseOf<UActorComponent, T>::Value, "AddComponent<T>: T must derive from UActorComponent");

        void* Mem = FMemory::Malloc(sizeof(T), alignof(T));
        T* Comp = new (Mem) T();
        Comp->SetOwner(this);
        m_Components.Add(static_cast<UActorComponent*>(Comp));

        return Comp;
    }

    template<typename T>
    T* GetComponent() const
    {
        for (int32 i = 0; i < m_Components.Num(); i++)
        {
            T* Comp = Cast<T>(m_Components[i]);
            if (Comp) 
            {
                return Comp;
            }
        }

        return nullptr;
    }

    void RemoveComponent(UActorComponent* Comp);

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay() override;

private:
    TArray<UActorComponent*> m_Components;
};
```

주목할 점은 `AActor`도 `UObject`를 직접 상속한다(`DECLARE_CLASS(AActor, UObject)`) — `AActor`가 `UObject`의 자매 계층이지 `UActorComponent`의 파생이 아니라는 점은 실제 언리얼과 동일하다.

`m_Components`는 `TArray<UActorComponent*>`로 소유 컴포넌트 목록을 보관한다.

`AddComponent<T>()`는 다음 단계로 동작한다:
1. `static_assert(TIsBaseOf<UActorComponent, T>::Value, ...)`로 `T`가 컴파일 타임에 `UActorComponent`의 파생이어야 함을 강제한다 (RTTI 없이 템플릿 타입 특성으로 검증).
2. `FMemory::Malloc(sizeof(T), alignof(T))`로 엔진 전역 할당자(GMalloc, `EnginePCH`/Phase 1의 메모리 시스템)를 통해 원시 메모리를 확보한다 (`new T()` 대신 STL/기본 `new`를 우회한다는 규칙에 부합).
3. `new (Mem) T()`로 placement-new 생성한다.
4. `Comp->SetOwner(this)`로 back-pointer를 설정한다.
5. `m_Components.Add(static_cast<UActorComponent*>(Comp))`로 소유권 목록에 등록한다.
6. 파생 타입 포인터 `T*`를 반환한다.

`GetComponent<T>()`는 `m_Components`를 선형 순회하며 각 원소에 `Cast<T>(m_Components[i])`(3번 항목의 `Cast`, `GetClass()->IsChildOf(...)` 기반)를 적용해, `T` 또는 `T`의 서브클래스인 첫 컴포넌트를 찾아 반환한다. 즉 정확한 타입 매치가 아니라 "IsChildOf" 매치이므로, 베이스 컴포넌트 타입으로 조회해도 서브클래스 인스턴스를 찾아낼 수 있다.

`AActor.cpp`(`Engine/Include/Object/AActor.cpp`)의 소멸/제거/라이프사이클 구현:

```cpp
// Engine/Include/Object/AActor.cpp
AActor::~AActor()
{
    for (int32 i = 0; i < m_Components.Num(); i++)
    {
        UActorComponent* Comp = m_Components[i];
        Comp->~UActorComponent();
        FMemory::Free(Comp);
    }

    m_Components.Empty();
}

void AActor::RemoveComponent(UActorComponent* Comp)
{
    for (int32 i = 0; i < m_Components.Num(); i++)
    {
        if (m_Components[i] == Comp)
        {
            m_Components.RemoveAt(i);
            Comp->~UActorComponent();
            FMemory::Free(Comp);
            return;
        }
    }
}

void AActor::BeginPlay()
{
    for (int32 i = 0; i < m_Components.Num(); i++)
    {
        m_Components[i]->BeginPlay();
    }
}

void AActor::Tick(float DeltaTime)
{
    for (int32 i = 0; i < m_Components.Num(); i++)
    {
        m_Components[i]->Tick(DeltaTime);
    }
}

void AActor::EndPlay()
{
    for (int32 i = 0; i < m_Components.Num(); i++)
    {
        m_Components[i]->EndPlay();
    }
}
```

`AddComponent<T>()`가 `FMemory::Malloc` + placement-new로 생성했으므로, 소멸 시에도 대칭적으로 명시적 소멸자 호출(`Comp->~UActorComponent()`) + `FMemory::Free(Comp)`로 해제한다(`delete`를 쓰지 않는다). 소멸자는 모든 컴포넌트를 이 방식으로 해제한 뒤 배열을 비운다. `RemoveComponent`는 개별 컴포넌트 하나를 배열에서 찾아 같은 방식으로 파괴/해제한다.

`BeginPlay`/`Tick`/`EndPlay`는 `UObject`의 가상 함수를 오버라이드하는데, 구현 내용은 자기 자신에 대한 로직이 아니라 **소유한 모든 컴포넌트에게 그대로 위임/브로드캐스트**하는 것뿐이다 — 즉 `AActor`는 자신의 라이프사이클 이벤트가 발생하면 `m_Components`를 순회하며 각 `UActorComponent`의 동일 이벤트를 호출해주는 파사드 역할을 한다. (5번 항목에서 지적했듯 이 세 함수 자체는 `UObject`에서 물려받은 것이며, `AActor`와 `UActorComponent` 둘 다 각자 다시 오버라이드해서 "부모→자식으로 전파"하는 패턴을 만든 것이다.)
---

## Timer 시스템 (FTimerManager)

### 목적

`FTimerManager`는 언리얼의 `FTimerManager`를 그대로 본뜬, "N초 후 콜백 실행" / "N초마다 반복 콜백 실행"을 담당하는 전역 시스템이다. 몬스터 리스폰 지연(`SetTimerNextFrame`), 스킬 쿨다운 표시, 주기적 도트뎀 틱 등 시간 기반 이벤트의 기반이 된다. 델리게이트는 함수 포인터(`FTimerFunc = void(*)(void*)`) + `void* Context` 쌍으로, STL 없이도 멤버 함수 바인딩을 가능하게 하는 얇은 래퍼다.

### FTimerHandle — 식별자

`Engine/Include/Timer/FTimerHandle.h`는 단순히 `uint64 m_Handle`을 감싼 구조체다.

```cpp
struct FTimerHandle
{
    uint64 m_Handle = 0;

    bool IsValid() const { return m_Handle != 0; }
    void Invalidate() { m_Handle = 0; }
    bool operator==(const FTimerHandle& O) const { return m_Handle == O.m_Handle; }
    bool operator!=(const FTimerHandle& O) const { return m_Handle != O.m_Handle; }
};
```

`0`은 "유효하지 않은 핸들"을 의미하며, `FTimerManager`는 `m_NextHandleID`(1부터 시작하는 uint64 카운터)를 발급해 매 `SetTimer` 호출마다 고유 ID를 부여한다. `GetTypeHash`도 함께 정의되어 있어 TMap/TSet의 키로도 쓸 수 있게 되어 있다(비트 믹싱을 위한 murmur 계열 해시 마무리 연산 `Val ^= Val >> 33; Val *= 0xff51afd7ed558ccdULL; ...`).

### FTimerDelegate — 콜백 타입

`Engine/Include/Timer/FTimerDelegate.h`는 함수 포인터 기반의 정적 델리게이트다.

```cpp
using FTimerFunc = void(*)(void*);

struct FTimerDelegate
{
    FTimerFunc m_pFunc = nullptr;
    void* m_pContext = nullptr;
    ...
    void Execute() const { if (m_pFunc) m_pFunc(m_pContext); }

    static FTimerDelegate CreateStatic(FTimerFunc Func, void* Context = nullptr);

    template<typename T, void(T::* Method)()>
    static FTimerDelegate CreateRaw(T* pObj)
    {
        return FTimerDelegate(&TMemberWrapper<T, Method>, static_cast<void*>(pObj));
    }

private:
    template<typename T, void(T::* Method)()>
    static void TMemberWrapper(void* Ctx)
    {
        (static_cast<T*>(Ctx)->*Method)();
    }
};
```

`CreateRaw<T, &T::Method>(this)`를 쓰면 컴파일 타임에 멤버 함수 포인터가 템플릿 비타입 파라미터로 고정되고, 런타임에는 `TMemberWrapper`라는 캡처 없는 정적 함수만 `void*` 컨텍스트와 함께 저장된다 — std::function/가상 델리게이트 없이 멤버 함수 콜백을 구현하는 전형적인 트릭이다.

### FTimerManager — FTimerData와 API

`Engine/Include/Timer/FTimerManager.h`의 `FTimerData`가 실제 타이머 슬롯이다.

```cpp
struct FTimerData
{
    FTimerHandle m_Handle;
    FTimerDelegate m_Delegate;
    float m_Rate = 0.f;
    float m_Remaining = 0.f;
    bool m_bLoop = false;
    bool m_bPaused = false;
    bool m_bPendingRemove = false;
};
```

`m_Timers`는 `TArray<FTimerData>`로 저장되며, 삭제/정지 요청은 즉시 배열을 건드리지 않고 플래그만 세운다(순회 도중 배열이 변형되지 않도록 하는 안전장치).

`SetTimer` (`Engine/Include/Timer/FTimerManager.cpp`)는 기존 핸들이 유효하면 먼저 `ClearTimer`로 정리한 뒤 새 ID를 발급하고 `FTimerData`를 채워 `m_Timers.Add(Data)`로 밀어 넣는다. `m_Remaining`은 처음엔 `Rate`와 같은 값으로 초기화된다.

```cpp
void FTimerManager::SetTimer(FTimerHandle& OutHandle, const FTimerDelegate& Delegate, float Rate, bool bLoop)
{
    if (OutHandle.IsValid()) { ClearTimer(OutHandle); }
    OutHandle.m_Handle = m_NextHandleID++;
    FTimerData Data;
    Data.m_Handle = OutHandle;
    Data.m_Delegate = Delegate;
    Data.m_Rate = Rate;
    Data.m_Remaining = Rate;
    Data.m_bLoop = bLoop;
    m_Timers.Add(Data);
}
```

`SetTimerNextFrame`은 `Rate = 0.f`, `bLoop = false`로 `SetTimer`를 호출하는 얇은 래퍼다 — 다음 `Tick`에서 `m_Remaining(=0) <= 0.f`가 즉시 참이 되어 "다음 프레임 실행"이 구현된다.

```cpp
void FTimerManager::SetTimerNextFrame(FTimerHandle& OutHandle, const FTimerDelegate& Delegate)
{
    SetTimer(OutHandle, Delegate, 0.f, false);
}
```

`ClearTimer`는 즉시 배열에서 제거하지 않고 `m_bPendingRemove = true`만 세운 뒤 핸들을 `Invalidate()`한다. `PauseTimer`/`ResumeTimer`는 `m_bPaused` 플래그를 토글하되, 이미 제거 대기 중인(`m_bPendingRemove`) 타이머에는 적용하지 않는다. `IsTimerActive`/`IsTimerPaused`/`GetTimerRemaining`은 `FindTimer`(선형 탐색)로 핸들과 일치하는 슬롯을 찾아 상태를 조회하는 단순 헬퍼다.

### Tick — 실제 시간 진행과 콜백 발화

`Tick`이 이 시스템의 심장부다. 매 프레임 게임 루프에서 `GTimerManager->Tick(DeltaTime)`이 호출된다고 가정할 수 있다.

```cpp
void FTimerManager::Tick(float DeltaTime)
{
    const int32 Count = m_Timers.Num();

    for (int32 i = 0; i < Count; i++)
    {
        FTimerData& Data = m_Timers[i];
        if (Data.m_bPendingRemove || Data.m_bPaused)
        {
            continue;
        }

        Data.m_Remaining -= DeltaTime;
        if (Data.m_Remaining <= 0.f)
        {
            Data.m_Delegate.Execute();
            if (Data.m_bLoop)
            {
                Data.m_Remaining += Data.m_Rate;
            }
            else
            {
                Data.m_bPendingRemove = true;
            }
        }
    }

    PurgePending();
}
```

동작을 단계별로 보면:
1. `Tick` 시작 시점의 `m_Timers.Num()`을 `Count`로 캐시해 두고 그 범위만 순회한다 — 콜백 실행 도중 새 타이머가 `Add`되어도(예: 델리게이트가 또 다른 `SetTimer`를 호출) 이번 프레임에서 그 신규 항목까지 처리하지 않도록 막는 안전장치다.
2. 제거 대기 중이거나 일시정지된 타이머는 건너뛴다.
3. `m_Remaining`에서 `DeltaTime`을 뺀다. `0` 이하로 떨어지면 만료 시점이므로 `Data.m_Delegate.Execute()`로 콜백을 실행한다.
4. 루프 타이머(`m_bLoop == true`)라면 `m_Remaining += m_Rate`로 다음 주기를 재장전한다(뺄셈 후 남은 "초과분"을 그대로 이어받는 방식이라 프레임 드랍이 있어도 누적 오차 없이 다음 주기가 당겨진다). 원샷 타이머라면 `m_bPendingRemove = true`로 표시해 이번 프레임 종료 후 제거되도록 한다.
5. 순회가 끝나면 `PurgePending()`을 호출해 실제 배열 정리를 수행한다.

```cpp
void FTimerManager::PurgePending()
{
    for (int32 i = m_Timers.Num() - 1; i >= 0; i--)
    {
        if (m_Timers[i].m_bPendingRemove)
        {
            m_Timers.RemoveAtSwap(i);
        }
    }
}
```

뒤에서 앞으로 순회하며 `RemoveAtSwap`(마지막 원소를 현재 위치로 옮기고 배열을 한 칸 줄이는 O(1) 삭제)을 쓰기 때문에, 인덱스가 앞쪽에서 밀리는 문제 없이 안전하게 여러 개를 한 번에 제거할 수 있다. 순서를 보존하지 않는 대신 빠르다 — 타이머는 순서가 의미 없으므로 적절한 선택이다.

### 전역 포인터

```cpp
extern FTimerManager* GTimerManager;
```

`.cpp`에서 `FTimerManager* GTimerManager = nullptr;`로 정의된다. `GMalloc`(메모리)이나 이후 나올 `GTimerManager`처럼, 이 엔진은 전역 싱글턴 포인터를 엔진 진입점에서 `new FTimerManager()`로 초기화하고 어디서든 `GTimerManager->SetTimer(...)`로 접근하는 패턴을 일관되게 쓴다. 클래스 자체는 싱글턴을 강제하지 않으며(다수의 `FTimerManager` 인스턴스를 만들 수도 있음), `GTimerManager`는 그중 "게임이 기본으로 쓰는 하나"를 가리키는 편의 포인터일 뿐이다.

---

## Gameplay Ability System (GAS)

### 개요

`Engine/Include/Ability/` 아래의 GAS는 언리얼의 GameplayAbilitySystem을 RTTI 없이, `UClass` 기반 `Cast<T>()`와 이 엔진 자체 컨테이너(`TArray`, `TMap`, `FName`)만으로 재구현한 것이다. 핵심 축은 세 가지다: (1) **태그**(`FGameplayTag`/`FGameplayTagContainer`)로 상태와 차단 조건을 표현하고, (2) **속성**(`FGameplayAttribute`/`UAttributeSet`)이 HP/MP/ATK 같은 수치를 Base/Current 이원화로 보관하며, (3) **효과**(`UGameplayEffect`)가 속성에 가하는 변경을 시간 축(Instant/Duration/Infinite)에 따라 정의한다. 이 모든 걸 `UAbilitySystemComponent`(줄여서 ASC)가 캐릭터에 붙는 `UActorComponent`로서 관리한다.

### 1. AbilityTypes.h — 공용 열거형/구조체

`Engine/Include/Ability/AbilityTypes.h`는 헤더 전용으로, 다른 모든 GAS 파일이 참조하는 원자적 타입을 정의한다.

```cpp
enum class EGameplayEffectDurationType : uint8
{
    Instant,
    Duration,
    Infinite,
};

enum class EGameplayModifierOperation : uint8
{
    Add,
    Multiply,
    Override,
};

struct FGameplayEffectModifier
{
    FName m_AttributeName;
    EGameplayModifierOperation m_Operation = EGameplayModifierOperation::Add;
    float m_Magnitude = 0.f;
};

struct FActiveGameplayEffect
{
    UGameplayEffect* m_pSpec = nullptr;
    float m_Duration = 0.f;
    float m_PeriodTimer = 0.f;
    int32 m_StackCount = 1;
};

struct FGameplayAbilitySpec
{
    UGameplayAbility* m_pAbility = nullptr;
    int32 m_Level = 1;
    bool m_bIsActive = false;
};
```

`EGameplayEffectDurationType`은 효과의 생명주기를(즉시 소멸/타이머 만료/영구 유지), `EGameplayModifierOperation`은 속성에 값을 적용하는 방식을(더하기/곱하기/덮어쓰기) 결정한다. `FGameplayEffectModifier`는 "어떤 속성 이름에, 어떤 연산으로, 얼마의 크기를" 적용할지를 담는 최소 단위다. `FActiveGameplayEffect`는 ASC가 실제로 들고 있는 "지금 적용 중인 효과 인스턴스"로, 원본 `UGameplayEffect*`(스펙, CDO 개념에 가까움), 남은 지속시간, 주기 타이머, 스택 수를 보관한다. `FGameplayAbilitySpec`은 캐릭터가 보유한 스킬 한 개의 런타임 상태(레벨, 활성 여부)다.

### 2. FGameplayTag / FGameplayTagContainer — 계층 태그와 매칭

`FGameplayTag`(`Engine/Include/Ability/FGameplayTag.h`)는 내부적으로 `FName m_TagName` 하나만 가지는 얇은 래퍼다. 핵심은 `MatchesParent`(`Engine/Include/Ability/FGameplayTag.cpp`)의 접두사 판정 로직이다.

```cpp
bool FGameplayTag::MatchesParent(const FGameplayTag& Parent) const
{
    if (!Parent.IsValid()) { return false; }
    if (!IsValid()) { return false; }

    FString ThisStr = m_TagName.ToString();
    FString ParentStr = Parent.m_TagName.ToString();

    if (ThisStr == ParentStr) { return true; }
    if (!ThisStr.StartsWith(ParentStr)) { return false; }

    int32 ParentLen = ParentStr.Len();
    if (ThisStr.Len() > ParentLen && ThisStr[ParentLen] == L'.')
    {
        return true;
    }
    return false;
}
```

문자열을 그대로 비교/`StartsWith`한 뒤, 접두사 바로 다음 문자가 `.`인지까지 확인한다. 이 덕분에 `"Skill.Attack.Slash"`는 `"Skill.Attack"`이나 `"Skill"`의 자식으로 인식되지만, `"Skill.AttackFoo"`는 `"Skill.Attack"`의 자식으로 오판되지 않는다(단순 `StartsWith`만 썼다면 발생했을 버그를 `.` 경계 체크로 막았다).

`FGameplayTagContainer`(`Engine/Include/Ability/FGameplayTagContainer.h/.cpp`)는 `TArray<FGameplayTag> m_Tags`를 감싸는 집합이다.

- `HasTag`: 정확히 같은 태그가 있는지 선형 탐색.
- `HasParentTag(Parent)`: 컨테이너 내 태그 중 하나라도 `MatchesParent(Parent)`를 만족하면 true — "Cooldown.Slash" 태그를 가진 캐릭터에게 부모 태그 "Cooldown"을 물었을 때도 감지된다.
- `HasAnyTag(Other)` / `HasAllTags(Other)`: 다른 컨테이너와의 교집합/포함관계 검사(각각 하나라도/전부).

`AddTag`는 중복 삽입을 막기 위해 `HasTag` 체크 후 추가하고, `RemoveTag`는 `RemoveAtSwap`으로 O(1) 제거한다.

### 3. FGameplayAttribute / UAttributeSet — Base/Current 이원화

`FGameplayAttribute`(`Engine/Include/Ability/FGameplayAttribute.h`, 헤더 전용 구현)는 다음 네 값을 가진다.

```cpp
struct FGameplayAttribute
{
    float m_BaseValue = 0.f;
    float m_CurrentValue = 0.f;
    float m_MinValue = 0.f;
    float m_MaxValue = 3.4e38f;

    void SetBaseValue(float NewBase)    { m_BaseValue = FMath::Clamp(NewBase, m_MinValue, m_MaxValue); }
    void SetCurrentValue(float NewCurrent) { m_CurrentValue = FMath::Clamp(NewCurrent, m_MinValue, m_MaxValue); }
    ...
};
```

`m_BaseValue`는 "영구적으로 확정된" 값(레벨업/장비 착용 등으로 즉시 반영된 수치), `m_CurrentValue`는 매 재계산마다 Base에서 다시 파생되는 "지금 순간의" 값이다. 두 값 모두 `SetXxxValue` 시 `FMath::Clamp(v, Min, Max)`로 강제 클램프되어, 예를 들어 HP는 `[0, MaxHP]`를 절대 벗어나지 않는다.

`UAttributeSet`(`Engine/Include/Ability/UAttributeSet.h/.cpp`)은 `UObject`를 상속하는 속성 묶음이며 저장소는 `TMap<FName, FGameplayAttribute> m_Attributes`다.

```cpp
void UAttributeSet::InitAttribute(const FName& Name, float Base, float Min, float Max)
{
    FGameplayAttribute Attr(Base, Min, Max);
    m_Attributes.Add(Name, Attr);
}
```

`GetAttribute`/`GetCurrentValue`/`GetBaseValue`는 `m_Attributes.Find(Name)`로 조회하는 얇은 래퍼이며, `ResetCurrentValues`는 매 재계산 시작점에서 호출되어 모든 속성의 `CurrentValue`를 `BaseValue`로 되돌린다.

```cpp
void UAttributeSet::ResetCurrentValues()
{
    for (auto& Bucket : m_Attributes)
    {
        Bucket.Value.SetCurrentValue(Bucket.Value.GetBaseValue());
    }
}
```

### 4. UGameplayEffect — 효과 정의

`Engine/Include/Ability/UGameplayEffect.h`는 `UObject`를 상속하며, 데이터 필드가 대부분이다.

```cpp
class UGameplayEffect : public UObject
{
    DECLARE_CLASS(UGameplayEffect, UObject)
public:
    EGameplayEffectDurationType m_DurationType = EGameplayEffectDurationType::Instant;
    float m_Duration = 0.f;
    float m_Period = 0.f;
    int32 m_MaxStacks = 1;

    TArray<FGameplayEffectModifier> m_Modifiers;
    FGameplayTagContainer m_GrantedTags;
    FGameplayTagContainer m_RequiredTargetTags;
    FGameplayTagContainer m_BlockedTargetTags;

    void AddModifier(const FName& Attr, EGameplayModifierOperation Op, float Mag);
    void AddGrantedTag(const FGameplayTag& Tag);
};
```

`m_DurationType`이 Instant/Duration/Infinite 중 무엇이냐에 따라 ASC가 이 효과를 완전히 다르게 취급한다(아래 `ApplyGameplayEffect` 참고). `m_Period`는 0이면 주기 재적용 없음, 0보다 크면 그 초마다 `m_Modifiers`가 반복 적용된다(도트 데미지/힐 오버타임). `m_MaxStacks`는 같은 효과 인스턴스가 몇 번까지 겹쳐질 수 있는지를 정한다. `m_GrantedTags`는 이 효과가 활성화되어 있는 동안 대상에게 부여되는 태그(쿨다운 태그, 상태이상 태그 등)이며, `m_RequiredTargetTags`/`m_BlockedTargetTags`는 헤더에 필드는 있으나 현재 `UAbilitySystemComponent.cpp`의 로직에서는 실제로 검사되지 않는다(향후 확장 여지로 존재).

### 5. UGameplayAbility — 스킬 정의

`Engine/Include/Ability/UGameplayAbility.h`:

```cpp
class UGameplayAbility : public UObject
{
    DECLARE_CLASS(UGameplayAbility, UObject)
public:
    UGameplayEffect* m_pCostEffect;
    UGameplayEffect* m_pCooldownEffect;

    FGameplayTagContainer m_AbilityTags;
    FGameplayTagContainer m_ActivationRequiredTags;
    FGameplayTagContainer m_ActivationBlockedTags;

    virtual bool CanActivate(UAbilitySystemComponent* ASC) const;
    virtual void ActivateAbility(UAbilitySystemComponent* ASC);
    virtual void EndAbility(UAbilitySystemComponent* ASC);
};
```

`CanActivate`(`UGameplayAbility.cpp`)는 ASC의 `HasTag`를 이용해 두 조건을 검사한다: `m_ActivationBlockedTags` 중 하나라도 ASC가 가지고 있으면 즉시 실패(예: 스턴 상태 태그), `m_ActivationRequiredTags`가 있는데 그중 하나라도 ASC가 갖고 있지 않으면 실패.

```cpp
bool UGameplayAbility::CanActivate(UAbilitySystemComponent* ASC) const
{
    if (!ASC) { return false; }

    if (!m_ActivationBlockedTags.IsEmpty())
    {
        const TArray<FGameplayTag>& BlockedTags = m_ActivationBlockedTags.GetTags();
        for (int32 i = 0; i < BlockedTags.Num(); i++)
        {
            if (ASC->HasTag(BlockedTags[i])) { return false; }
        }
    }
    ...
    return true;
}
```

`ActivateAbility`는 코스트 효과와 쿨다운 효과가 설정되어 있으면 그대로 ASC에 적용한다.

```cpp
void UGameplayAbility::ActivateAbility(UAbilitySystemComponent* ASC)
{
    if (!ASC) { return; }
    if (m_pCostEffect)     { ASC->ApplyGameplayEffect(m_pCostEffect); }
    if (m_pCooldownEffect) { ASC->ApplyGameplayEffect(m_pCooldownEffect); }
}
```

`EndAbility`는 베이스 클래스에서는 아무 것도 하지 않는(`(void)ASC;`) 훅으로, 파생 스킬 클래스가 오버라이드해 실제 스킬 로직(투사체 생성, 애니메이션 재생 등)을 넣는 지점이다.

### 6. UAbilitySystemComponent — 시스템의 핵심

`UAbilitySystemComponent`(`Engine/Include/Ability/UAbilitySystemComponent.h`)는 `UActorComponent`를 상속하며, 캐릭터 액터에 붙어 다음을 소유한다.

```cpp
private:
    UAttributeSet* m_pAttributeSet;
    TArray<FActiveGameplayEffect> m_ActiveEffects;
    TArray<FGameplayAbilitySpec> m_GrantedAbilities;
    FGameplayTagContainer m_ActiveTags;
```

#### ApplyGameplayEffect — Instant vs Duration/Infinite 분기

```cpp
bool UAbilitySystemComponent::ApplyGameplayEffect(UGameplayEffect* pEffect)
{
    if (!pEffect) { return false; }

    if (pEffect->m_DurationType == EGameplayEffectDurationType::Instant)
    {
        ApplyModifiersToBase(pEffect->m_Modifiers, 1);
        return true;
    }

    if (TryStackEffect(pEffect))
    {
        RecalculateAttributes();
        return true;
    }

    FActiveGameplayEffect Active;
    Active.m_pSpec = pEffect;
    Active.m_Duration = pEffect->m_Duration;
    Active.m_PeriodTimer = pEffect->m_Period;
    Active.m_StackCount = 1;

    m_ActiveEffects.Add(Active);

    const TArray<FGameplayTag>& GrantedTags = pEffect->m_GrantedTags.GetTags();
    for (int32 i = 0; i < GrantedTags.Num(); i++)
    {
        m_ActiveTags.AddTag(GrantedTags[i]);
    }

    RecalculateAttributes();
    return true;
}
```

분기 로직:
- **Instant**: `m_ActiveEffects`에 전혀 들어가지 않는다. `ApplyModifiersToBase`가 즉시 호출되어 `BaseValue`를 직접 바꾸고, 그걸로 끝(포션 회복, 즉발 데미지).
- **Duration/Infinite**: 먼저 `TryStackEffect`로 "이미 같은 `UGameplayEffect*` 포인터를 가진 활성 효과가 있는지" 확인한다. 스택 가능(있으면 스택 카운트만 늘리고 지속시간 갱신)하면 바로 `RecalculateAttributes()` 후 반환. 새 인스턴스라면 `FActiveGameplayEffect`를 만들어 `m_ActiveEffects`에 추가하고, 효과의 `m_GrantedTags`를 `m_ActiveTags`(ASC가 보유한 활성 태그 집합)에 병합한 뒤 `RecalculateAttributes()`를 호출한다.

`TryStackEffect`:

```cpp
bool UAbilitySystemComponent::TryStackEffect(UGameplayEffect* pEffect)
{
    if (pEffect->m_MaxStacks <= 1) { return false; }

    for (int32 i = 0; i < m_ActiveEffects.Num(); i++)
    {
        if (m_ActiveEffects[i].m_pSpec == pEffect)
        {
            if (m_ActiveEffects[i].m_StackCount < pEffect->m_MaxStacks)
            {
                m_ActiveEffects[i].m_StackCount++;
                m_ActiveEffects[i].m_Duration = pEffect->m_Duration;
            }
            return true;
        }
    }
    return false;
}
```

`m_MaxStacks <= 1`인 효과는 애초에 스택 대상이 아니므로 false를 반환(호출자는 새 인스턴스를 만들게 됨). 이미 활성 중인 같은 스펙을 찾으면(포인터 동일성 비교, 즉 같은 `UGameplayEffect` CDO), 스택 상한 이내면 카운트 증가와 지속시간 리셋(재적용 시 갱신)을 하고, 스택 상한과 무관하게 항상 `true`를 반환한다 — 이미 존재하는 효과이므로 새로 추가하지 않고 여기서 처리를 끝낸다는 의미다.

#### RecalculateAttributes — Add → Multiply → Override 3-패스

이 함수가 "현재 걸려 있는 모든 지속 효과로부터 CurrentValue를 매번 처음부터 다시 계산"하는 GAS의 핵심 알고리즘이다.

```cpp
void UAbilitySystemComponent::RecalculateAttributes()
{
    if (!m_pAttributeSet) { return; }

    m_pAttributeSet->ResetCurrentValues();

    for (int32 pass = 0; pass < 3; ++pass)
    {
        EGameplayModifierOperation OpFilter;

        if (pass == 0)      { OpFilter = EGameplayModifierOperation::Add; }
        else if (pass == 1) { OpFilter = EGameplayModifierOperation::Multiply; }
        else                { OpFilter = EGameplayModifierOperation::Override; }

        for (int32 i = 0; i < m_ActiveEffects.Num(); i++)
        {
            const FActiveGameplayEffect& Active = m_ActiveEffects[i];
            UGameplayEffect* pSpec = Active.m_pSpec;
            if (!pSpec || pSpec->m_Period > 0.f)
            {
                continue;
            }

            const TArray<FGameplayEffectModifier>& Mods = pSpec->m_Modifiers;
            for (int32 m = 0; m < Mods.Num(); m++)
            {
                const FGameplayEffectModifier& Mod = Mods[m];
                if (Mod.m_Operation != OpFilter) { continue; }

                FGameplayAttribute* Attr = m_pAttributeSet->GetAttribute(Mod.m_AttributeName);
                if (!Attr) { continue; }

                float Val = Attr->GetCurrentValue();
                float Mag = Mod.m_Magnitude * Active.m_StackCount;

                if (OpFilter == EGameplayModifierOperation::Add)
                {
                    Val += Mag;
                }
                else if (OpFilter == EGameplayModifierOperation::Multiply)
                {
                    Val += Val * Mag;
                }
                else
                {
                    Val = Mod.m_Magnitude;
                }

                Attr->SetCurrentValue(Val);
            }
        }
    }
}
```

단계별 동작:
1. `ResetCurrentValues()`로 모든 속성의 `CurrentValue`를 `BaseValue`로 초기화한다 — 매 재계산은 항상 Base부터 다시 쌓아 올리는 방식이라, 효과가 하나 제거되어도 "잔여 오차"가 남지 않는다.
2. 바깥 루프는 정확히 3회 돌며 연산 종류별로 순서를 강제한다: **pass 0 = Add, pass 1 = Multiply, pass 2 = Override**. 이 순서 덕분에 "기본 100에 +20(Add) 하고 나서 최종 합계에 1.5배(Multiply)를 곱하는" 식의 결과가 모디파이어 등록 순서와 무관하게 항상 동일하게 계산되며, `Override`가 마지막에 실행되므로 다른 어떤 Add/Multiply보다 우선해 값을 강제로 덮어쓴다.
3. 각 패스 안에서 `m_ActiveEffects`를 순회하되, `pSpec->m_Period > 0.f`인 효과(주기형/도트형)는 **건너뛴다** — 이런 효과는 여기서 매 프레임 재적용되는 게 아니라 `TickActiveEffects`에서 주기가 돌 때만 `ApplyModifiersToBase`로 Base에 직접 반영되기 때문이다(즉, 순간 데미지처럼 처리됨).
4. 해당 패스의 연산 종류와 일치하는 모디파이어만 필터링해(`Mod.m_Operation != OpFilter`면 skip) 속성을 찾고, `Mag = Mod.m_Magnitude * Active.m_StackCount`로 스택 수만큼 배율을 곱한 크기를 계산한다.
5. Add는 `Val += Mag`, Multiply는 `Val += Val * Mag`(즉 `Val *= (1+Mag)`와 동일한 "증가율" 방식), Override는 스택과 무관하게 `Val = Mod.m_Magnitude`(모디파이어 원본값 그대로, 스택 곱셈 미적용)로 최종값을 강제 설정한다.
6. `Attr->SetCurrentValue(Val)`는 내부적으로 Min/Max 클램프를 거친다.

#### TickActiveEffects — 지속시간 감소, 주기 발동, 만료 처리

```cpp
void UAbilitySystemComponent::TickActiveEffects(float DeltaTime)
{
    bool bNeedRecalc = false;

    for (int32 i = m_ActiveEffects.Num() - 1; i >= 0; i--)
    {
        FActiveGameplayEffect& Active = m_ActiveEffects[i];
        UGameplayEffect* pSpec = Active.m_pSpec;
        if (!pSpec)
        {
            m_ActiveEffects.RemoveAtSwap(i);
            bNeedRecalc = true;
            continue;
        }

        if (pSpec->m_DurationType == EGameplayEffectDurationType::Infinite)
        {
            if (pSpec->m_Period > 0.f)
            {
                Active.m_PeriodTimer -= DeltaTime;
                if (Active.m_PeriodTimer <= 0.f)
                {
                    ApplyModifiersToBase(pSpec->m_Modifiers, Active.m_StackCount);
                    Active.m_PeriodTimer += pSpec->m_Period;
                    bNeedRecalc = true;
                }
            }
            continue;
        }

        Active.m_Duration -= DeltaTime;

        if (pSpec->m_Period > 0.f)
        {
            Active.m_PeriodTimer -= DeltaTime;
            if (Active.m_PeriodTimer <= 0.f)
            {
                ApplyModifiersToBase(pSpec->m_Modifiers, Active.m_StackCount);
                Active.m_PeriodTimer += pSpec->m_Period;
                bNeedRecalc = true;
            }
        }

        if (Active.m_Duration <= 0.f)
        {
            const TArray<FGameplayTag>& GrantedTags = pSpec->m_GrantedTags.GetTags();
            for (int32 t = 0; t < GrantedTags.Num(); t++)
            {
                m_ActiveTags.RemoveTag(GrantedTags[t]);
            }
            m_ActiveEffects.RemoveAtSwap(i);
            bNeedRecalc = true;
        }
    }

    if (bNeedRecalc)
    {
        RecalculateAttributes();
    }
}
```

로직을 나눠보면:
- 배열을 **뒤에서 앞으로** 순회하여 `RemoveAtSwap`으로 제거해도 인덱스가 어긋나지 않게 한다(`FTimerManager::PurgePending`과 동일한 패턴).
- 스펙이 널이면(비정상 상태) 즉시 제거.
- `Infinite` 효과는 **지속시간 차감이 없다**(`m_Duration`을 건드리지 않음) — 영구 지속이 이 분기에서 명시적으로 보장된다. 다만 `m_Period > 0.f`이면 주기 타이머만 돌려서, 예컨대 "영구 재생(Regen)" 같은 Infinite+Period 효과가 가능하다. 이 분기는 처리 후 `continue`로 아래의 Duration 감소/만료 로직을 건너뛴다.
- `Duration`(과 암묵적으로 `Instant`는 여기 들어오지 않음, ApplyGameplayEffect가 애초에 ActiveEffects에 안 넣으므로) 효과는 매 틱 `Active.m_Duration -= DeltaTime`으로 카운트다운된다.
- `m_Period > 0.f`인 경우 도트형이므로 `Active.m_PeriodTimer`가 감소하다가 0 이하가 되면 `ApplyModifiersToBase(pSpec->m_Modifiers, Active.m_StackCount)`를 호출해 **Base 값에 직접** 데미지/회복을 적용하고(RecalculateAttributes의 패스 루프가 아니라 즉발 방식), 타이머를 `+= m_Period`로 재장전한다. 이는 독 도트(초당 HP 감소)를 구현하는 지점이다.
- 지속시간이 다 되면(`Active.m_Duration <= 0.f`) 그 효과가 부여했던 `m_GrantedTags`를 `m_ActiveTags`에서 모두 제거하고, `m_ActiveEffects`에서도 제거한다(쿨다운 종료, 버프 만료 시 태그 자동 해제).
- 루프 종료 후 `bNeedRecalc`가 세워져 있으면 `RecalculateAttributes()`를 한 번만 호출해 Add/Multiply/Override 모디파이어 목록이 바뀐 것을 CurrentValue에 반영한다.

`ApplyModifiersToBase`(Base 값을 직접 조작하는 헬퍼)는 Instant 효과 적용과 Period 틱 양쪽에서 재사용된다.

```cpp
void UAbilitySystemComponent::ApplyModifiersToBase(const TArray<FGameplayEffectModifier>& Mods, int32 Stacks)
{
    if (!m_pAttributeSet) { return; }

    for (int32 i = 0; i < Mods.Num(); i++)
    {
        const FGameplayEffectModifier& Mod = Mods[i];
        FGameplayAttribute* Attr = m_pAttributeSet->GetAttribute(Mod.m_AttributeName);
        if (!Attr) { continue; }

        float Mag = Mod.m_Magnitude * Stacks;

        if (Mod.m_Operation == EGameplayModifierOperation::Add)
        {
            Attr->SetBaseValue(Attr->GetBaseValue() + Mag);
        }
        else if (Mod.m_Operation == EGameplayModifierOperation::Multiply)
        {
            float Base = Attr->GetBaseValue();
            Attr->SetBaseValue(Base + Base * Mag);
        }
        else
        {
            Attr->SetBaseValue(Mod.m_Magnitude);
        }
        Attr->SetCurrentValue(Attr->GetBaseValue());
    }
}
```

여기서는 `RecalculateAttributes`처럼 매번 처음부터 다시 계산하는 게 아니라, 지금 Base 값에 **누적으로 가감**한다 — 즉 HP 포션을 두 번 먹으면 Base HP가 두 번 다 반영되어 그대로 남는다(반면 `Infinite`/`Duration` 버프의 Add/Multiply 모디파이어는 Base를 건드리지 않고 매번 `RecalculateAttributes`가 Base로부터 CurrentValue를 재구성하므로, 버프가 사라지면 흔적 없이 원상복구된다). 이 차이가 "Instant는 영구 반영, Duration/Infinite는 걸려 있는 동안만 반영"이라는 GAS의 근본 원칙을 코드 레벨에서 구현한다.

#### RemoveEffectsWithTag / RemoveEffectsOfClass

```cpp
void UAbilitySystemComponent::RemoveEffectsWithTag(const FGameplayTag& Tag)
{
    for (int32 i = m_ActiveEffects.Num() - 1; i >= 0; i--)
    {
        UGameplayEffect* pSpec = m_ActiveEffects[i].m_pSpec;
        if (pSpec && pSpec->m_GrantedTags.HasTag(Tag))
        {
            const TArray<FGameplayTag>& GrantedTags = pSpec->m_GrantedTags.GetTags();
            for (int32 t = 0; t < GrantedTags.Num(); t++)
            {
                m_ActiveTags.RemoveTag(GrantedTags[t]);
            }
            m_ActiveEffects.RemoveAtSwap(i);
        }
    }
    RecalculateAttributes();
}
```

`m_GrantedTags`에 지정 태그를 가진 활성 효과를 전부 찾아 제거하고, 그 효과가 부여했던 태그도 함께 해제한 뒤 `RecalculateAttributes()`로 마무리한다 — "정화(Cleanse)" 스킬이 `Tag = "Status"`로 이걸 호출하면 상태이상 계열 효과가 한 번에 제거되는 식이다. `RemoveEffectsOfClass`는 태그가 아니라 `UGameplayEffect*` 포인터 동일성으로 특정 효과 인스턴스(스펙)를 지정해 제거하는 버전으로, 로직은 동일하고 매칭 조건만 `m_pSpec == pEffect`로 다르다 — 장비 해제 시 그 장비가 부여했던 정확한 `UGameplayEffect`를 제거하는 데 적합하다.

#### GrantAbility / TryActivateAbility / TryActivateAbilityByTag

```cpp
int32 UAbilitySystemComponent::GrantAbility(UGameplayAbility* pAbility, int32 Level)
{
    FGameplayAbilitySpec Spec;
    Spec.m_pAbility = pAbility;
    Spec.m_Level = Level;
    Spec.m_bIsActive = false;
    m_GrantedAbilities.Add(Spec);
    return m_GrantedAbilities.Num() - 1;
}
```

스킬을 배열에 추가하고 인덱스를 반환한다(퀵슬롯이 이 인덱스를 들고 있으면 됨).

```cpp
bool UAbilitySystemComponent::TryActivateAbility(int32 SpecIndex)
{
    if (SpecIndex < 0 || SpecIndex >= m_GrantedAbilities.Num()) { return false; }

    FGameplayAbilitySpec& Spec = m_GrantedAbilities[SpecIndex];
    if (!Spec.m_pAbility) { return false; }
    if (!Spec.m_pAbility->CanActivate(this)) { return false; }

    Spec.m_bIsActive = true;
    Spec.m_pAbility->ActivateAbility(this);
    return true;
}
```

인덱스 유효성 검사 → `CanActivate`(차단/필수 태그 검사, 사실상 여기서 쿨다운/스턴 체크가 이뤄짐) → 통과 시 활성 플래그를 세우고 `ActivateAbility`(코스트/쿨다운 효과 적용)를 실행한다. `TryActivateAbilityByTag`는 `m_GrantedAbilities`를 순회하며 `m_AbilityTags.HasTag(AbilityTag)`로 스킬을 찾아 `TryActivateAbility(i)`에 위임한다 — 퀵슬롯 UI가 스킬 인덱스 대신 태그로 스킬을 지정할 수 있게 해준다.

#### AddLooseTag / RemoveLooseTag, HasTag / HasParentTag

```cpp
bool UAbilitySystemComponent::HasTag(const FGameplayTag& Tag) const { return m_ActiveTags.HasTag(Tag); }
bool UAbilitySystemComponent::HasParentTag(const FGameplayTag& Tag) const { return m_ActiveTags.HasParentTag(Tag); }
void UAbilitySystemComponent::AddLooseTag(const FGameplayTag& Tag) { m_ActiveTags.AddTag(Tag); }
void UAbilitySystemComponent::RemoveLooseTag(const FGameplayTag& Tag) { m_ActiveTags.RemoveTag(Tag); }
```

"Loose Tag"는 어떤 `UGameplayEffect`에도 종속되지 않고 직접 `m_ActiveTags`에 추가/제거하는 태그다(효과 만료 시 자동 해제되는 `m_GrantedTags`와 달리, 게임 코드가 명시적으로 넣고 빼야 함). 예를 들어 "지금 사다리를 타고 있음" 같은, 효과 시스템과 무관한 순수 상태 플래그에 적합하다.

#### Tick

```cpp
void UAbilitySystemComponent::Tick(float DeltaTime)
{
    TickActiveEffects(DeltaTime);
}
```

`UActorComponent::Tick`을 오버라이드해 매 프레임 `TickActiveEffects`만 호출한다 — GAS의 시간 기반 로직(지속시간 카운트다운, 주기 발동, 만료)은 전부 여기 하나로 수렴한다.

### 7. 메이플스토리 패턴 → GAS 매핑

이 시스템의 필드/분기 구조를 보면 아래와 같이 구체적으로 매핑된다.

- **패시브 스킬(예: ATK +20% 영구)**: `EGameplayEffectDurationType::Infinite`인 `UGameplayEffect`를 만들어 `AddModifier(L"ATK", EGameplayModifierOperation::Multiply, 0.2f)`로 등록하고, 스킬 습득 시 한 번 `ApplyGameplayEffect`. `TickActiveEffects`에서 `Infinite`는 `Active.m_Duration`을 절대 감소시키지 않으므로(위 코드의 `if (Infinite) { ...; continue; }` 분기) 명시적으로 `RemoveEffectsOfClass`/`RemoveEffectsWithTag`를 호출하기 전까지 영구히 `RecalculateAttributes`의 Add/Multiply 패스에 계속 참여한다.
- **독 도트(1초마다 HP -50)**: `EGameplayEffectDurationType::Duration`(지속시간이 다 되면 효과 자체가 사라지도록) + `m_Period = 1.0f` + `AddModifier(L"HP", Add, -50.f)`. `TickActiveEffects`가 매초 `ApplyModifiersToBase`를 호출해 Base HP를 직접 깎으며, `RecalculateAttributes`의 패스 루프에서는 `pSpec->m_Period > 0.f`인 효과를 건너뛰므로 이 도트 자체는 Add/Multiply/Override 3-패스 대상이 아니다(순수 즉발 피해의 반복일 뿐).
- **힐 포션(HP 즉시 +500)**: `EGameplayEffectDurationType::Instant` + `AddModifier(L"HP", Add, 500.f)`. `ApplyGameplayEffect`가 `m_ActiveEffects`에 전혀 넣지 않고 `ApplyModifiersToBase`만 즉시 실행 후 끝.
- **스킬 쿨다운(3초)**: `UGameplayAbility::m_pCooldownEffect`에 `EGameplayEffectDurationType::Duration`, `m_Duration = 3.f`, `AddGrantedTag(FGameplayTag(L"Cooldown.Slash"))`로 설정된 `UGameplayEffect`를 연결한다. 스킬 사용 시 `ActivateAbility`가 이를 `ApplyGameplayEffect`로 걸고, 그 스킬의 `m_ActivationBlockedTags`에 `"Cooldown.Slash"`(또는 부모 태그 `"Cooldown"` — `HasTag`가 아니라 `HasParentTag` 계열을 쓰면 상위 매칭도 가능)를 넣어 두면, 쿨다운 중에는 `CanActivate`의 차단 태그 검사에서 걸려 재발동이 막힌다. 3초 후 `TickActiveEffects`가 만료를 감지해 `Cooldown.Slash` 태그를 `m_ActiveTags`에서 제거하면 다시 활성화 가능.
- **장비 스탯(활: ATK +200)**: `EGameplayEffectDurationType::Infinite` + `AddModifier(L"ATK", Add, 200.f)`, 장착 시 `ApplyGameplayEffect`. 해제 시에는 `RemoveEffectsOfClass(pWeaponEffect)`(정확히 그 무기가 만든 효과 인스턴스를 지목)를 호출해 `m_ActiveEffects`에서 제거하고 `RecalculateAttributes`가 재계산되면서 ATK 보너스가 사라진다.
- **크리티컬 충전 스택**: `m_MaxStacks = 5`인 `Duration` 효과. 스택을 쌓을 때마다 같은 `UGameplayEffect*`로 `ApplyGameplayEffect`를 호출하면 `TryStackEffect`가 이를 감지해 `m_StackCount++`(상한 5까지)와 `m_Duration` 리셋을 수행한다. `RecalculateAttributes`에서 `Mag = Mod.m_Magnitude * Active.m_StackCount`이므로, 스택마다 크리티컬 확률/피해량 모디파이어가 선형으로 배가되어 반영된다(단, Override 연산은 스택을 곱하지 않고 `Mod.m_Magnitude` 그대로 덮어씀에 유의).