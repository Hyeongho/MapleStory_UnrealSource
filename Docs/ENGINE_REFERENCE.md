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

### IAllocator — 추상 얼로케이터 인터페이스

**목적:** 모든 구체 얼로케이터(`FMallocAnsi`, `FMallocBinned`)가 반드시 구현해야 하는 최소한의 가상 계약(`Malloc`/`Realloc`/`Free`)을 정의한다. 이를 통해 엔진의 나머지 부분은 단일 전역 포인터 뒤에 숨겨진 구체적인 할당 전략과 분리될 수 있다.

**코드** (`Engine/Include/Core/Memory/IAllocator.h`):
```cpp
struct IAllocator
{
	virtual void* Malloc(size_t size, uint32 alignment) = 0;
	virtual void* Realloc(void* ptr, size_t newSize, uint32 alignment) = 0;
	virtual void  Free(void* ptr) = 0;
	virtual ~IAllocator() = default;
};
```

**런타임 동작:** 이 구조체는 순수 추상 베이스로, `.cpp` 파일이 없으며 그 자체로는 런타임에 아무 동작도 하지 않는다. 오직 vtable 계약으로서만 존재한다. `IAllocator*`를 통한 모든 호출은 현재 바인딩된 구체 얼로케이터(`FMallocAnsi` 또는 `FMallocBinned`, 아래 `FMemory`/`GMalloc` 참고)로 가상 디스패치된다. 가상 소멸자는 베이스 포인터를 통해 삭제되더라도 파생 얼로케이터가 올바르게 소멸되도록 보장한다.

**공개 API 목록:**
| 멤버 | 설명 |
|---|---|
| `virtual void* Malloc(size_t size, uint32 alignment)` | `alignment`에 맞춰 정렬된 `size` 바이트를 할당 |
| `virtual void* Realloc(void* ptr, size_t newSize, uint32 alignment)` | 기존 할당의 크기 조정/재할당 |
| `virtual void Free(void* ptr)` | 기존 할당 해제 |
| `virtual ~IAllocator()` | 기본 가상 소멸자 |

---

### FMallocAnsi — 얇은 `_aligned_malloc` 래퍼

**목적:** CRT의 정렬 할당 함수를 그대로 전달하는 최소한의 `IAllocator` 구현체다. 버킷/풀링 로직이 전혀 없는, 가능한 가장 단순한 얼로케이터로 사용된다.

**코드** (`Engine/Include/Core/Memory/FMallocAnsi.h` 및 `.cpp`):
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

**단계별 런타임 동작:**
1. `Malloc(size, alignment)`은 MSVC CRT의 `_aligned_malloc`으로 그대로 위임된다. 이 함수는 내부적으로 실제 필요한 크기보다 더 많이 할당하고, 반환된 포인터 앞에 작은 헤더를 저장해 두어 나중에 해제할 때 실제 블록 시작 위치를 찾을 수 있게 한다 — 이 기록 관리는 전적으로 CRT 내부에서 이루어지며 이 클래스에게는 보이지 않는다.
2. `Realloc`도 마찬가지로 `_aligned_realloc`으로 위임되는데, CRT는 이를 제자리에서 확장하거나 새로 할당 후 복사하고 기존 블록을 해제하는 방식으로 구현할 수 있으며, 이 역시 투명하게 처리된다.
3. `Free`는 `_aligned_free`로 위임되며, 이는 CRT 내부 헤더를 이용해 실제 할당 위치를 찾아 해제한다.
4. bin/pool/추적 로직은 전혀 없다 — 모든 호출은 1:1 통과다. 이 때문에 `FMallocAnsi`는 사실상 `FMallocBinned`가 대체한 "최적화되지 않은" 기본 얼로케이터에 해당한다(`CLAUDE.md`의 Phase 7.5+ 최적화 항목 참고: "현재: `_aligned_malloc` 래핑... 목표: 크기 클래스 버킷 방식").

**공개 API 목록:**
| 멤버 | 설명 |
|---|---|
| `Malloc(size, alignment)` | `_aligned_malloc(size, alignment)` |
| `Realloc(ptr, newSize, alignment)` | `_aligned_realloc(ptr, newSize, alignment)` |
| `Free(ptr)` | `_aligned_free(ptr)` |

---

### FMallocBinned — 64KB 페이지 기반 크기 클래스 Bin 얼로케이터

**목적:** 64KB 정렬 페이지에서 잘라낸 프리 리스트로 소형 할당(≤512바이트, ≤16바이트 정렬)을 처리하고, 그 외의 경우에는 페이지 정렬된 "대형 할당" 경로로 폴백하는 버킷/Bin 방식 얼로케이터다. 이를 통해 소형 객체에 대한 할당 단위 CRT 오버헤드와 외부 단편화를 제거한다.

**코드** (`Engine/Include/Core/Memory/FMallocBinned.h`):
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

Bin 테이블 초기화 (`Engine/Include/Core/Memory/FMallocBinned.cpp`):
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

포인터 마스킹을 통한 페이지 → 헤더 복원:
```cpp
FMallocBinned::FPageHeader* FMallocBinned::PageOf(void* ptr)
{
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t base = addr & ~(uintptr_t)(PAGE_SIZE - 1);
    return reinterpret_cast<FPageHeader*>(base);
}
```

Bin 확장 (페이지 절단):
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

`Free` (bin/페이지를 찾기 위한 포인터 마스킹, 소형 블록에는 포인터별 크기 조회가 필요 없음):
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

**단계별: `Malloc(size, alignment)`**
1. `size == 0`이면 `1`로 올림된다(크기가 0인 요청도 유효하고 해제 가능한 포인터를 반환하도록 하기 위함).
2. **소형 경로 판정:** `size <= 512`이고 `alignment <= 16`이면 `SizeToBin(size)`가 `m_BinBlockSize[0..5]`(16, 32, 64, 128, 256, 512)를 선형으로 스캔하여 블록 크기가 `size` 이상인 첫 번째 인덱스를 반환한다. 그렇지 않으면 `LARGE_BIN`을 반환한다.
3. 유효한 bin이 발견되면: 해당 bin의 프리 리스트(`m_FreeLists[bin]`)가 비어 있을 경우 `GrowBin(bin)`을 호출해 새 페이지를 잘라낸다(아래 참고).
4. 프리 리스트의 head 노드(`FFreeBlock* pBlock`)를 팝하고(`m_FreeLists[bin] = pBlock->m_pNext`), 그 주소를 그대로 사용자 포인터로 반환한다 — 할당당 헤더 검색 없이 O(1) 팝이다.
5. 크기/정렬이 어떤 bin에도 맞지 않으면 `AllocateLarge`로 넘어간다.

**단계별: `GrowBin(binIndex)` (페이지 절단)**
1. `_aligned_malloc(PAGE_SIZE, PAGE_SIZE)`를 통해 한 페이지 전체(`PAGE_SIZE`, 64KB) 블록을 할당하며, 페이지의 기준 주소 자체가 64KB 정렬되도록 보장한다(`check((addr & (PAGE_SIZE-1)) == 0)`로 검증).
2. 페이지의 처음 `HEADER_SIZE`(32) 바이트는 `FPageHeader`가 된다: 매직 넘버 `PAGE_MAGIC`, 소유 `m_BinIndex`, `m_AllocSize = 0`(소형 bin에서는 미사용)이 설정되며, 인트루시브 `m_pAllPages` 리스트에 연결된다(앞쪽에 삽입).
3. 나머지 `PAGE_SIZE - HEADER_SIZE` 바이트는 `blockSize` 크기 슬롯의 원시 배열로 취급된다(`blockCount = usable / blockSize`).
4. 각 슬롯을 순회하며 `FFreeBlock` 노드로서 `m_FreeLists[binIndex]`에 밀어 넣어 프리 리스트를 뒤에서 앞으로 구성한다(각 새 노드가 앞쪽에 삽입되므로 메모리상 마지막 슬롯이 리스트의 꼬리에 놓이게 된다). 이 과정 후 해당 bin은 한 페이지 분량의 프리 블록을 가지게 된다.

**단계별: `Free(ptr)` (포인터 마스킹)**
1. `PageOf(ptr)`는 포인터의 하위 `PAGE_SIZE-1` 비트를 마스킹하여(`addr & ~(PAGE_SIZE-1)`) 페이지의 기준 주소로 바로 이동한다 — 이는 모든 페이지(소형 bin이든 대형이든)가 64KB 정렬로 할당되기 때문에 가능하며, 페이지 내부의 *어떤* 포인터든 오프셋 0에 위치한 `FPageHeader`와 같은 마스킹된 기준 주소를 공유한다. 이것이 별도의 조회 테이블 없이 "이 포인터가 어느 bin에 속하는지" 복원하는 방식이다.
2. 헤더의 `m_Magic`을 `PAGE_MAGIC`과 비교해 검증하고(손상/외부 포인터 가드), `m_BinIndex`가 `LARGE_BIN`이거나 유효한 bin 인덱스인지 검증한다.
3. `LARGE_BIN`인 경우: 단일 연결 리스트인 `m_pAllPages`를 커서로 순회하며 일치하는 노드를 찾아 페이지 헤더를 연결 해제한 뒤, 페이지 전체를 `_aligned_free`로 해제한다.
4. 그렇지 않은 경우(소형 bin): 블록을 단순히 `m_FreeLists[bin]`에 다시 밀어 넣는다(`pBlock->m_pNext = m_FreeLists[bin]; m_FreeLists[bin] = pBlock`) — 메모리는 OS로 반환되지 않으며, 페이지는 재사용을 위해 얼로케이터가 계속 소유한다. 참고로 페이지가 bin 간에 협조적으로 소형-bin 프리 풀로 반환되는 일은 없으며, bin이 비었을 때 페이지를 해제하는 로직도 없다 — 페이지는 얼로케이터 자체가 소멸될 때까지 살아있는다.

**단계별: `AllocateLarge(size, alignment)` (대형 폴백)**
1. `alignment`는 최소 `BIN_MAX_ALIGN`(16)까지 올림 조정되며, `PAGE_SIZE`보다 작은지 `check`로 검증된다.
2. 필요한 바이트 수는 `HEADER_SIZE + alignment + size`(헤더, 최악의 경우 정렬 여유분, 페이로드)로 계산된 후 `PAGE_SIZE`의 다음 배수로 올림된다(`totalSize`).
3. `_aligned_malloc(totalSize, PAGE_SIZE)`가 페이지 수만큼의 크기로 64KB 페이지 정렬된 블록을 할당한다.
4. 앞부분에 `FPageHeader`가 기록되며 `m_BinIndex = LARGE_BIN`, `m_AllocSize = size`(사용자가 요청한 정확한 크기이며, 대형 블록은 고정 bin 크기가 없으므로 나중에 `Realloc`의 복사 크기 계산에 필요)로 설정된다. 이 페이지는 `m_pAllPages`에 연결된다.
5. 페이로드 포인터는 `pRaw + HEADER_SIZE`로 계산된 뒤 요청된 `alignment` 경계로 올림되며, 그 정렬된 주소가 반환된다. 이는 헤더와 페이로드 사이에 최대 `alignment - 1` 바이트의 패딩을 낭비한다.

**단계별: `Realloc(ptr, newSize, alignment)`**
1. `ptr == nullptr` → 전체를 `Malloc(newSize, alignment)`으로 위임한다.
2. `newSize == 0` → `ptr`을 해제하고 `nullptr`을 반환한다.
3. 그 외의 경우: `PageOf(ptr)`로 페이지 헤더를 찾고 매직 넘버를 검증한 뒤 `oldSize`를 결정한다 — `LARGE_BIN`이면 저장된 `m_AllocSize`이고, 소형 bin이면 `m_BinBlockSize[binIndex]`(원래 요청 크기가 아니라 해당 bin의 고정 슬롯 크기다 — 소형 bin은 정확한 요청 크기를 추적하지 않는다)이다.
4. 항상 `Malloc(newSize, alignment)`을 통해 완전히 새로운 블록을 할당하고(제자리 확장은 없다), `FMemory::Memcpy`로 `min(oldSize, newSize)` 바이트를 복사한 뒤 기존 블록을 해제하고 새 포인터를 반환한다. 이는 같은 bin 내에서조차 축소/확장을 제자리에서 최적화하지 않는, 복사 기반 realloc이다.

**공개 API 목록:**
| 멤버 | 설명 |
|---|---|
| `Malloc(size, alignment)` | Bin 기반 소형 할당 또는 대형 페이지 폴백 |
| `Realloc(ptr, newSize, alignment)` | 항상 복사 기반(새 할당 + memcpy + 해제) |
| `Free(ptr)` | 포인터를 마스킹하여 페이지 헤더를 찾고, 대형 블록은 해제하거나 블록을 bin 프리 리스트로 반환 |
| `NUM_BINS`, `MAX_SMALL_SIZE`, `BIN_MAX_ALIGN`, `PAGE_SIZE`, `PAGE_MAGIC`, `LARGE_BIN` | bin/페이지 구조를 나타내는 공개 상수 |
| `SizeToBin`, `GrowBin`, `AllocateLarge`, `PageOf` *(비공개)* | 내부 bin 조회, 페이지 절단, 대형 경로, 헤더 복원 |

