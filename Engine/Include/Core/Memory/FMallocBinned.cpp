#include "EnginePCH.h"
#include "FMallocBinned.h"

FMallocBinned::FMallocBinned() : m_pAllPages(nullptr)
{
    static_assert(sizeof(FPageHeader) <= HEADER_SIZE, "FPageHeader must fit in the reserved page header area");
    static_assert((HEADER_SIZE % BIN_MAX_ALIGN) == 0, "first block must start BIN_MAX_ALIGN-aligned");

    int32 size = 16;
    for (int32 i = 0; i < NUM_BINS; i++)
    {
        m_FreeLists[i] = nullptr;
        m_BinBlockSize[i] = size;
        size *= 2;
    }
}

FMallocBinned::~FMallocBinned()
{
    // Return every reserved page to the OS.
    FPageHeader* pPage = m_pAllPages;
    while (pPage)
    {
        FPageHeader* pNext = pPage->m_pNextPage;
        _aligned_free(pPage);
        pPage = pNext;
    }
    m_pAllPages = nullptr;
}

int32 FMallocBinned::SizeToBin(size_t size) const
{
    for (int32 i = 0; i < NUM_BINS; i++)
    {
        if (size <= (size_t)m_BinBlockSize[i])
        {
            return i;
        }
    }
    return LARGE_BIN;
}

FMallocBinned::FPageHeader* FMallocBinned::PageOf(void* ptr)
{
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t base = addr & ~(uintptr_t)(PAGE_SIZE - 1);
    return reinterpret_cast<FPageHeader*>(base);
}

void FMallocBinned::GrowBin(int32 binIndex)
{
    const size_t blockSize = (size_t)m_BinBlockSize[binIndex];

    void* pRaw = _aligned_malloc(PAGE_SIZE, PAGE_SIZE);
    check(pRaw != nullptr);
    check((reinterpret_cast<uintptr_t>(pRaw) & (PAGE_SIZE - 1)) == 0);

    FPageHeader* pHeader = reinterpret_cast<FPageHeader*>(pRaw);
    pHeader->m_Magic = PAGE_MAGIC;
    pHeader->m_BinIndex = binIndex;
    pHeader->m_AllocSize = 0;

    pHeader->m_pNextPage = m_pAllPages;
    m_pAllPages = pHeader;

    // Carve the remainder of the page into blocks and push them onto the bin.
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

void* FMallocBinned::AllocateLarge(size_t size, uint32 alignment)
{
    if (alignment < BIN_MAX_ALIGN)
    {
        alignment = BIN_MAX_ALIGN;
    }

    // The payload must stay inside the first page so PageOf(ptr) can recover
    // the header by masking. Any alignment below PAGE_SIZE satisfies this.
    check((size_t)alignment < PAGE_SIZE);

    // One or more whole pages so masking any payload address still yields the
    // page header. Reserve HEADER_SIZE (plus alignment slack) up front.
    size_t needed = HEADER_SIZE + alignment + size;
    size_t totalSize = (needed + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);

    void* pRaw = _aligned_malloc(totalSize, PAGE_SIZE);
    check(pRaw != nullptr);
    check((reinterpret_cast<uintptr_t>(pRaw) & (PAGE_SIZE - 1)) == 0);

    FPageHeader* pHeader = reinterpret_cast<FPageHeader*>(pRaw);
    pHeader->m_Magic = PAGE_MAGIC;
    pHeader->m_BinIndex = LARGE_BIN;
    pHeader->m_AllocSize = size;

    pHeader->m_pNextPage = m_pAllPages;
    m_pAllPages = pHeader;

    uintptr_t payload = reinterpret_cast<uintptr_t>(pRaw) + HEADER_SIZE;
    payload = (payload + (alignment - 1)) & ~(uintptr_t)(alignment - 1);

    return reinterpret_cast<void*>(payload);
}

void* FMallocBinned::Malloc(size_t size, uint32 alignment)
{
    if (size == 0)
    {
        size = 1;
    }

    if (size <= (size_t)MAX_SMALL_SIZE && alignment <= BIN_MAX_ALIGN)
    {
        const int32 bin = SizeToBin(size);
        if (bin != LARGE_BIN)
        {
            if (!m_FreeLists[bin])
            {
                GrowBin(bin);
            }

            FFreeBlock* pBlock = m_FreeLists[bin];
            check(pBlock != nullptr);
            m_FreeLists[bin] = pBlock->m_pNext;
            return pBlock;
        }
    }

    return AllocateLarge(size, alignment);
}

void FMallocBinned::Free(void* ptr)
{
    if (!ptr)
    {
        return;
    }

    FPageHeader* pHeader = PageOf(ptr);
    check(pHeader->m_Magic == PAGE_MAGIC);
    check(pHeader->m_BinIndex == LARGE_BIN || (pHeader->m_BinIndex >= 0 && pHeader->m_BinIndex < NUM_BINS));

    if (pHeader->m_BinIndex == LARGE_BIN)
    {
        // Unlink from the page list, then release to the OS.
        FPageHeader** ppCursor = &m_pAllPages;
        while (*ppCursor && *ppCursor != pHeader)
        {
            ppCursor = &(*ppCursor)->m_pNextPage;
        }
        if (*ppCursor == pHeader)
        {
            *ppCursor = pHeader->m_pNextPage;
        }

        _aligned_free(pHeader);
        return;
    }

    const int32 bin = pHeader->m_BinIndex;
    FFreeBlock* pBlock = reinterpret_cast<FFreeBlock*>(ptr);
    pBlock->m_pNext = m_FreeLists[bin];
    m_FreeLists[bin] = pBlock;
}

void* FMallocBinned::Realloc(void* ptr, size_t newSize, uint32 alignment)
{
    if (!ptr)
    {
        return Malloc(newSize, alignment);
    }

    if (newSize == 0)
    {
        Free(ptr);
        return nullptr;
    }

    FPageHeader* pHeader = PageOf(ptr);
    check(pHeader->m_Magic == PAGE_MAGIC);
    check(pHeader->m_BinIndex == LARGE_BIN || (pHeader->m_BinIndex >= 0 && pHeader->m_BinIndex < NUM_BINS));

    size_t oldSize;
    if (pHeader->m_BinIndex == LARGE_BIN)
    {
        oldSize = pHeader->m_AllocSize;
    }
    else
    {
        oldSize = (size_t)m_BinBlockSize[pHeader->m_BinIndex];
    }

    void* pNew = Malloc(newSize, alignment);
    const size_t copySize = (oldSize < newSize) ? oldSize : newSize;
    FMemory::Memcpy(pNew, ptr, copySize);
    Free(ptr);

    return pNew;
}
