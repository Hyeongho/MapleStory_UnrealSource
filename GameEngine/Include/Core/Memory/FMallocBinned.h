#pragma once
#include "IAllocator.h"


class FMallocBinned : public IAllocator
{
public:
    FMallocBinned();
    ~FMallocBinned() override;

    void* Malloc(size_t size, uint32 alignment) override;
    void* Realloc(void* ptr, size_t newSize, uint32 alignment) override;
    void  Free(void* ptr) override;

    static const int32 NUM_BINS = 6;                 // 16, 32, 64, 128, 256, 512
    static const int32 MAX_SMALL_SIZE = 512;
    static const uint32 BIN_MAX_ALIGN = 16;
    static const size_t PAGE_SIZE = 65536;           // 64 KB, pages are PAGE_SIZE-aligned
    static const uint32 PAGE_MAGIC = 0xB17EED00u;
    static const int32 LARGE_BIN = -1;

private:
    struct FFreeBlock
    {
        FFreeBlock* m_pNext;
    };

    struct FPageHeader
    {
        uint32 m_Magic;
        int32 m_BinIndex;      // 0..NUM_BINS-1, or LARGE_BIN
        size_t m_AllocSize;     // user-requested size (for Realloc copy)
        FPageHeader* m_pNextPage;    // intrusive list of all pages (for shutdown)
    };

    // HEADER_SIZE is 16-aligned and >= sizeof(FPageHeader); the first block
    // therefore starts 16-aligned, matching BIN_MAX_ALIGN.
    static const size_t HEADER_SIZE = 32;

    int32 SizeToBin(size_t size) const;
    void GrowBin(int32 binIndex);
    void* AllocateLarge(size_t size, uint32 alignment);

    static FPageHeader* PageOf(void* ptr);

    FFreeBlock* m_FreeLists[NUM_BINS];
    int32 m_BinBlockSize[NUM_BINS];
    FPageHeader* m_pAllPages;        // every page we allocated (small + large)
};