---

### FMemory — 전역 얼로케이터 진입점

**목적:** 프로세스 전역 `GMalloc` 포인터 위에 만들어진 정적 파사드로, 다른 모든 시스템이 `IAllocator`/CRT를 직접 건드리는 대신 호출하는 엔진 전역 할당 API(및 원시 메모리 연산)를 제공한다.

**코드** (`Engine/Include/Core/Memory/FMemory.h`):
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

**단계별 런타임 동작:**
1. `InitMemory()`는 엔진 시작 시 한 번 호출되어야 한다. 이 함수는 **함수 지역 정적(static)** `FMallocBinned`를 생성하고(따라서 첫 호출 시 지연 생성되며 프로그램이 종료될 때까지 유지된다) 전역 `GMalloc`이 이를 가리키도록 설정한다. 이곳이 코드베이스에서 구체적인 얼로케이터 전략을 결정하는 유일한 지점이다 — `FMallocAnsi`로 교체하려면 오직 이 함수 하나만 바꾸면 된다.
2. `Malloc`/`Realloc`/`Free`는 얇은 정적 래퍼다: 각각 `check(GMalloc)`(얼로케이터가 초기화되었는지 하드 어설트)을 수행한 뒤 `GMalloc->Malloc/Realloc/Free`로 가상 디스패치한다. 이것이 다형성의 접합점이다 — 엔진 내 모든 호출자는 이 단일 전역 포인터를 통해 `IAllocator`의 vtable을 거치게 된다.
3. `Memcpy`/`Memset`/`Memmove`는 CRT의 `memcpy`/`memset`/`memmove`로 직접 전달된다(얼로케이터와는 무관하며, "CRT를 여기저기 직접 호출하지 않는다"는 관례에 따라 원시 메모리 연산을 한 클래스에 모아둔 것이다).
4. `Memzero(dest, count)`는 `memset(dest, 0, count)`다 — 편의를 위한 래퍼다.

**공개 API 목록:**
| 멤버 | 설명 |
|---|---|
| `GMalloc` (전역 `extern IAllocator*`) | `InitMemory()`가 설정하는 현재 활성 얼로케이터 인스턴스 |
| `InitMemory()` | 정적 `FMallocBinned`를 생성하고 `GMalloc`을 바인딩 |
| `Malloc(size, alignment=16)` | `GMalloc->Malloc`으로 디스패치 |
| `Realloc(ptr, newSize, alignment=16)` | `GMalloc->Realloc`으로 디스패치 |
| `Free(ptr)` | `GMalloc->Free`로 디스패치 |
| `Memcpy/Memset/Memmove/Memzero` | 얇은 CRT `mem*` 래퍼 |

---

### MemoryOverride — 전역 `new`/`delete` 라우팅

**목적:** 전역 `operator new`/`operator delete`(스칼라, 배열, sized-delete 형태 모두)를 오버라이드하여 엔진 전역의 *모든* `new`/`delete` 호출이 투명하게 `FMemory`/`GMalloc`을 거치고, (Debug 빌드에서는) `FMemoryTracker`를 거치도록 한다.

**코드** (`Engine/Include/Core/Memory/MemoryOverride.cpp`):
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
(배열 형태 `operator new[]`/`operator delete[]`는 스칼라 버전과 정확히 동일한 구조다.)

**단계별 런타임 동작:**
1. 이 `.cpp` 파일은 `FMemory.h`에서 `extern`으로 선언된 `IAllocator* GMalloc`의 실제 저장 공간이 정의되는 곳이기도 하다(`IAllocator* GMalloc = nullptr;`) — `FMemory::InitMemory()`가 실행되기 전까지는 null 상태로 시작한다.
2. 엔진 내 모든 `new T(...)`는 이 오버라이드된 `operator new(size_t)`로 귀결된다(RTTI/예외가 비활성화되어 있고 일반 `new`에 커스텀 placement-new가 사용되지 않기 때문이다). `_DEBUG` 빌드에서는 먼저 `FMemoryTracker::OnAlloc(size)`를 호출해 트래커의 할당 카운터를 증가시킨 뒤, `FMemory::Malloc(size)`(`FMemory`의 기본 16바이트 정렬 사용)를 호출하며, 이는 최종적으로 `GMalloc->Malloc`으로 디스패치된다.
3. `delete ptr`은 `operator delete(void*)`로 귀결되며, (Debug 빌드에서는) `FMemoryTracker::OnFree()`를 호출한 뒤 `FMemory::Free(ptr)`를 호출한다.
4. 두 개의 **sized-delete** 오버로드(`operator delete(void*, size_t)` / `operator delete[](void*, size_t)`)가 제공되는 이유는, C++14 이상 컴파일러가 sized-delete 오버로드가 존재할 경우 이를 선호할 수 있기 때문이다(전역 operator-delete 오버로드 집합의 필수 구성 요소). 두 버전 모두 unsized 버전과 동일하게 구현되어 있으며, 추가로 전달되는 `size_t` 매개변수는 무시한다. `FMallocBinned::Free`가 컴파일러가 전달하는 크기에 의존하지 않고 포인터 마스킹을 통해 필요한 크기/bin 정보를 스스로 복원하기 때문이다.
5. 이 오버라이드는 `main` 이전에 생성되는 정적/전역 객체에 대해서는 `FMemory::InitMemory()`가 실행되기 *전에* 일어나므로, 그런 이른 시점의 `new` 호출은 null인 `GMalloc`을 역참조하여 `FMemory::Malloc`의 `check(GMalloc)` 어설트에 걸리게 된다 — 즉, 정적으로 초기화되지 않는 힙 할당 전역/정적 객체 생성보다 `InitMemory()`가 먼저 실행되어야 한다.

**공개 API 목록:**
| 심볼 | 설명 |
|---|---|
| `IAllocator* GMalloc` | 전역 얼로케이터 포인터의 정의(저장 공간) |
| `operator new(size_t)` / `operator new[](size_t)` | `FMemoryTracker::OnAlloc`(Debug) + `FMemory::Malloc`으로 라우팅 |
| `operator delete(void*)` / `operator delete[](void*)` | `FMemoryTracker::OnFree`(Debug) + `FMemory::Free`로 라우팅 |
| `operator delete(void*, size_t)` / `operator delete[](void*, size_t)` | sized-delete 오버로드, 동일한 동작, `size_t`는 미사용 |

---

### FPoolAllocator — 고정 블록 크기 풀

**목적:** 하나의 연속 버퍼 위에서 동작하는 고정 크기 블록 얼로케이터로, `CLAUDE.md`에 따르면 몬스터나 파티클처럼 대량의 동종 객체를 할당하는 용도로 사용된다. 사용되지 않는 블록 내부에 직접 임베드된 인트루시브 단일 연결 프리 리스트를 사용한다.

**코드** (`Engine/Include/Core/Memory/FPoolAllocator.h`):
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

**단계별 런타임 동작:**
1. `Init(blockSize, blockCount)`: `m_BlockSize`를 최소 `sizeof(void*)`까지 올림 조정한다(프리 블록이 항상 다음 포인터를 담을 수 있도록 하기 위함). 그런 다음 `FMemory::Malloc`(16바이트 정렬)을 통해 `blockSize * blockCount` 바이트 크기의 연속 버퍼 하나를 할당한다.
2. 이후 버퍼를 블록 단위로 순회하며 *각 블록의 첫 `sizeof(void*)` 바이트*에 다음 블록의 주소를 기록하여, 블록 0부터 블록 `N-2`까지 이어지는 순방향 연결 프리 리스트를 구성한다. 마지막 블록의 링크는 `nullptr`(리스트 종료자)로 설정된다. `m_pFreeList`는 블록 0(이 리스트의 head)을 가리키도록 설정된다.
3. `Acquire()`: 프리 리스트가 비어 있지 않은지 `check`한다(풀 고갈은 우아한 실패가 아니라 하드 어설트로 처리된다 — "예외 없음" 엔진 철학과 일치한다). head 블록을 리스트에서 팝하고(블록에 임베드된 다음 포인터를 읽어 `m_pFreeList`를 전진시킨다), 해당 블록의 주소를 호출자에게 반환한다. O(1), 스캔 없음.
4. `Release(ptr)`: 현재 `m_pFreeList`를 `ptr`의 첫 `sizeof(void*)` 바이트에 기록한 뒤 `ptr`을 새로운 head로 만듦으로써, 반환된 블록을 프리 리스트의 head로 다시 밀어 넣는다. O(1). 이는 블록에 있던 기존 데이터를 파괴하므로, 호출자는 `Release`를 호출하기 전에 객체의 소멸자가 이미 실행되었는지(또는 객체의 첫 포인터 폭만큼 바이트를 덮어써도 안전한지) 반드시 확인해야 한다.
5. `Destroy()`: `FMemory::Free(m_pMemory)`를 통해 전체 백킹 버퍼를 해제하고 기록 필드를 0으로 초기화한다. 참고로 이 함수는 여전히 "획득된" 상태로 남아 있는 살아있는 객체의 소멸자를 **호출하지 않는다** — 이는 순수한 원시 메모리 풀이며, 각 블록에 저장된 것의 소유권/수명을 추적하지 않는다.

**공개 API 목록:**
| 멤버 | 설명 |
|---|---|
| `Init(blockSize, blockCount)` | 백킹 버퍼를 할당하고 인트루시브 프리 리스트를 구성 |
| `Acquire()` | 프리 블록 하나를 팝하여 반환(풀 고갈 시 어설트) |
| `Release(void* ptr)` | 블록을 프리 리스트로 다시 밀어 넣음 |
| `Destroy()` | 백킹 버퍼를 해제하고 상태를 초기화 |

---

### FStackAllocator — 프레임/선형(bump) 얼로케이터

**목적:** 사전 할당된 하나의 버퍼 위에서 동작하는 선형 "bump-pointer" 얼로케이터로, 프레임 단위 임시 할당을 위한 것이며 단일 `Reset()` 호출로 모든 할당이 한꺼번에 폐기된다(할당별 `Free` 없음).

**코드** (`Engine/Include/Core/Memory/FStackAllocator.h`):
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

**단계별 런타임 동작:**
1. `Init(capacity)`은 `FMemory::Malloc`을 통해 `capacity` 바이트(16바이트 정렬) 크기의 버퍼 하나를 할당하고 bump 커서 `m_Offset`을 `0`으로 설정한다.
2. `Alloc(size, alignment)`: 표준적인 `(offset + align - 1) & ~(align - 1)` 비트마스크 트릭을 사용하여 현재 `m_Offset`을 요청된 `alignment` 경계로 올림한다(`alignment`가 2의 거듭제곱이어야 하며, 이는 검증되지 않는다). 정렬된 오프셋에 `size`를 더한 값이 `m_Capacity`를 넘지 않는지 `check`한다(하드 어설트이며 우아한 확장은 없다 — 이는 고정 용량 선형 얼로케이터다). 그런 다음 `m_pBuffer + aligned`를 반환하고 `m_Offset`을 `aligned + size`로 전진시킨다. 할당별 해제는 존재하지 않는다 — 이 방식으로 할당된 객체는 호출을 필요로 하는 사소한/없는 소멸자를 가지고 있거나, 호출자가 별도로 소멸을 관리한다고 가정된다. 오직 원시 bump 포인터만 추적되기 때문이다.
3. `Reset()`은 단순히 `m_Offset`을 다시 `0`으로 되돌려, 마지막 리셋(또는 `Init`) 이후 할당된 모든 것을 커서 폐기만으로 즉시 "해제"한다 — 실제로 메모리가 OS에 반환되거나 0으로 초기화되지는 않으며, 다음 `Alloc` 호출들은 그저 기존 내용을 덮어쓸 뿐이다. 이것이 의도된 프레임 단위 관용구다: 프레임마다 한 번 `Reset()`, 프레임 도중 여러 번 `Alloc()`.
4. `Destroy()`는 백킹 버퍼를 해제하고 기록 필드를 0으로 초기화한다.

참고: `.cpp` 파일의 인라인 주석은 소스에서 깨진/모지바케(mojibake) 텍스트로 인코딩되어 있다(다른 코드페이지로 잘못 읽힌 UTF-8 한글 주석으로 추정된다) — 이는 컴파일된 동작에는 영향을 주지 않으며, 해당 파일의 문서 가독성에만 영향을 준다.

**공개 API 목록:**
| 멤버 | 설명 |
|---|---|
| `Init(capacity)` | 백킹 버퍼를 할당하고 커서를 0으로 초기화 |
| `Alloc(size, alignment=16)` | 다음 정렬된 오프셋에 `size` 바이트를 bump 할당(오버플로 시 어설트) |
| `Reset()` | 커서를 0으로 되돌려 마지막 리셋 이후의 모든 할당을 폐기 |
| `Destroy()` | 백킹 버퍼를 해제 |

---

### FMemoryTracker — 디버그 전용 릭/할당 추적

**목적:** 오버라이드된 `operator new`/`delete`에 훅으로 연결된 `_DEBUG` 전용 전역 할당/해제 카운터로, 종료 시점에 릭(할당/해제 카운트 불일치)을 감지한다. non-debug 빌드에서는 완전히 컴파일에서 제외된다.

**코드** (`Engine/Include/Core/Memory/FMemoryTracker.h`):
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

**단계별 런타임 동작:**
1. 클래스 전체(선언과 정의 모두)가 `#ifdef _DEBUG`로 감싸져 있다 — Release 빌드에서는 이 클래스가 아예 존재하지 않으며, `MemoryOverride.cpp`에서 이를 호출하는 코드 역시 같은 `#ifdef` 가드에 의해 컴파일에서 제외된다. 따라서 Release 빌드에는 추적 오버헤드가 전혀 없다.
2. `OnAlloc(size)`는 오버라이드된 모든 전역 `operator new`/`new[]`에서 실제 `FMemory::Malloc` 호출 전에 호출되며, `m_AllocCount`를 증가시키고 `m_TotalAllocBytes`에 `size`를 더한다.
3. `OnFree()`는 오버라이드된 모든 `operator delete`/`delete[]`(sized/unsized 포함 네 가지 오버로드 전부)에서 실제 `FMemory::Free` 호출 전에 호출되며, `m_FreeCount`를 증가시킨다. 참고로 이 함수는 해제된 크기를 알지 못한다(sized-delete의 `size_t` 매개변수는 여기서도 버려진다). 따라서 `m_TotalAllocBytes`는 오직 누적 할당 바이트만 추적할 뿐, 순/현재 바이트 수는 추적하지 않는다.
4. `ReportLeaks()`는 종료 시점에 호출되도록 의도되었다. `leaked = m_AllocCount - m_FreeCount`를 계산한다. 양수라면 릭 경고 메시지(카운트와 총 할당 바이트)를 `swprintf_s`를 통해 `wchar_t` 버퍼에 포맷한 뒤 두 번 출력한다: 한 번은 `OutputDebugStringW`(디버거의 출력 창에서 확인 가능)로, 한 번은 `wprintf`(표준 출력/콘솔에서 확인 가능)로. `leaked <= 0`이면 `wprintf`만으로 "릭 없음" 메시지를 출력한다.
5. 중요한 주의사항: 이 클래스는 오직 **카운트**만 추적할 뿐 개별 포인터/호출 지점은 추적하지 않는다 — 따라서 *어떤* 할당이 릭되었는지는 보고할 수 없고, 오직 할당/해제 카운트가 불균형한지, 그리고 프로세스 수명 동안 총 몇 바이트가 할당되었는지만 알 수 있다.

**공개 API 목록 (Debug 빌드 전용):**
| 멤버 | 설명 |
|---|---|
| `OnAlloc(size)` | `m_AllocCount`를 증가시키고 `m_TotalAllocBytes`에 더함 |
| `OnFree()` | `m_FreeCount`를 증가시킴 |
| `ReportLeaks()` | 할당/해제 카운트를 비교하여 디버거 출력 + 표준 출력으로 릭 보고서를 출력 |

---

## Templates

### TypeTraits.h — 컴파일러 인트린식 기반 타입 트레이트 라이브러리

**목적:** 직접 손으로 작성한, STL을 사용하지 않는 컴파일 타임 타입 트레이트 모음이다(정수 상수 래퍼, 동일 타입 검사, 참조/CV 제거, 포인터 판별, 컴파일러 인트린식을 통한 POD/trivial/enum/class 검사, `TEnableIf`/`TConditional` SFINAE 헬퍼, `TDecay`, 그리고 SFINAE 오버로드 해석으로 처음부터 구현한 `TIsBaseOf`).

**코드** (`Engine/Include/Core/Templates/TypeTraits.h`):
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

**단계별: 이 코드가 사용될 때 컴파일 타임에 일어나는 일**
1. `TIntegralConstant<T, Val>`이 기반이 된다: 타입 `T`의 컴파일 타임 상수를 `static constexpr` 멤버 `Value`로 감싸고, 암시적 `constexpr operator ValueType()`을 제공하여 이를 상속하는 모든 트레이트가 `if constexpr`나 `static_assert` 문맥에서 곧바로 불리언처럼 사용될 수 있게 한다. `FTrueType`/`FFalseType`은 이 불리언 특수화이며, 아래의 거의 모든 트레이트가 여기서 상속받는다.
2. `TIsSame<A, B>`는 부분 특수화를 사용한다: 기본 템플릿은 `FFalseType`을 상속하고, 특수화된 `TIsSame<T, T>`(양쪽 템플릿 인자가 정확히 동일할 때만 매칭)는 `FTrueType`을 상속한다. `A == B`일 때 컴파일러는 더 특수화된 쪽을 선택한다.
3. `TIsPointer<T>`도 마찬가지로 기본값은 `FFalseType`이며, `T*`와 `T* const`에 대한 부분 특수화가 매칭되어 `FTrueType`을 산출한다 — 이는 런타임 코드 생성 없이 순수하게 타입에 대한 패턴 매칭이다.
4. `TIsPOD`, `TIsTriviallyCopyable`, `TIsEnum`, `TIsClass`는 모두 MSVC/Clang **컴파일러 빌트인**(`__is_pod`, `__is_trivially_copyable`, `__is_enum`, `__is_class`)에 직접 위임한다 — 이런 트레이트는 이런 인트린식 없이는 이식 가능한 C++로 구현할 수 없으므로, 라이브러리는 이를 처음부터 재구현하는 대신 컴파일러에 의존한다. 이는 `CLAUDE.md`에서 언급된 `TArray`의 POD 분기 최적화(자명한(trivial) 타입에 대해 생성자/소멸자 호출을 건너뛰기 위해 `TIsPOD<T>::Value`에 대해 `if constexpr`로 분기)와 같은 곳에 사용된다.
5. `TEnableIf<Condition, T>`: `Condition`이 false일 때 기본 템플릿에는 `Type` 멤버가 **전혀** 없다. `Type::Type`이 정의되는 것은 오직 `TEnableIf<true, T>` 특수화뿐이다. 함수의 반환 타입이나 템플릿 매개변수에서 `TEnableIf<false, X>::Type`을 참조하면 SFINAE(치환 실패는 오류가 아니다)가 발동한다 — 해당 오버로드는 하드 에러 없이 조용히 오버로드 집합에서 빠지며, 이것이 선언 레벨에서 `if constexpr` 없이 조건부 오버로드 디스패치를 달성하는 방식이다.
6. `TConditional<Cond, TrueT, FalseT>`: 컴파일 타임 삼항 연산자다 — 기본 템플릿은 `FalseT`를 선택하고, `<true, ...>` 부분 특수화는 `TrueT`를 선택한다. 이는 `AndOrNot.h`의 `TAnd`/`TOr`이 내부적으로 재귀를 계속할지 short-circuit할지 선택하는 데 정확히 그대로 사용하는 메커니즘이다.
7. `TDecay<T>`는 `TRemoveReference<T>::Type`을 구한 뒤 `TRemoveCV<...>::Type`을 적용하는 식으로 구성되며, `std::decay`의 참조/CV 제거 동작을 그대로 재현한다(다만 `std::decay`의 배열-to-포인터/함수-to-포인터 감쇠는 재현하지 않는다는 점에 유의해야 한다 — 여기에는 그런 특수화가 보이지 않으며 CV/참조 제거 부분만 구현되어 있다).
8. `TIsBaseOf<Base, Derived>`는 가장 복잡한 부분이다: 컴파일러 인트린식이 아니라 전통적인 **SFINAE 오버로드 해석**으로 구현되어 있다. `TIsBaseOfHelper::Test`는 오버로드되어 있다: 하나는 `const Base*`를 정확히 받는 오버로드(`char`를 반환, 크기 1)이고, 다른 하나는 C-스타일 가변인자 캐치올 `Test(...)`(`char[2]`에 대한 참조를 반환, 크기 2)이다. `const Derived*`(즉 `static_cast<const Derived*>(nullptr)`)로 호출될 때, `Derived*`가 `Base*`로 암시적 변환 가능한 경우에만(즉 `Derived`가 `Base`를 public/명확하게 상속하거나 `Base == Derived`인 경우) 컴파일러는 `const Base*` 오버로드를 선호한다. 그렇지 않으면 가변인자 캐치올만 바인딩될 수 있다. `sizeof(Test(...))`는 그 결과 `1`(true) 또는 `2`(false)가 되며, 이는 런타임에 함수가 실제로 호출되지 않고 전적으로 컴파일 타임에 계산된다 — `Value`는 이 `sizeof` 비교에서 도출되는 `constexpr bool`이다. 이 코드가 `UObjectPrivate` 네임스페이스 안에 있다는 점은, 이것이 엔진의 RTTI 없는 `Cast<T>()`/`UClass` 시스템(`CLAUDE.md`의 Phase 7)을 지원하기 위해 만들어졌음을 시사한다. 이 시스템에서는 `dynamic_cast` 없이 컴파일 타임에 base/derived 관계를 검증해야 한다.

**공개 API 목록:**
| 트레이트 | 의미 |
|---|---|
| `TIntegralConstant<T,Val>` / `FTrueType` / `FFalseType` | 컴파일 타임 상수 래퍼 베이스 |
| `TIsSame<A,B>` | `A`와 `B`가 정확히 같은 타입일 때 `true` |
| `TRemoveReference<T>`, `TRemoveConst<T>`, `TRemoveVolatile<T>`, `TRemoveCV<T>` | `&`/`&&`, `const`, `volatile` 제거 |
| `TIsPointer<T>`, `TRemovePointer<T>` | 포인터 타입 판별/제거 |
| `TIsLValueReference<T>`, `TIsRValueReference<T>` | `&` vs `&&` 판별 |
| `TIsPOD<T>`, `TIsTriviallyCopyable<T>`, `TIsEnum<T>`, `TIsClass<T>` | 컴파일러 인트린식 기반의 trivial/enum/class 검사 |
| `TEnableIf<Cond,T=void>` | SFINAE 게이트; `Cond`가 true일 때만 `::Type`이 존재 |
| `TConditional<Cond,TrueT,FalseT>` | 컴파일 타임 삼항 타입 선택 |
| `TDecay<T>` | 참조 + CV가 제거된 타입 |
| `TIsBaseOf<Base,Derived>` | SFINAE 기반 base/derived 관계 검사 |

---

### Utility.h — MoveTemp / Forward / Swap

**목적:** `std::move`, `std::forward`, `std::swap`을 대체하는 STL 없는 구현으로, 위의 `TRemoveReference` 트레이트 위에 직접 만들어져 있다.

**코드** (`Engine/Include/Core/Templates/Utility.h`):
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

**단계별 런타임/컴파일 타임 동작:**
1. `MoveTemp(T&& Obj)`: `T`는 포워딩/유니버설 참조를 통해 추론되므로 lvalue(`T`가 `U&`로 추론)나 rvalue(`T`가 `U`로 추론) 모두에 바인딩될 수 있다. 어느 쪽이 바인딩되든 본문은 무조건 `TRemoveReference<T>::Type&&`(정규화된 `T`에 대한 rvalue 참조)로 `static_cast`한다. 이는 무조건적으로 lvalue성을 제거하며, `std::move`의 의미와 정확히 일치한다: 이 함수 자체는 아무것도 이동시키지 않으며, 단지 이후의 이동 생성자/이동 대입 연산자가 바인딩할 rvalue 타입 표현식을 생성할 뿐이다.
2. `Forward<T>(Obj)`는 `std::forward`의 lvalue/rvalue 오버로드 쌍을 그대로 반영하는 두 개의 오버로드를 가진다: 첫 번째는 `TRemoveReference<T>::Type&`(lvalue 바인딩)를 받고, 두 번째는 `TRemoveReference<T>::Type&&`(rvalue 바인딩)를 받으며, 둘 다 `T&&`로 캐스트한다. 포워딩 참조 함수 템플릿 내부에서 `Forward<T>(arg)`로 사용될 때, 호출자가 lvalue를 전달했다면 `T`는 `U&`로 추론되므로 `T&&`는 참조 붕괴(reference collapsing)에 의해 `U&`로 축소된다 — lvalue는 lvalue로 전달된다. 호출자가 rvalue를 전달했다면 `T`는 `U`로 추론되므로 `T&&`는 `U&&`가 된다 — rvalue로 전달된다. 이것이 `<utility>` 없이 재구현된 표준 완벽 전달(perfect-forwarding) 관용구다.
3. `Swap(T& A, T& B)`: 교과서적인 세 번의 이동으로 이루어진 스왑이다 — `A`를 임시 변수 `Tmp`로 이동시키고(`MoveTemp`를 통해 `T`의 이동 생성자 호출), `B`를 `A`로 이동 대입한 뒤, `Tmp`를 `B`로 이동 대입한다. 이 함수는 `T`가 이동 생성자와 이동 대입 연산자를 가지고 있음에 의존한다(사용자가 제공하지 않았거나 사용할 수 없는 경우에는 일반적인 C++ 오버로드 해석 규칙에 따라 복사 시맨틱으로 폴백한다) — `CLAUDE.md`에 언급된 `TArray`의 POD 최적화처럼 `TIsTriviallyCopyable` 기반의 memcpy 고속 경로는 여기에는 존재하지 않는다.

**공개 API 목록:**
| 함수 | 설명 |
|---|---|
| `MoveTemp(T&& Obj)` | rvalue 참조로 캐스트(`std::move` 대체) |
| `Forward<T>(Obj)` (오버로드 2개) | 완벽 전달 캐스트(`std::forward` 대체) |
| `Swap(T& A, T& B)` | 두 객체를 세 번의 이동으로 교환 |

---

### AndOrNot.h — 가변 인자 컴파일 타임 불리언 조합자

**목적:** 각각 `::Value`를 노출하는 트레이트 타입 팩에 대한 가변 템플릿 기반 컴파일 타임 논리 AND/OR/NOT이며, 재귀적인 `TConditional` short-circuit을 통해 구현된다 — 여러 개의 `TIsX<T>::Value` 검사를 `TEnableIf`용 단일 조건으로 결합하는 등, 트레이트 조합에서 흔히 쓰이는 배관 코드다.

**코드** (`Engine/Include/Core/Templates/AndOrNot.h`):
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

**단계별: `TAnd<A, B, C>::Value`가 평가될 때 일어나는 일**
1. `TAnd<A, B, C>`는 `First = A`, `Rest = {B, C}`인 가변 인자 부분 특수화와 매칭된다. 이는 `TConditional<A::Value, TAnd<B, C>, FFalseType>::Type`을 상속한다.
2. `A::Value`가 `false`이면 `TConditional`은 `FalseT = FFalseType`을 선택하고, 따라서 `TAnd<A,B,C>`는 `FFalseType`을 직접 상속한다 — 결정적으로 이 분기에서는 **`TAnd<B, C>`가 결코 인스턴스화되지 않는다**(이는 오직 `TConditional`의 인스턴스화되지 않은 템플릿 인자로만 등장하며, `TConditional`의 기본/특수화 템플릿은 `Type`으로 두 인자 중 하나만 이름을 붙일 뿐 나머지 하나를 평가를 위해 인스턴스화하지는 않는다... 참고: 두 분기 모두 템플릿 인자로 이름이 붙으므로 컴파일러는 최소한 `TAnd<Rest...>`라는 타입은 형성해야 하지만, 그 베이스 클래스 목록/본문은 실제로 상속될 때만 인스턴스화된다). 이는 `::Value`에 대한 진정한 short-circuit *평가*를 제공한다(`A::Value`가 false가 되는 순간 `TAnd<B,C>`는 오직 평가되지 않는 템플릿 인자로만 사용될 뿐이므로 재귀가 더 이상 *평가*되지 않으며, 그 자신의 `Value`를 계산할 필요도 없다).
3. `A::Value`가 `true`이면 `TConditional`은 `TrueT = TAnd<Rest...>`를 선택하고, 따라서 `TAnd<A,B,C>`는 `TAnd<B,C>`를 상속하며 재귀한다 — 이는 `false`를 만나(`FFalseType`으로 short-circuit) 종료되거나, 팩이 소진되어 `TAnd<>` 기본 케이스에 도달할 때까지(빈 팩에 대해 `FTrueType`, AND의 표준 관례상 공허하게 참) 반복된다.
4. `TOr<A,B,C...>`는 정반대의 거울상이다: `First::Value == true`이면 즉시 `FTrueType`을 상속하고(short-circuit true), 그렇지 않으면 `TOr<Rest...>`로 재귀한다. 빈 팩 기본 케이스인 `TOr<>`는 `FFalseType`이다(OR에 대해 공허하게 거짓).
5. `TNot<T>`은 훨씬 단순하다 — 재귀 없이 그저 `TIntegralConstant<bool, !T::Value>`이며, 전달된 트레이트 `T`를 직접 부정한다.
6. 이 세 가지 모두에서 결과 트레이트(`TAnd<...>`, `TOr<...>`, `TNot<T>`)는 `TIntegralConstant`가 요구되는 어디서든 그대로 사용될 수 있다(예: `TEnableIf<TAnd<TIsPOD<T>, TNot<TIsPointer<T>>>::Value, X>`의 `Condition`으로), 왜냐하면 `TIntegralConstant`의 `constexpr operator ValueType()`을 전이적으로 상속하기 때문이다.

**공개 API 목록:**
| 트레이트 | 의미 |
|---|---|
| `TAnd<Types...>` | 모든 `Types::Value`가 `true`일 때 `true`; 빈 팩 → `true` |
| `TOr<Types...>` | 어느 하나라도 `Types::Value`가 `true`이면 `true`; 빈 팩 → `false` |
| `TNot<T>` | `!T::Value` |

---

### TResult.h — 예외 없는 에러 전파를 위한 `Ok`/`Err` 결과 타입

**목적:** `std::expected`/`Result` 스타일의 값-또는-에러 래퍼로, 엔진의 예외 없는 에러 전파 전략으로 사용된다(`CLAUDE.md`의 Phase 5.5: "예외 없는 에러 전파 전략 (`TResult<T,E>` 패턴)" 참고). 기본 에러 타입은 엔진 전역 `EEngineError` 열거형이다.

**코드** (`Engine/Include/Core/Templates/TResult.h`):
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

**단계별 런타임 동작:**
1. 기본 생성자 `TResult()`는 `private`이다 — 외부에서 `TResult<T,E>`를 생성할 수 있는 유일한 방법은 두 개의 정적 팩토리 함수 `Ok(...)`와 `Fail(...)`를 통하는 것뿐이다. 이는 모든 결과가 애매한 기본 상태로 남겨지지 않고 생성 시점에 성공/실패로 명시적으로 태그되도록 강제한다.
2. `Ok(const T& Value)`는 기본 생성된 `TResult`를 만든 뒤(`m_Value`를 0/기본값으로 초기화하고, `m_Error`도 기본 초기화하며, `m_bOk = false`로 설정), `Value`를 `m_Value`로 복사하고 `m_bOk = true`로 뒤집은 다음 값으로 반환한다(컴파일러의 복사/이동 생략 또는 반환 시 암시적 이동 생성자에 의존).
3. `Ok(T&& Value)`는 이동 최적화된 오버로드다: 위와 동일하지만 `Value`를 `m_Value`로 복사하는 대신 `static_cast<T&&>(Value)`(엔진의 `MoveTemp` 헬퍼를 실제로 호출하지는 않는, `MoveTemp`와 동등한 원시 캐스트)를 사용해 이동 대입한다.
4. `Fail(E Error = E{})`는 기본 `TResult`를 만들고, `m_Error`를 주어진 에러 값(또는 기본 생성된 `E{}`, `EEngineError`의 경우 첫 번째 열거자이므로 `None = 0`이다 — 즉 인자 없이 호출한 `Fail()`은 `GetError()`가 `EEngineError::None`을 반환하는 에러 결과를 만든다는 뜻이며, 이는 의미상 다소 어색하지만 실제 코드가 정확히 그렇게 동작한다)으로 설정하고, `m_bOk`은 (private 기본 생성자에 의해 이미 설정된) `false`로 남겨둔다. 참고로 `m_Value`는 여기서 전혀 건드려지지 않으며, 기본 `T()` 생성자가 만든 값 그대로 유지된다.
5. `IsOk()`/`IsErr()`는 `m_bOk`를 읽는 단순한 불리언 접근자다.
6. `GetValue()`(const와 non-const 오버로드 모두)는 `m_Value`에 대한 참조를 반환하기 전에 `check(m_bOk)`(**하드 어설트**)를 수행한다. `Fail`로 생성된 결과에 대해 `GetValue()`를 호출하면 `assert(m_bOk)`가 발동하여(`EnginePCH.h`의 `#define check(expr) assert(expr)`에 따라) Debug 빌드에서 중단된다. `assert`가 컴파일에서 제외되는 빌드(예: `NDEBUG`)에서는 아무 보호 없이 조용히 기본 생성된 `m_Value`를 반환하게 되며, 헤더는 이 경우를 특별히 처리하지 않는다.
7. `GetError()`는 반대로 `m_Error`를 값으로 반환하기 전에 `check(!m_bOk)`를 수행한다 — `Ok` 결과에 대해 이를 호출해도 하드 어설트가 발동한다.
8. `TResult.h`에는 `.cpp` 파일이 없다 — 이는(템플릿이므로 반드시 그래야 하듯) 완전히 헤더 전용 템플릿 클래스이며, 프로젝트가 보통 클래스에 대해 명시한 "헤더 + .cpp 쌍" 관례와는 다르지만 템플릿은 항상 헤더에 전부 있어야 한다는 프로젝트의 일반 규칙과는 일치한다.

**공개 API 목록:**
| 멤버 | 설명 |
|---|---|
| `EEngineError` (`None`, `FileNotFound`, `OutOfMemory`, `InvalidArgument`, `Unknown`) | `TResult`의 두 번째 템플릿 매개변수로 사용되는 기본 에러 열거형 |
| `TResult<T, E=EEngineError>::Ok(const T&)` / `Ok(T&&)` | 성공 결과를 생성(복사 또는 이동) |
| `TResult<T, E>::Fail(E Error = E{})` | 에러 코드를 담은 실패 결과를 생성 |
| `IsOk()` / `IsErr()` | 성공/실패 상태 조회 |
| `GetValue()` (const/non-const) | 성공 값에 접근; 실패에 대해 호출 시 `check(m_bOk)` 어설트 |
| `GetError()` | 에러 코드에 접근; 성공에 대해 호출 시 `check(!m_bOk)` 어설트 |

---

**소스 인코딩에 대한 참고:** `TypeTraits.h`와 `FStackAllocator.cpp`에는 원본 소스에서 모지바케(예: `// ��� Ÿ�� ---`)로 렌더링되는 주석이 포함되어 있다 — 이는 텍스트 인코딩이 맞지 않는 상태로 저장/읽힌 한글 주석으로 보인다. 이는 컴파일된 동작에는 영향을 주지 않으며, 순전히 이 두 파일의 문서/가독성 문제일 뿐이라 여기서 추측해서 "수정"하지 않고 그대로 언급해 둔다.

---

## TArray / TArrayView

#### 목적
`TArray<T, AllocatorType>`는 STL을 사용하지 않는 언리얼 스타일 동적 배열로, 항상 힙에 할당하는 방식(`TDefaultAllocator`)과 스몰 버퍼/인라인 방식(`TInlineAllocator<N>`) 저장 정책을 모두 지원한다. `TArrayView<T>`는 임의의 연속된 `T` 버퍼(원시 포인터+크기 또는 `TArray`) 위에 얹히는 비소유(non-owning) 읽기 전용 슬라이스다.

두 타입 모두 `/home/user/MapleStory_UnrealSource/Engine/Include/Core/Containers/TArray.h`와 `TArrayView.h`에 위치한다.

#### 실제 코드 발췌

`N == 0`일 때의 EBO(Empty Base Optimization) 트릭이 적용된 얼로케이터 정책:

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
`TArray`는 `TArrayInlineStorage<T, InlineCapacity>`를 private 상속한다. `InlineCapacity == 0`일 때는 데이터 멤버가 하나도 없는 특수화된 빈 구조체가 사용되므로, EBO(Empty Base Optimization) 덕분에 `TDefaultAllocator` 기반 배열은 (사용하지도 않는) 인라인 버퍼 때문에 추가 크기를 전혀 지불하지 않는다.

trivially-copyable 분기가 포함된 성장 정책(`Add`):

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

순서를 보존하는 O(n)의 `RemoveAt`과 O(1)의 `RemoveAtSwap`:

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

`Sort`(introsort: 16개 이하는 삽입 정렬, 그 이상은 median-of-3 로무토 퀵정렬)와 `StableSort`(top-down 병합 정렬):

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
`MergeImpl`은 임시 버퍼(`Left`, `Right`) 두 개를 `FMemory::Malloc`으로 할당하고, 두 절반을 그 버퍼로 복사/이동시킨 뒤(`TIsTriviallyCopyable<T>`에 따라 `Memcpy`와 `MoveTemp` 중 분기) 원소 단위로 다시 병합하고, 마지막으로 임시 버퍼의 원소를 소멸시키고 해제한다.

`TArray`로부터의 `TArrayView` 생성:

```cpp
template<typename AllocatorType>
TArrayView(const TArray<T, AllocatorType>& InArray) noexcept
	: m_pData(InArray.GetData()), m_Size(InArray.Num())
{
}
```

#### 단계별 런타임 동작

**얼로케이터 정책.** `TArray`는 `TArrayInlineStorage<T, AllocatorType::InlineCapacity>`를 상속한다. `TDefaultAllocator`를 쓰면 `InlineCapacity == 0`이 되어 특수화된 빈 저장소 타입에는 멤버가 하나도 없고, EBO 덕분에 이 베이스 서브객체는 추가 바이트를 전혀 소비하지 않는다 — 이 배열은 전통적인 힙 전용 동적 배열과 완전히 동일하게 동작한다(`m_pData`는 `nullptr`로, `m_Capacity`는 `0`으로 시작). `TInlineAllocator<N>`을 쓰면 베이스 클래스가 `alignas(T) uint8 m_InlineBytes[sizeof(T)*N]` 버퍼를 `TArray` 객체 내부에 직접 임베드한다. `InitInlineBaseline()`은 `m_pData`가 이 버퍼를 가리키도록 설정하고 `m_Capacity = InlineCapacity`로 초기화하므로, 갓 생성된 배열은 힙을 전혀 건드리지 않고도 즉시 "최대 용량" 상태가 된다.

**스몰 버퍼 전환.** `IsInline()`은 `m_pData == StorageType::GetInline()`을 비교해서 배열이 현재 인라인 버퍼에 살고 있는지, 힙에 있는지를 판별한다. `GrowTo()`는 항상 `FMemory::Malloc`으로 완전히 새로운 힙 블록을 할당하고, 기존 원소를 `RelocateElements`로 그쪽에 재배치한 다음 `FreeRaw()`를 호출한다(이 함수는 `!IsInline()`일 때만 실제로 해제를 수행하므로, 인라인 버퍼 자체가 `FMemory::Free`로 해제되는 일은 결코 없다) — 이것이 인라인 배열이 `N`개 원소를 초과했을 때 힙으로 "넘쳐 나가는(spill)" 방식이다. `Shrink()`는 축소 후 `m_Size <= InlineCapacity`가 되면 데이터를 다시 인라인 버퍼로 옮길 수 있다(`MoveToInline()`). 이동 생성/대입에 쓰이는 `MoveFrom`은 `Other`가 힙에 있을 때는 힙 포인터를 그대로 가로채고(steal), `Other`가 인라인 상태일 때는(스택 버퍼의 주소는 "가로챌" 수 없으므로) 원소 단위로 재배치한다.

**성장(2배씩).** `EnsureCapacity(Required)`는 `m_Capacity >= Required`이면 아무 것도 하지 않는다. 그렇지 않으면 새 용량을 4에서 시작(빈 배열일 경우)하거나 현재 용량을 2배로 늘리며(`m_Capacity * 2`), `>= Required`가 될 때까지 루프를 돌며 계속 2배씩 늘린 뒤 `GrowTo`를 호출한다. `GrowTo`는 "새로 할당 → 기존 원소 재배치 → 기존 버퍼 해제" 순서로 이루어지는 단순한 절차다.

**trivial/non-trivial 분기(`if constexpr TIsTriviallyCopyable<T>`).** 거의 모든 변경 연산(`Add`, `RemoveAt`, `RemoveAtSwap`, `RemoveAll`, `RelocateElements`, `CopyElementsFrom`, `DestroyElements`, `MergeImpl`)은 컴파일 타임에 분기한다: trivially-copyable 타입은 `FMemory::Memcpy`/`Memmove`(생성자·소멸자 호출 없는 원시 바이트 복사이며, 컴파일 타임에 이미 분기가 확정되므로 런타임 분기 비용이 없다)를 사용하고, 비trivial 타입은 placement-new(`new (ptr) T(...)`)와 명시적 `~T()` 호출, `MoveTemp`를 거친다. 즉 POD 타입(예: `int32`, `FVector2D`)은 memcpy 수준의 속도로 배열 연산을 수행하는 반면, 사용자 정의 생성자·소멸자를 가진 클래스 타입은 올바른 생성·소멸 의미론을 보장받는다.

**RemoveAt vs RemoveAtSwap.** `RemoveAt`은 `Index` 위치의 원소를 소멸시킨 뒤, 뒤따르는 모든 원소를 한 칸씩 앞으로 당긴다(trivial 타입은 `Memmove`, 그 외에는 원소별 이동+소멸 루프). 상대적 순서를 보존하며 비용은 O(n - Index)다. `RemoveAtSwap`은 `Index` 위치의 원소를 소멸시킨 뒤(그 원소가 마지막 원소가 아니라면) 마지막 살아있는 원소로 덮어쓰고(`Memcpy`/이동) `m_Size`를 하나 줄인다 — O(1)이지만 순서는 깨진다.

**Sort / StableSort.** `Sort()`는 불안정(unstable) 정렬인 introsort 방식으로, 16개 이하 구간에서는 단순 삽입 정렬로 처리하고, 더 큰 구간에서는 median-of-three(`Low`, `Mid`, `High`) 방식으로 피벗을 선택해 `High` 위치로 옮긴 뒤 로무토(Lomuto) 파티션을 수행하고 양쪽을 재귀 호출한다 — 힙정렬 폴백을 갖춘 진짜 깊이 제한형 introsort는 아니며, 단순히 삽입 정렬 + 퀵정렬 조합이다. `StableSort()`는 교과서적인 top-down 병합 정렬이다: `[Low, Mid]`와 `[Mid+1, High]`를 각각 재귀적으로 정렬한 뒤, `MergeImpl`이 `FMemory::Malloc`으로 스크래치 버퍼 두 개를 할당하고 양쪽 절반을 그리로 복사한 다음, 동률일 때 왼쪽 런(run)을 우선시하도록(`!InPred(Right[j], Left[i])` 비교, 안정성 보장) 다시 병합하고 스크래치 버퍼를 해제한다.

**TArrayView.** 완전히 비소유(non-owning) 타입이다: `const T* m_pData`와 `int32 m_Size`만 저장하며 할당·해제 로직이 전혀 없다. 임의의 `TArray<T, AllocatorType>`(얼로케이터에 대해 템플릿화되어 있어 기본/인라인 배열 모두에서 동작)로부터, 혹은 원시 `(data, size)` 쌍으로부터 암묵적으로 생성될 수 있으며, 경계 검사(`check`)를 동반하는 서브뷰 생성을 위한 `Slice()`를 지원한다.

#### 공개 API 목록

| 타입 | 멤버 | 동작 |
|---|---|---|
| TArray | `Add(const T&)/Add(T&&)` | 추가, 필요 시 성장 |
| TArray | `Emplace(Args&&...)` | 끝에 제자리 생성 |
| TArray | `RemoveAt(Index)` | O(n), 순서 보존 |
| TArray | `RemoveAtSwap(Index)` | O(1), 마지막 원소와 스왑 |
| TArray | `Remove(const T&)` / `RemoveAll(const T&)` | 첫 번째 일치 / 모든 일치 원소 제거 |
| TArray | `Find`/`Contains` | 선형 탐색 |
| TArray | `Sort()`/`Sort(Pred)` | 불안정 introsort 계열 |
| TArray | `StableSort()`/`StableSort(Pred)` | 병합 정렬 |
| TArray | `Reserve`/`Shrink`/`Reset`/`Empty` | 용량 관리 |
| TArray | `operator[]`, `Last`, `GetData` | 원소 접근 |
| TArray | `Num`, `Max`, `IsEmpty`, `IsValidIndex` | 상태 조회 |
| TArray | `begin`/`end` | 범위 기반 for |
| TArrayView | `Slice`, `operator[]`, `GetData` | 슬라이싱/접근 |
| TArrayView | `Find`, `Contains`, `Num`, `IsEmpty`, `IsValidIndex` | 조회 |
| TArrayView | `begin`/`end` | 범위 기반 for |

---

## TSparseArray

#### 목적
`TSparseArray<T>`는 프리 리스트(free-list) 기반의, 인덱스가 안정적으로 유지되는 구멍 뚫린(holey) 배열로 `TSet`과 `TMap`을 모두 뒷받침한다 — 각 원소는 생존 기간 동안 고정된 인덱스를 유지하며, 해제된 슬롯은 압축(compaction)되는 대신 침습적(intrusive) 단일 연결 프리 리스트를 통해 재활용된다.

파일: `/home/user/MapleStory_UnrealSource/Engine/Include/Core/Containers/TSparseArray.h`

#### 실제 코드 발췌

슬롯 레이아웃:

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

`Emplace`(프리 리스트 재사용 또는 추가)와 `RemoveAt`(프리 리스트로 반환):

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

`Grow`(더 큰 버퍼로의 재배치):

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

`CopyFrom`(인덱스를 보존하는 구조적 복사):

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

구멍을 건너뛰는 이터레이터:

```cpp
void SkipToAllocated()
{
    while (m_Index < m_pArray->m_HighWater && !m_pArray->m_pSlots[m_Index].m_bAllocated)
    {
        m_Index++;
    }
}
```

#### 단계별 런타임 동작

**FSlot 레이아웃.** 각 `FSlot`은 `T` 하나를 위한 원시 정렬 저장 공간(`alignas(T) uint8 m_Storage[sizeof(T)]`)과 `int32 m_NextFree`, `bool m_bAllocated`를 함께 갖는다. `T`는 슬롯이 할당될 때(`Emplace`)까지는 결코 생성되지 않으며, `RemoveAt`/`Reset`/`Empty` 시점에 명시적으로 소멸된다(`~T()`) — 원시 바이트 버퍼 구조 덕분에 할당되지 않은 슬롯은 살아있는 `T`의 생명주기를 전혀 비용으로 지불하지 않는다.

**프리 리스트 기반 Emplace.** `Emplace`는 먼저 `m_FirstFree`를 확인한다. `INDEX_NONE`이 아니면 이전에 제거된 슬롯이 존재한다는 뜻이므로, 프리 리스트의 헤드를 꺼내고(`Index = m_FirstFree; m_FirstFree = m_pSlots[Index].m_NextFree`) 그 슬롯의 저장 공간을 재사용한다 — 할당이나 재배치가 전혀 필요 없고, 다른 곳의 기존 인덱스들도 전혀 영향을 받지 않는다. 프리 리스트가 비어 있으면 뒤쪽에 추가하는 방식으로 대체된다: `m_HighWater == m_Capacity`이면 버퍼를 성장(2배씩, 혹은 4부터 시작)시킨 뒤 `Index = m_HighWater++`를 취한다. 두 경우 모두 새 `T`를 `m_pSlots[Index].m_Storage`에 placement-new로 생성하고, `m_bAllocated = true`로 표시하며, `m_NumAllocated`를 증가시킨다.

**RemoveAt.** 살아있는 객체를 제자리에서 소멸시키고(`GetPtr(Index)->~T()`), `m_bAllocated = false`로 뒤집은 뒤, 이제 비워진 슬롯을 프리 리스트의 헤드로 밀어넣는다(`m_pSlots[Index].m_NextFree = m_FirstFree; m_FirstFree = Index`). 이 연산은 O(1)이며 `Index` 위치에 "구멍"을 남긴다 — 이 인덱스는 이후 `Emplace`가 프리 리스트에서 그것을 다시 꺼내기 전까지는 그 무엇에도 재사용되지 않는다. `m_HighWater`(지금까지 한 번이라도 사용된 슬롯의 최고 수위)는 제거 연산으로 인해 변경되지 않는다.

**Grow 재배치.** `Grow(NewCapacity)`는 `FMemory::Malloc`으로 완전히 새로운 `FSlot` 배열을 할당한 뒤, `[0, m_HighWater)` 범위 전체를 순회하며 (할당 여부와 무관하게) 모든 슬롯의 `m_bAllocated`/`m_NextFree` 메타데이터를 복사하고(이는 살아있는 원소뿐 아니라 `[0, m_HighWater)` 범위 전체에 걸친 프리 리스트 연결 관계를 보존하기 위함이다) 살아있는 `T`들을 재배치한다(trivially-copyable 타입은 `Memcpy`, 그 외에는 이동 생성 + 소멸). 이후 기존 버퍼는 해제되고 포인터/`m_Capacity`가 새 값으로 교체된다. 전체 `HighWater` 범위에 대해 할당/미할당 장부가 그대로 복사되므로, 성장 이후에도 프리 리스트는 유효하게 유지되고 살아있는 모든 원소는 원래의 인덱스를 그대로 유지한다.

**CopyFrom(구조적 복사).** 복사 생성자와 복사 대입 연산자에서 사용된다: `Other.m_HighWater` 용량만큼 `Grow`한 뒤, `[0, Other.m_HighWater)` 범위의 모든 슬롯 인덱스에 대해 `m_bAllocated`/`m_NextFree` 메타데이터를 그대로 복사하고, 할당된 슬롯이라면 `Other`로부터 `T`를 복사 생성한다. 마지막으로 `m_HighWater`, `m_NumAllocated`, `m_FirstFree`를 직접 복사한다. `TMap`/`TSet`의 헤더 주석에서 "sparse 인덱스는 구조적 복사에 의해 보존된다"고 언급하는 이유가 바로 이것이다 — 복사본의 모든 원소는 `Other`에서 가졌던 것과 정확히 동일한 인덱스에 위치하게 되며, 이 덕분에 `TSet`/`TMap`은 `m_Elements`를 복사한 뒤 버킷 배열을 재해싱하지 않고 단순히 `Memcpy`할 수 있다.

**이터레이션.** `FIterator`/`FConstIterator`는 `(TSparseArray*, int32 m_Index)` 쌍을 감싼다. 생성 시점과 매 `operator++` 이후마다 `SkipToAllocated()`가 `m_Index`를 `m_bAllocated`가 false인 슬롯들 너머로 전진시키므로, 역참조(`operator*`, 내부적으로 `(*m_pArray)[m_Index]`를 호출)는 항상 살아있는 원소에 도달한다. `begin()`은 인덱스 0에서 시작하며(앞쪽의 구멍은 건너뜀), `end()`는 `FIterator(this, m_HighWater)`다. `GetIndex()`는 현재의 sparse 인덱스를 노출하는데, 이는 `TSet::Rehash`/`TMap::Rehash`가 원소 저장소를 건드리지 않고 해시 버킷 체인을 다시 연결할 때 정확히 사용하는 값이다.

#### 공개 API 목록

| 멤버 | 동작 |
|---|---|
| `Emplace(Args&&...)` / `Add(const T&)` / `Add(T&&)` | 삽입, 프리 슬롯 재사용 또는 뒤에 추가 |
| `RemoveAt(Index)` | O(1) 소멸 후 프리 리스트로 반환 |
| `IsAllocated(Index)` | 경계 + `m_bAllocated` 검사 |
| `operator[](Index)` | 원소 접근(검사 포함) |
| `Num()` | `m_NumAllocated` |
| `GetMaxIndex()` | `m_HighWater`(`IsAllocated`와 함께 `[0, GetMaxIndex())` 순회) |
| `IsEmpty()` | `m_NumAllocated == 0` |
| `Reset()` | 모든 원소 소멸, 버퍼는 유지 |
| `Empty()` | 모든 원소 소멸 + 버퍼 해제 |
| `begin()/end()` (`FIterator`/`FConstIterator`) | 구멍을 건너뛰는 이터레이션, `GetIndex()` |

---

## TSet / TMap / TMultiMap

#### 목적
`TSet<KeyType>`과 `TMap<KeyType, ValueType>`은 원소 저장을 위한 `TSparseArray` 위에 2의 거듭제곱 크기 버킷 배열(intrusive chain head 방식)을 얹은 분리 연결(separate chaining) 해시 컨테이너다. `TMultiMap<KeyType, ValueType>`은 `TMap<KeyType, TArray<ValueType>>` 위에 하나의 키에 여러 값을 매핑하는 API를 한 겹 씌운 구조다.

파일: `/home/user/MapleStory_UnrealSource/Engine/Include/Core/Containers/TSet.h`, `TMap.h`, `TMultiMap.h`.

#### 실제 코드 발췌

intrusive 체인 링크를 가진 `FSetElement` / `FMapElement`:

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

마스크를 이용한 버킷 인덱싱 (버킷 개수가 2의 거듭제곱임을 이용):

```cpp
static const int32 INITIAL_BUCKETS = 16;

int32 BucketIndex(uint32 Hash) const
{
    return (int32)(Hash & (uint32)(m_NumBuckets - 1));
}
```

`ConditionalRehash`와 `Rehash` (원소를 이동시키지 않고 재연결만 수행):

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

`TSet::Add` (버킷 헤드에 체이닝하며 삽입):

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

`TMap::Add` / `FindOrAdd` (제자리 갱신 vs 신규 삽입):

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

Remove (체인에서 분리한 뒤 `TSparseArray` 슬롯을 해제):

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

`TSparseArray`의 반복자에 위임하는 이터레이터:

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

`TMap<K, TArray<V>>`를 감싸는 `TMultiMap`:

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

#### 단계별 런타임 동작

**원소 레이아웃과 체이닝.** `TSet`은 `FSetElement { KeyType Key; int32 m_HashNext; }`를 저장하고, `TMap`은 `FMapElement { KeyType Key; ValueType Value; int32 m_HashNext; }`를 저장한다. 둘 다 `TSparseArray<FSetElement>`/`TSparseArray<FMapElement>`(`m_Elements`) 안에 보관되므로, 각 원소는 살아있는 동안 안정적인 sparse 인덱스를 유지하며, `m_HashNext`는 같은 버킷으로 해시되는 모든 원소를 엮는 intrusive 단일 연결 리스트 포인터(sparse 인덱스 기준, `INDEX_NONE`으로 종료)다 — 이는 open addressing이 아니라 전형적인 분리 연결(separate chaining) 방식이다.

**버킷 배열.** `m_pBuckets`는 `m_NumBuckets`개의 체인 헤드 인덱스(`INDEX_NONE` = 빈 버킷)를 담는 평평한 `int32*` 배열이며, `m_NumBuckets`는 항상 2의 거듭제곱으로 유지되어 `BucketIndex(Hash)`가 모듈로 연산 대신 `Hash & (m_NumBuckets - 1)`(마스크 연산)로 계산될 수 있다. `AllocateBuckets`는 배열을 malloc하고 모든 슬롯을 `INDEX_NONE`으로 초기화한다. `TSet`과 `TMap` 모두 `m_pBuckets == nullptr, m_NumBuckets == 0` 상태로 시작하며, 첫 삽입 시 `ConditionalRehash`를 통해 지연 방식으로 `INITIAL_BUCKETS == 16`개의 버킷을 할당한다.

**Add / Find.** 두 컨테이너 모두 `BucketIndex(GetTypeHash(Key))`를 계산해 버킷 헤드 슬롯에 대한 참조를 얻은 뒤, `m_HashNext` 체인을 따라가며 `==`로 키를 비교한다. `TSet::Add`의 경우 일치하는 키가 발견되면 `false`를 반환하고(중복 삽입 없음), 그렇지 않으면 새로운 `FSetElement(Key, Bucket)`이 `m_Elements`에 emplace된다 — 이때 새 원소의 `m_HashNext`는 `Bucket`이 새 원소의 인덱스로 재할당되기 *이전의* 버킷 헤드 값으로 초기화되므로, 삽입은 체인 맨 앞에 O(1)로 push하는 형태가 된다. `TMap::Add`도 동일한 방식으로 체인을 순회하지만, 일치 시 실패 처리하는 대신 `m_Elements[i].Value`를 제자리에서 덮어쓴다(갱신 시맨틱). `FindOrAdd`/`operator[]`도 같은 방식으로 순회하며, 기존 값이 있으면 그 참조를 반환하고 없으면 기본 생성된 값을 삽입한다(`FMapElement(Key, Bucket)` — 값 없는 생성자가 `Value`를 기본 생성함). `Find`/`FindIndex`/`Contains`는 동일한 형태의 읽기 전용 순회이며, 미스 시 `nullptr`/`INDEX_NONE`을 반환한다.

**Remove.** 단순 인덱스로 순회하는 대신, `Remove`는 버킷 헤드에서 시작하는 포인터의 포인터(`int32* pLink`)를 들고 체인을 순회한다. 일치하는 키를 찾으면 `*pLink = m_Elements[Index].m_HashNext`를 써서 체인에서 해당 원소를 잘라낸다(일치한 대상이 버킷 헤드든 중간/끝 링크든 균일하게 동작하는데, 이는 `pLink`가 그 이전 단계에서 `&m_pBuckets[...]`이거나 앞선 링크의 `&m_Elements[Index].m_HashNext`였기 때문이다). 그런 다음 `m_Elements.RemoveAt(Index)`를 호출해 해당 sparse 슬롯을 `TSparseArray`의 free list로 반환한다.

**ConditionalRehash / Rehash.** 모든 `Add`/`FindOrAdd`는 먼저 `ConditionalRehash(m_Elements.Num() + 1)`를 호출한다. 버킷이 아직 할당되지 않았다면 초기 16개를 할당하고 리턴한다(빈 컨테이너에는 rehash가 필요 없음). 그렇지 않고 예상 원소 개수가 `m_NumBuckets`를 초과할 경우(즉 load factor > 1.0), `NewNumBuckets`가 `NumElements` 이상이 될 때까지 두 배씩 늘린 뒤 `Rehash(NewNumBuckets)`를 호출한다. 중요한 점은, `Rehash`가 `m_Elements` 안의 원소를 **이동하거나 재구성하지 않는다는** 것이다 — (훨씬 작은) `int32` 버킷 배열만 해제 후 재할당하고, `m_Elements.begin()..end()`를 순회하며(구멍을 이미 건너뛰는 `TSparseArray` 이터레이터) 각 원소에 대해 `BucketIndex(GetTypeHash(It->Key))`를 다시 계산해 `It.GetIndex()`를 새 버킷의 체인 헤드에 밀어넣는 방식으로 모든 기존 원소를 재연결한다. `TSparseArray` 인덱스가 안정적이기 때문에 rehash 도중 어떤 `T` 객체도 복사/이동/소멸되지 않으며, 오직 작은 버킷 헤드/`m_HashNext` 정수 값만 바뀐다.

**이터레이터는 TSparseArray의 이터레이터에 위임한다.** `TSet::FIterator`/`FConstIterator`는 `TSparseArray<FSetElement>::FIterator m_It`를 담고 있는 얇은 래퍼로, `operator++`는 `++m_It`에, `operator!=`는 `m_It != Other.m_It`에 위임하며, `operator*`는 `(*m_It).Key`를 반환한다(즉 `FSetElement`를 벗겨내어 `KeyType&`만 노출한다). `TMap`은 한 걸음 더 나아가 아예 래핑조차 하지 않는다 — `FIterator = typename TSparseArray<FMapElement>::FIterator`를 직접 별칭으로 사용하며, `begin()/end()`는 단순히 `m_Elements.begin()/end()`를 반환한다. 따라서 `TMap` 이터레이터를 역참조하면 공개 멤버 `.Key`/`.Value`를 가진 `FMapElement&`가 직접 나온다(`TMultiMap::Num()`에서 `for (const auto& Bucket : m_Map) { Total += Bucket.Value.Num(); }`와 같이 사용되는 것을 볼 수 있다). 두 경우 모두, 구멍 건너뛰기(할당된 슬롯만 방문하는 동작)는 전적으로 `TSparseArray::FIterator::SkipToAllocated()`로부터 상속받는다.

**복사 시맨틱.** `TSet`과 `TMap` 모두 복사 생성자/복사 대입에서 `m_Elements`를 `TSparseArray`의 구조적 `CopyFrom`(sparse 인덱스를 보존함)을 통해 복사한 뒤, 버킷 배열은 재해싱하지 않고 그냥 `Memcpy`한다(`FMemory::Memcpy(m_pBuckets, Other.m_pBuckets, sizeof(int32) * m_NumBuckets)`) — 이는 구조적 복사가 원본과 복사본에서 모든 원소가 동일한 인덱스를 유지함을 보장하기 때문에 정확히 성립하며, 그 결과 기존 버킷 체인 인덱스가 그대로 유효하게 남는다.

**TMultiMap.** 이 클래스는 `TMap<KeyType, TArray<ValueType>> m_Map`이라는 단일 private 멤버만 가지고 있으며 자체적인 해싱/버킷 로직은 전혀 없다 — `Add`는 `m_Map.FindOrAdd(Key).Add(Value)`를 호출한다(값 배열을 가져오거나 생성한 뒤 추가); `AddUnique`는 추가하기 전에 `Values.Contains(Value)`를 추가로 검사한다; `MultiFind`/`Contains`는 `m_Map.Find`에 위임한다; `RemoveSingle`은 배열을 찾아 `TArray::Remove`로 일치하는 값 하나를 제거하고, 만약 배열이 비게 되면 `m_Map.Remove(Key)`를 호출해 키 자체도 제거한다; `RemoveAll`은 그냥 `m_Map.Remove(Key)`를 호출한다. `NumKeys()`는 `m_Map.Num()`이며; `Num()`(전체 값 개수)은 모든 버킷을 순회하며 `Bucket.Value.Num()`을 합산한다.

#### 공개 API 목록

| 컨테이너 | 멤버 | 동작 |
|---|---|---|
| TSet | `Add(const K&)/Add(K&&)` | 없으면 삽입, bool 반환 |
| TSet | `Contains(const K&)` | 존재 여부 검사 |
| TSet | `Remove(const K&)` | 체인에서 분리 + 슬롯 해제 |
| TSet | `Reset()`/`Empty()` | 원소 비우기 (버킷은 유지/해제) |
| TSet | `Num()`/`IsEmpty()` | `m_Elements`에 위임 |
| TSet | `begin()/end()` | `KeyType&`를 산출하는 `FIterator`/`FConstIterator` |
| TMap | `Add(K,V)` | 삽입 또는 값 덮어쓰기 |
| TMap | `FindOrAdd(const K&)` / `operator[]` | 있으면 참조 반환, 없으면 기본값 삽입 후 참조 반환 |
| TMap | `Find(const K&)` | `ValueType*` 또는 `nullptr` 반환 |
| TMap | `FindRef(const K&)` | 참조 반환, 존재 여부를 `check()`로 검증 |
| TMap | `Contains(const K&)` | 존재 여부 검사 |
| TMap | `Remove(const K&)` | 체인에서 분리 + 슬롯 해제 |
| TMap | `Reset()`/`Empty()` | 비우기 |
| TMap | `Num()`/`IsEmpty()` | `m_Elements`에 위임 |
| TMap | `begin()/end()` | `TSparseArray<FMapElement>::FIterator`를 직접 사용, `.Key`/`.Value` 산출 |
| TMultiMap | `Add(K,V)` | 해당 키 아래에 값 추가 |
| TMultiMap | `AddUnique(K,V)` | 쌍이 없을 때만 추가 |
| TMultiMap | `MultiFind(const K&)` | `const TArray<V>*` 반환 |
| TMultiMap | `Contains(K)` / `Contains(K,V)` | 키만 검사 또는 키+값 검사 |
| TMultiMap | `RemoveSingle(K,V)` | 값 하나 제거, 배열이 비면 키도 제거 |
| TMultiMap | `RemoveAll(K)` | 키 전체 제거 |
| TMultiMap | `Reset()`/`Empty()` | `m_Map`에 위임 |
| TMultiMap | `NumKeys()`/`Num()`/`IsEmpty()` | 키 개수 / 전체 값 개수 / 비어있는지 여부 |

---

## HashFunctions

#### 목적
`HashFunctions.h` (`/home/user/MapleStory_UnrealSource/Engine/Include/Core/Containers/HashFunctions.h`)는 `TSet`/`TMap`이 `BucketIndex(GetTypeHash(Key))`를 통해 호출하는 자유 함수 해싱 훅인 `GetTypeHash`를 정의하며, 범용 바이트 스캔 폴백과 흔히 쓰이는 스칼라 타입들에 대한 명시적 특수화를 함께 제공한다.

#### 실제 코드 발췌

범용 폴백 (바이트 단위 재해석 + 바이트별 Murmur 계열 파이널라이저):

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

`int32`/`uint32` 특수화:

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

`int64`/`uint64` 특수화 (MurmurHash3 64비트 파이널라이저, 32비트로 폴딩):

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
(`uint64`도 동일한 특수화를 가진다.)

`float` 특수화 (해싱 전에 `-0.0f`를 `0.0f`로 정규화):

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

`bool` 특수화:

```cpp
template<>
inline uint32 GetTypeHash(const bool& Value)
{
    return Value ? 1u : 0u;
}
```

포인터 오버로드 (특수화가 아니라 `T*`를 받는 별도의 템플릿 오버로드):

```cpp
template<typename T>
inline uint32 GetTypeHash(T* Ptr)
{
    uint64 Addr = (uint64)(uintptr_t)Ptr;
    return GetTypeHash(Addr);
}
```

#### 단계별 런타임 동작

**범용 폴백.** 명시적 특수화가 없는 임의의 타입 `T`에 대해, `GetTypeHash`는 `&Value`를 `const uint8*`로 재해석한 뒤 각 바이트를 바이트별 Murmur 스타일 믹스(`Hash ^= byte << ((i&3)*8); Hash ^= Hash>>16; Hash *= 0x45d9f3bU; Hash ^= Hash>>16`)로 32비트 누적 해시에 접어넣는다. 이 덕분에 `GetTypeHash`는 값 또는 참조로 전달되는 단순 해싱 가능한(trivially-hashable) 임의의 POD 구조체에 대해 별도 작업 없이 바로 동작하지만, 대신 객체 표현에 대한 순수 비트 단위 해시라는 대가를 치른다(패딩 바이트까지 포함되며, 비트 패턴이 동일한 두 객체는 의미론적 동등성과 무관하게 동일한 해시로 취급된다).

**int32 / uint32.** 단일 패스 정수 파이널라이저다: XOR-우측시프트-16, 홀수 32비트 상수 `0x45d9f3bU` 곱하기, 다시 XOR-우측시프트-16 — 저렴하면서도 잘 섞이는 avalanche 파이널라이저다(이 `0x45d9f3bU` 상수는 범용 폴백의 바이트 믹싱 곱셈 인자와 float 해시에서도 동일하게 재사용된다).

**int64 / uint64.** 전형적인 MurmurHash3 64비트 파이널라이저(`fmix64`)를 사용한다: `H ^= H >> 33; H *= <64비트 홀수 상수>;`를 세 라운드 반복하며 상수로 `0xff51afd7ed558ccdULL`와 `0xc4ceb9fe1a85ec53ULL`을 사용하고, 그 뒤 상위/하위 절반을 XOR(`H ^ (H >> 32)`)해서 64비트 결과를 32비트로 접는다.

**float.** 먼저 `-0.0f`와 `0.0f`가 동일한 해시를 갖도록 정규화한다(`(Value == 0.f) ? 0.f : Value` — 부동소수점에서 `-0.0f == 0.0f`는 참으로 평가되기 때문). 그런 다음 `memcpy`로 비트를 `uint32`에 재해석해 넣고(strict-aliasing UB를 피하기 위함), `int32`/`uint32`와 동일한 단일 패스 정수 파이널라이저를 실행한다.

**bool.** 단순하다: `true`면 `1u`, `false`면 `0u` — 가능한 해시값이 두 가지뿐이므로 믹싱이 필요 없다.

**포인터 오버로드.** `T*`(임의의 포인터 타입)에 대해 템플릿화되어 있으며, 포인터를 `uint64` 주소(`(uint64)(uintptr_t)Ptr`)로 캐스팅한 뒤 실제 믹싱을 위해 `int64`/`uint64` `GetTypeHash` 특수화로 위임한다. 이 덕분에 `TSet`/`TMap`의 포인터 키는 단순한 raw-address-modulo-buckets 방식이 아니라 완전한 MurmurHash3 파이널라이저 처리를 받는다.

#### 공개 API 목록

| 함수 | 대상 특수화 | 알고리즘 |
|---|---|---|
| `GetTypeHash(const T&)` | 범용/임의 타입 | 원시 바이트에 대한 바이트별 Murmur 스타일 믹스 |
| `GetTypeHash(const int32&)` | `int32` | 단일 패스 xorshift/곱셈 파이널라이저 |
| `GetTypeHash(const uint32&)` | `uint32` | 단일 패스 xorshift/곱셈 파이널라이저 |
| `GetTypeHash(const int64&)` | `int64` | MurmurHash3 64비트 `fmix64`, 32비트로 폴딩 |
| `GetTypeHash(const uint64&)` | `uint64` | MurmurHash3 64비트 `fmix64`, 32비트로 폴딩 |
| `GetTypeHash(const float&)` | `float` | `-0.0`/`0.0` 정규화 후 비트 캐스트 + 파이널라이저 |
| `GetTypeHash(const bool&)` | `bool` | `1`/`0` 상수 |
| `GetTypeHash(T*)` | 임의의 포인터 타입 | `uint64`로 주소 캐스팅 후 int64 해시로 위임 |

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

## FString

**목적.** `FString`은 엔진의 동적으로 힙에 소유권을 두는 와이드 문자열 타입이다(`Engine/Include/Core/String/FString.h`, `FString.cpp`). 언리얼의 `FString`에 대응하는 타입으로, STL 컨테이너가 아니라 `wchar_t`를 기반으로 하며, `FMemory::Malloc`/`FMemory::Free`를 통해서만 할당된다.

**버퍼 레이아웃.** 전체 상태는 멤버 세 개로 구성된다.

```cpp
// FString.h
wchar_t* m_pData;
int32 m_Length;
int32 m_Capacity;
```

`m_pData`는 빈 문자열/기본 문자열일 때 `nullptr`이며(`FString()`은 아무 할당도 하지 않는다), `m_Length`는 널 종료 문자를 제외한 문자 개수, `m_Capacity`는 실제로 할당된 `wchar_t` 슬롯 개수다(항상 `m_Length + 1` 이상이므로 종료 문자를 위한 공간이 언제나 확보되어 있다). `GetData()`는 버퍼가 널인 경우를 보정하기 위해 `nullptr` 대신 `L""`을 반환한다.

```cpp
const wchar_t* GetData() const
{
    return m_pData ? m_pData : L"";
}
```

증가(growth)는 `FString.cpp`의 private 함수 `Grow(int32 NewCapacity)`가 처리한다. 이 함수는 새 버퍼를 malloc하고, 기존 버퍼가 있다면 종료 문자를 포함한 `m_Length + 1`개의 와이드 문자를 복사하며(기존 버퍼가 없었다면 단독으로 `L'\0'` 하나만 기록한다), 그다음 기존 버퍼를 해제한다. 호출부(`operator+=`)는 새 용량을 `max(m_Capacity * 2, NewLength + 1)`로 계산한다 — 정확히 필요한 만큼을 하한선으로 두는 전형적인 2배 증가 전략이다.

**생성.**
- `FString()` — 제로 초기화, 할당 없음.
- `FString(const wchar_t* Str)` — 정확히 `wcslen(Str)+1` 슬롯을 할당하고 소스를 종료 문자까지 포함해 `Memcpy`한다. `check(m_pData != nullptr)`로 보호된다. `nullptr`이거나 빈 입력이 들어오면 문자열은 기본(미할당) 상태로 남는다.
- `FString(const FString& Other)` — `*this = Other`(복사 대입)로 위임한다. 즉 `Other.m_Length + 1` 크기로 새로 할당하는 진짜 깊은 복사다.
- `FString(FString&& Other) noexcept` — `Other`의 포인터/길이/용량을 그대로 훔쳐오고 `Other`를 빈 상태로 리셋한다. 할당도, 복사도 일어나지 않는다.
- 소멸자는 `m_pData`가 널이 아니면 해제한다.

**대입.** `operator=(const FString&)`는 자기 대입 여부를 확인한 뒤 기존 버퍼를 해제하고, 다시 할당해 복사한다(복사 생성자와 동일한 방식). `operator=(FString&&) noexcept`는 자기 대입 여부를 확인한 뒤 현재 버퍼를 해제하고, 소스의 멤버를 훔쳐온 다음 소스를 리셋한다. `operator=(const wchar_t*)`는 임시 `FString(Str)`을 생성한 뒤 이동 대입하는 방식으로 구현되어 있다 — 즉 로직을 중복시키지 않고 이동 경로를 재사용한다.

**존재하는 연산자 오버로드:**
- `operator=` — 복사, 이동, `const wchar_t*` 세 가지.
- `operator+=` — `const FString&`, `const wchar_t*`, 단일 `wchar_t` 세 가지(각각 필요한 만큼 버퍼를 늘린다. `wchar_t` 오버로드는 해당 문자를 직접 기록하고 새 널 종료 문자를 쓴 뒤 `m_Length`를 증가시킨다).
- `operator+` — `const FString&`와 `const wchar_t*` 두 가지이며, 둘 다 "`*this`를 복사한 뒤 `+=`"로 구현되어 있다.
- `operator==` / `operator!=` — 먼저 길이를 비교하고, 그다음 `wcscmp`를 호출한다(양쪽 길이가 같고 0이 아닐 때만 호출되며, 두 빈 문자열은 `m_pData`를 건드리지 않고도 같다고 판정된다).
- `operator<` — 널 버퍼 케이스를 명시적으로 처리한 뒤(`nullptr < nullptr`은 `false`, `nullptr < non-null`은 `true`) `wcscmp(...) < 0`으로 폴백한다.
- `operator[]`(mutable과 const 둘 다) — `check(m_pData && Index >= 0 && Index < m_Length)`로 범위를 검사한다.

CLAUDE.md 로드맵에서는 `operator*`를 언급하지만 실제로는 존재하지 않는다 — 실제 헤더에는 `+`, `+=`, `==`, `!=`, `<`, `[]`, 그리고 대입 연산자들만 선언되어 있다.

**상태 조회.** `Len()`, `IsEmpty()`, `GetData()` — 모두 사소한 인라인 접근자다.

**검색 메서드**(`FString.cpp`):
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
`StartsWith`는 접두사 길이만큼 `wcsncmp`를 사용하고, `EndsWith`는 `m_pData + (m_Length - Suffix.m_Length)`부터 `wcscmp`를 사용한다. 세 메서드 모두 빈 검색 문자열은 자명하게 매칭되는 것으로 취급한다.

**변환 메서드:**
- `ToUpper()` / `ToLower()` — `*this`를 복사한 뒤, 그 복사본의 버퍼를 순회하며 수동으로 ASCII 범위 매핑(`'a'..'z'` <-> `'A'..'Z'`)을 수행한다. 로케일/유니코드 대소문자 폴딩은 없다.
- `Substring(int32 Start, int32 Length) const` — 범위를 검증하고(`Start`가 유효 범위 안에 있는지, `Length > 0`인지), `ActualLen`을 문자열 끝을 넘어가지 않도록 클램프한 뒤, 새 버퍼를 할당해 해당 구간을 `Memcpy`하고 널로 종료한다.
- `Split(wchar_t Delim, TArray<FString>& OutParts) const` — `OutParts`를 리셋한 다음, 단일 순방향 스캔(`i`를 `0`부터 `m_Length`까지 포함해서 순회)을 수행하며 구분자에 도달하거나 문자열 끝에 도달할 때마다 `Substring(Start, i - Start)`를 호출해 `OutParts`에 넣는다. 반환값은 `OutParts.Num()`이다.

**파싱 메서드:**
```cpp
int32 FString::ToInt() const { return !m_pData ? 0 : (int32)wcstol(m_pData, nullptr, 10); }
float FString::ToFloat() const { return !m_pData ? 0.f : wcstof(m_pData, nullptr); }
```
둘 다 C 런타임의 와이드 문자열 파서를 얇게 감싼 래퍼일 뿐이며, 버퍼가 널인 경우에는 0 값을 반환하는 가드가 있다.

**Printf.** 별도의 `Format` 함수는 없고 `Printf`만 있다.
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
고정 크기 4096-`wchar_t` 스택 버퍼와 `vswprintf`를 사용한다. 포맷팅이 실패하거나 아무것도 만들어내지 못하면(`vswprintf`는 잘림/오류 시 음수를 반환한다) 잘린 결과 대신 기본(빈) `FString`을 반환한다. 결과는 `FString(Buffer)`를 복사 생성하는 방식으로 만들어지므로, 최종 문자열은 스택 버퍼 그 자체가 아니라 정확한 크기로 새로 힙에 할당된 것이다.

## FName / FNamePool

**목적.** `FName`은 인터닝된(interned) 문자열을 가리키는, 복사와 비교 비용이 저렴한 핸들이다. 언리얼의 `FName`을 그대로 본떠, 동일한 문자열은 같은 내부 엔트리로 합쳐지고 비교/복사는 문자열 연산이 아니라 `uint32` 연산이 된다. 실제 저장과 인터닝 로직은 모두 `FNamePool`(`Engine/Include/Core/String/FNamePool.h/.cpp`)에 있으며, `FName` 자체(`Engine/Include/Core/String/FName.h`)는 그 풀 안의 인덱스를 감싸는 래퍼에 불과하다. 참고로, 이 트리에는 `FName.cpp`가 존재하지 않는다 — `ToString()`을 포함한 `FName`의 모든 멤버는 `FName.h`에 인라인으로 정의되어 있다.

**FNameEntry.** 엔트리별 별도 힙 할당이 없는, 고정 크기 인라인 저장 방식이다.
```cpp
// FNamePool.h
struct FNameEntry
{
    static const int32 NAME_SIZE = 64;
    wchar_t m_Name[NAME_SIZE];
};
```
배열이 구조체 내부에 인라인으로 들어있기 때문에, `FNamePool`의 `TArray<FNameEntry> m_Entries`는 이름들을 엔트리별 포인터 추적/할당 없이 연속된 메모리에 저장한다 — 그 대가로 이름 하나당 63자(+종료 문자)라는 확고한 상한이 생긴다(등록 시점에 강제되며, 아래에서 다룬다).

**FNamePool 싱글턴.** private 생성자와 삭제된 복사 연산으로 선언되며, 함수 로컬 static을 통해서만 노출된다.
```cpp
FNamePool& FNamePool::Get()
{
    static FNamePool Instance;
    return Instance;
}
```
이는 전형적인 마이어스 싱글턴(Meyers singleton) 패턴으로, 최초 사용 시점에 (C++11 이상에서는 스레드 안전하게) 지연 초기화되며, 번역 단위 간 정적 초기화 순서와 무관하게 "사용 전 생성"을 보장한다. 생성자는 센티널 이름을 미리 등록한다.
```cpp
FNamePool::FNamePool()
{
    FindOrRegister(L"None");
}
```
이 때문에 인덱스 `0`은 항상 `"None"`을 위해 예약된다 — `FName()`의 기본값 `m_Index(0)`과 `IsNone()`의 `m_Index == 0` 검사는 둘 다 `"None"`이 언제나 가장 먼저 등록되는 엔트리라는 사실에 의존한다.

**해싱.** `HashString`은 djb2 변형이며, 흔히 쓰이는 덧셈 결합 대신 XOR 결합을 사용한다.
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
(`(Hash << 5) + Hash`는 `Hash * 33`으로, 전형적인 djb2 승수다. 문자를 XOR로 결합하는 것은 "djb2a" 변형에 해당한다.)

**FindOrRegister — 실제 인터닝 경로.** 저장소는 밀집되어(dense) 인덱스로 직접 접근 가능한 `TArray<FNameEntry> m_Entries`와, 해시 -> 후보 엔트리 인덱스들을 매핑하는 `TMultiMap<uint32, uint32> m_HashToIndex`로 구성되며, 후자는 완전한 문자열 동등성 검사를 통해 해시 충돌을 해소하는 데 쓰인다.
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
단계별로 보면, `FName`을 문자열로부터 생성할 때마다 다음이 일어난다.
1. `HashString`(djb2a)으로 입력 문자열을 해시한다.
2. `m_HashToIndex.MultiFind(Hash)`로 같은 값으로 해시된 기존 엔트리 인덱스들의 버킷을 조회한다. 이는 전체 이름에 대해 `O(n)`이 아니라, 버킷에 도달하기까지 상각(amortized) `O(1)`이다.
3. 서로 다른 문자열이라도 32비트 해시값이 충돌할 수 있으므로, 그 버킷에 있는 모든 후보는 저장된 인라인 버퍼(`m_Entries[Index].m_Name`)와 완전한 `wcscmp` 비교를 거친 후에야 채택된다 — 이것이 충돌에 대한 안전망이다. 후보 중 하나가 문자열과 정확히 일치하면 그 기존 인덱스가 반환되고 새로 등록되는 것은 없다.
4. 일치하는 후보가 없으면(버킷이 비어 있거나, 모든 `wcscmp`가 실패한 경우) 이름은 새것으로 간주된다. 이때 길이가 64-`wchar_t` 용량과 비교 검사되고(`check(Length < FNameEntry::NAME_SIZE)` — 앞서 언급한 63자 상한이 강제되는 지점이며, 이는 우아한 실패 처리가 아니라 하드한 `assert` 방식의 `check`다), 종료 문자까지 포함한 문자열이 스택 지역 변수 `FNameEntry`에 `Memcpy`되며, 그 엔트리가 `m_Entries`에 추가된다(그 인덱스는 `Add` *이전*에 계산된 `NewIndex = m_Entries.Num()`, 즉 앞으로 그 엔트리가 차지하게 될 인덱스다). 그리고 동일한 `(Hash, NewIndex)` 쌍이 `m_HashToIndex`에 삽입되어 이후의 조회가 이를 찾을 수 있게 된다.
5. 새로 만들어졌거나 이미 찾은 인덱스가 반환된다.

`GetEntryName`은 역방향 조회로, 범위 검사를 거친 뒤 풀의 인라인 저장소를 가리키는 포인터를 그대로 반환한다(이 포인터는 풀이 살아있는 동안 유효하다. `m_Entries`는 추가만 되며 엔트리가 재배치되어 사라지지는 않기 때문이다 — 다만 `Add` 시 `TArray`가 성장하며 재할당이 일어나면 원칙적으로 배열 전체가 메모리상에서 이동할 수 있으므로, 호출부는 등록 사이에 원시 포인터를 캐시해두기보다 매번 다시 가져오는 것이 원칙이다).
```cpp
const wchar_t* FNamePool::GetEntryName(uint32 Index) const
{
    check(Index < (uint32)m_Entries.Num());
    return m_Entries[(int32)Index].m_Name;
}
```
`Num()`은 그냥 `m_Entries.Num()`으로 위임한다.

**FName 자체.** 객체 전체가 `uint32` 하나다.
```cpp
// FName.h
private:
    uint32 m_Index;
```
`const wchar_t*`나 `const FString&`로부터의 생성은 널/빈 입력을 가드하고(이 경우 `m_Index == 0`, 즉 `"None"`으로 남는다), 그 외에는 `FNamePool::Get().FindOrRegister(...)`를 호출한다.
```cpp
FName(const wchar_t* Name) : m_Index(0)
{
    if (Name && Name[0] != L'\0')
    {
        m_Index = FNamePool::Get().FindOrRegister(Name);
    }
}
```
비교는 순수한 인덱스 비교다 — 이것이 인터닝의 존재 이유 그 자체다.
```cpp
bool operator==(const FName& Other) const { return m_Index == Other.m_Index; }
bool operator!=(const FName& Other) const { return m_Index != Other.m_Index; }
bool operator<(const FName& Other) const { return m_Index < Other.m_Index; }
```
`ToString()`은 풀에 저장된 버퍼를 조회한 뒤 그것으로 `FString`을 복사 생성함으로써, 독립적으로 소유권을 가진 진짜 `FString`으로 되돌려준다.
```cpp
FString ToString() const
{
    const wchar_t* pEntryName = FNamePool::Get().GetEntryName(m_Index);
    return FString(pEntryName);
}
```
이 함수는 별도의 `.cpp` 파일에 out-of-line으로 정의되어 있지 않고 **헤더에 인라인으로** 정의되어 있으며, `Engine/Include/Core/String/` 트리 어디에도 `ENGINE_NOINLINE`/`noinline` 속성은 존재하지 않는다(검색으로 확인했다 — 저장소 전체에서 `NOINLINE`은 매치되지 않았고, `FName.cpp` 자체도 존재하지 않는다). 이는 이 저장소 자체의 `CLAUDE.md`에 있는 로드맵 항목과 모순된다. 해당 항목은 `FName::ToString()`이 "MSVC Debug 인라인 코드생성 버그"를 우회하기 위해 의도적으로 `FName.cpp`로 out-of-line 분리되고 `noinline`이 붙었다고 서술하고 있다. 실제로 존재하는 소스를 기준으로 보면, 그 마이그레이션은 애초에 수행되지 않았거나 이후 되돌려진 것으로 보인다 — 실제 코드에서 `ToString()`은 평범한 인라인 헤더 멤버다.

또한 `IsNone()`(`m_Index == 0`), `GetIndex()`(원시 인덱스 접근자), 그리고 `FName.h`에 있는 프리 함수 형태의 해시 특수화도 존재한다.
```cpp
inline uint32 GetTypeHash(const FName& Name)
{
    return GetTypeHash(Name.GetIndex());
}
```
이를 통해 `FName`은 문자열을 다시 해싱하는 대신 인덱스를 해싱함으로써(`HashFunctions.h`에 이미 있는 `GetTypeHash(uint32)` 오버로드를 사용해서) 엔진의 `TMap`/`TSet`/`TMultiMap`에서 키로 사용될 수 있다 — 이는 일단 인터닝된 `FName`은 해싱이든 비교든 다시는 자신의 문자열 내용을 건드릴 필요가 없다는 점을 다시금 보여준다.

## FText

**실제로 무엇인가.** `FText`(`Engine/Include/Core/String/FText.h/.cpp`)는 단일 `FString` 멤버를 감싼 얇은 값 타입 래퍼다 — 이 구현에는 로컬라이제이션 테이블도, 컬처/로케일 키도, 포맷 인자 지원도, "원본 문자열 대 표시 문자열"이라는 별도 개념도 존재하지 않는다. private 상태 전체는 다음과 같다.
```cpp
private:
    FString m_String;
```

**생성/대입.** 여섯 개의 특수 멤버 함수 모두가 `FText.cpp`에 out-of-line으로 선언·정의되어 있으며, 각각은 그저 대응하는 `FString` 연산으로 위임할 뿐이다.
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
이동 생성자가 그 자체 시그니처 상 `noexcept`로 표시되어 있지 않은 것처럼 보일 수 있는데 — 실제로 확인해보면: 헤더의 `FText(FText&& Other) noexcept`는 `noexcept`이며, `FText.cpp`에 있는 대응 정의(`FText::FText(FText&& Other) noexcept`) 역시 이와 정확히 일치한다. 즉 선언과 정의가 서로 일치한다.

**나머지는 전부 헤더에 인라인으로 있다.**
```cpp
bool operator==(const FText& Other) const { return m_String == Other.m_String; }
bool operator!=(const FText& Other) const { return m_String != Other.m_String; }
const FString& ToString() const { return m_String; }
bool IsEmpty() const { return m_String.IsEmpty(); }
```
`operator==`/`operator!=`는 `FString` 자체의 비교 연산자(위에서 설명한 길이 검사 + `wcscmp`)로 그대로 위임한다. `ToString()`은 `const FString&`을 반환한다 — 복사본이 아니라 내부 버퍼에 대한 참조다(풀 저장소로부터 새 `FString`을 만들어내야 하는 `FName::ToString()`과는 다르다). `IsEmpty()`는 `FString::IsEmpty()`로 위임한다.

요컨대, 이 프로젝트의 다른 기획 문서에서 시사하는 "3종 문자열 타입 / 로컬라이제이션 래퍼"라는 틀과는 달리, 실제로 존재하는 코드는 `FText`를 단지 별개의 타입 정체성과 제한된 인터페이스를 가진 `FString`으로 구현하고 있을 뿐이다(직접적인 `[]` 인덱싱도, `+=`도, `Split`/`Contains` 등도 없다 — `FString`의 검색/변환/파싱 메서드는 어느 것도 `FText`를 통해 노출되지 않는다). `FText.h`/`FText.cpp` 어디에도 로컬라이제이션 키, 컬처, 로케일에 따른 런타임 재해석 같은 것은 존재하지 않는다.

---

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