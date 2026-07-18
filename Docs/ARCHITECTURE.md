# 엔진 아키텍처 상세 — 구현 방법과 동작 원리

> 각 시스템이 **어떻게 구현되어 있고, 내부에서 어떻게 동작하는지**를 코드 기준으로 설명한다.
> 모든 코드는 `Engine/Include/` 실제 소스 기준.

---

## 1. 메모리 시스템

### 1.1 전체 구조 — 모든 할당이 한 곳을 지나간다

```
new Foo()  ──┐
TArray 성장 ──┼──▶ FMemory::Malloc/Free ──▶ GMalloc (IAllocator*) ──▶ FMallocBinned
FString 버퍼─┘         (정적 진입점)          (교체 가능한 전역)        (실제 구현)
```

- `IAllocator`는 Malloc/Realloc/Free 3개의 순수 가상 함수 인터페이스
- `GMalloc`은 전역 `IAllocator*` — `FMemory::InitMemory()`에서 `FMallocBinned` 인스턴스를 꽂음.
  한 줄만 바꾸면 엔진 전체의 할당자가 교체됨 (실제로 디버깅 때 FMallocAnsi로 바꿔 이분 탐색에 활용)
- 전역 `operator new/delete`를 오버라이드해 일반 `new`도 GMalloc을 경유.
  **sized delete**(`operator delete(void*, size_t)`)까지 오버라이드 — C++14부터 컴파일러가
  크기를 아는 경우 sized 버전을 우선 호출하므로, 빠뜨리면 그 경로가 엔진 할당자를 우회한다

### 1.2 FMallocBinned — 동작 원리

**핵심 아이디어: "포인터만 보고 어느 Bin에서 왔는지 알아내기"**

```
64KB 정렬된 64KB 페이지:
┌────────────┬──────┬──────┬──────┬─ ─ ─┐
│ FPageHeader │ 블록0 │ 블록1 │ 블록2 │ ... │   블록 크기 = 이 페이지의 Bin 크기 (16~512B)
└────────────┴──────┴──────┴──────┴─ ─ ─┘
▲ 페이지 시작 주소는 항상 64KB의 배수
```

- **Malloc(작은 크기)**: 크기→Bin 인덱스(16/32/64/128/256/512) → 해당 Bin 프리 리스트에서 pop.
  비어있으면 `_aligned_malloc(64KB, 64KB)`로 새 페이지를 받아 블록으로 잘라 리스트에 채움(GrowBin)
- **Free(ptr)**: `ptr & ~(65536-1)` — 하위 16비트만 지우면 페이지 시작 주소.
  거기 있는 FPageHeader에서 Bin 인덱스를 읽고 그 Bin 프리 리스트에 push.
  **블록마다 헤더를 붙일 필요가 없어 8바이트짜리 할당도 오버헤드 0**
- **프리 리스트는 침습적(intrusive)**: 해제된 블록의 메모리 자체에 다음 블록 포인터를 저장
  (`FFreeBlock { FFreeBlock* m_pNext; }`) — 별도 관리 메모리 불필요
- **큰 할당/큰 정렬**: 페이지 단위로 통째로 할당(AllocateLarge), 헤더에 `LARGE_BIN` 표시.
  Free에서 같은 마스킹으로 구분해 OS로 반납
- 방어: 헤더에 매직 넘버(0xB17EED00) + Bin 인덱스 범위 check, `static_assert(sizeof(FPageHeader) <= HEADER_SIZE)`

### 1.3 FNamePool과의 궁합

FName 엔트리(128B), FRefCountBlock(24B), TMap 버킷 등 엔진의 잦은 소형 할당이
전부 Bin에서 O(1)로 나가고 재사용됨 → 외부 단편화 제거.

---

## 2. 컨테이너

### 2.1 TArray — placement new와 POD 분기

**저장소는 "원시 메모리"다.** `FMemory::Malloc`으로 바이트만 확보하고, 원소 생성은 placement new로 직접:

```cpp
new (m_pData + m_Size) T(Element);     // 추가: 그 자리에 생성자 직접 호출
m_pData[Index].~T();                    // 제거: 소멸자 직접 호출
```

**POD 최적화** — 컴파일 타임 분기라 런타임 비용 0:

```cpp
if constexpr (TIsTriviallyCopyable<T>::Value)
    FMemory::Memcpy(...);              // int, FVector2D 등: 비트 복사
else
    { new (dst) T(MoveTemp(src)); src.~T(); }   // FString 등: 이동 생성 + 파괴
```

**성장 전략**: capacity 0→4, 이후 ×2. Reserve/Shrink/Reset(용량 유지)/Empty(해제) 구분.

### 2.2 TInlineAllocator — 스택 인라인 저장

```cpp
TArray<FName, TInlineAllocator<4>> Tags;   // 첫 4개는 힙 할당 0회
```

- 두 번째 템플릿 파라미터로 할당자 정책을 받음. `TInlineAllocator<N>`이면 배열 객체 **내부에**
  `alignas(T) uint8[sizeof(T)*N]` 버퍼를 둠 (`TArrayInlineStorage` 베이스)
- `m_pData`가 이 내부 버퍼를 가리키면 "인라인 상태". N 초과 시 힙 버퍼로 원소 이전(스필),
  Shrink로 다시 N 이하가 되면 인라인 복귀
- **이동 의미론이 두 갈래**: 힙 상태면 포인터만 탈취(O(1)), 인라인 상태면 원소 단위 이동
  (내 버퍼는 남의 객체 안에 있으므로 탈취 불가)
- 기본 `TArray<T>`는 `TDefaultAllocator`(N=0) — 빈 베이스 클래스 최적화(EBO)로 크기 증가 0,
  기존 코드와 100% 호환

### 2.3 TSparseArray — 안정 인덱스 + 프리 리스트

**"지워도 인덱스가 안 변하는 배열"** — 해시 컨테이너의 백본.

```
슬롯:  [0: A] [1: (빈)→3] [2: B] [3: (빈)→NONE] [4: C]
                 ▲ m_FirstFree = 1 (프리 리스트가 빈 슬롯을 체인으로 연결)
```

- 각 슬롯 = `{ alignas(T) 원시 저장소, m_NextFree, m_bAllocated }`
- **Add/Emplace**: 프리 리스트가 있으면 pop해서 그 자리 재사용, 없으면 HighWater 증가.
  반환값은 그 원소의 영구 인덱스
- **RemoveAt(i)**: 소멸자 호출 → 슬롯을 프리 리스트 head에 push. O(1), 다른 원소는 안 움직임
- **성장**: 새 버퍼에 같은 인덱스로 재배치(move+destroy 또는 memcpy) — 인덱스 불변이 핵심 계약
- **구조 보존 복사**: 복사본도 같은 인덱스/프리 리스트 구조를 가짐 → 해시 컨테이너가
  버킷 배열을 그대로 memcpy 복사 가능
- 반복자는 `m_bAllocated == false`인 홀을 건너뜀

### 2.4 TSet / TMap — 해시 버킷 인덱스 체인 (언리얼 실제 구조)

```
버킷 배열(int32, pow2):      원소 저장소(TSparseArray):
buckets[hash & (N-1)] ──▶ [3] ──m_HashNext──▶ [0] ──▶ INDEX_NONE
                            └ 같은 버킷에 걸린 원소들이 인덱스 체인으로 연결
```

- **탐색**: 해시로 버킷 하나 고르고, 그 체인만 따라가며 Key 비교 — 관계없는 칸을 지나갈 일 없음
- **삽입**: 체인 중복 검사 → `m_Elements.Emplace(Key, 기존 head)` → 버킷 head를 새 인덱스로 (체인 앞에 끼움)
- **삭제**: `int32* pLink`로 체인을 따라가다 발견하면 `*pLink = 그 원소의 m_HashNext`(unlink) →
  슬롯 반납. **톰스톤 개념 자체가 없음** — 추가/삭제를 무한 반복해도 성능 불변
- **Rehash**: 로드 팩터 1.0 초과 시 버킷 배열만 2배로 새로 만들고 전 원소를 재링크.
  원소(Key/Value)는 한 바이트도 안 움직임 — TSparseArray 인덱스가 안정적이기 때문
- TMap은 원소가 `{Key, Value, m_HashNext}`, TSet은 `{Key, m_HashNext}` — 골격 동일

**vs 이전 구조(Open Addressing)**: 예전엔 삭제 시 Deleted 톰스톤을 남겨야 했고(뒤 원소 탐색이
끊기므로), 톰스톤이 쌓이면 탐색이 느려지고 불필요한 Rehash가 발생했다. 현 구조로 전환하며 해결.

---

## 3. 문자열

### 3.1 FName / FNamePool — 문자열 인터닝

**"같은 문자열은 풀에 한 번만 저장하고, 이후엔 번호로만 다룬다"**

```
FName(L"Player") 생성:
  1. djb2 해시 계산 (h = h*33 ^ ch)
  2. m_HashToIndex.MultiFind(hash) → 후보 인덱스들
  3. 각 후보를 wcscmp로 실제 비교 (해시 충돌 안전망)
  4. 있으면 그 인덱스, 없으면 m_Entries에 추가 후 새 인덱스
  → FName은 uint32 인덱스 하나만 보관

FName == FName  →  인덱스 정수 비교 한 번 (O(1), 문자열 길이 무관)
```

- `FNameEntry { wchar_t m_Name[64]; }` — 문자열을 구조체 **내부에 인라인 저장**.
  FString처럼 별도 힙 포인터가 없어 엔트리당 할당 1회 절약 + 포인터 기반 해시 문제 원천 제거
- 엔트리 0 = "None" 예약 → 기본 생성 FName은 IsNone()
- TMap 키로 쓸 때 `GetTypeHash(FName) = GetTypeHash(인덱스)` — 재해싱도 정수 해시

### 3.2 FString

wchar_t 동적 버퍼(m_pData/m_Length/m_Capacity) + Printf(vswprintf)/Split/ToInt/ToLower 등.
복사 시 깊은 복사, 이동 시 포인터 탈취.

---

## 4. 스마트 포인터

### 4.1 컨트롤 블록과 수명 이원화

```
TSharedPtr<T> ──┬──▶ T 객체            (SharedCount가 0 되면 파괴)
                └──▶ FRefCountBlock     (SharedCount·WeakCount 모두 0 되면 해제)
TWeakPtr<T>  ────────▲ (블록만 참조 — IsValid()가 SharedCount>0 확인에 블록이 필요)
```

**왜 둘을 따로 죽이나**: TWeakPtr는 객체가 죽은 뒤에도 "죽었는지" 물어볼 수 있어야 한다.
그래서 객체는 먼저 죽어도 블록은 마지막 weak 참조가 사라질 때까지 살아있어야 한다.

### 4.2 암묵적 weak 참조 — 이중 해제 방지의 핵심

```cpp
FRefCountBlock() : m_SharedCount(1), m_WeakCount(1) ...   // WeakCount가 1로 시작!
```

WeakCount의 1은 "shared 소유자 그룹 전체"가 가진 몫이다. 마지막 TSharedPtr가 해제될 때:

```cpp
if (--SharedCount == 0) {
    Deleter(객체);              // ← 이 안에서 멤버 TWeakPtr들이 자기 WeakCount를 반납해도
    if (--WeakCount == 0)       //    그룹 몫 1이 남아있어 블록은 아직 살아있음
        Free(블록);             // ← 그룹 몫까지 반납된 뒤에야 해제
}
```

이 패턴이 없으면(초기 WeakCount=0) 순환 참조 파괴 시 Deleter 체인 내부의 TWeakPtr 소멸자가
블록을 먼저 해제해버려, 바깥 코드가 해제된 블록을 만지는 이중 해제가 발생한다 (실제로 겪고 고친 버그).

### 4.3 원자성

카운트 증감은 `FSmartPtrAtomics` — MSVC `_InterlockedIncrement/Decrement`,
GCC `__atomic_add_fetch` 분기. `--x == 0` 판정이 "감소 후 값"을 원자적으로 받아
두 스레드가 동시에 마지막 해제를 수행하는 레이스를 방지한다.
(검증: 4~8스레드 × 수십만 회 복사/해제 후 카운트 정확히 1)

---

## 5. UObject / Cast — RTTI 없는 타입 시스템

### 5.1 UClass 메타데이터는 어디서 오나

```cpp
#define DECLARE_CLASS(TClass, TSuperClass)                  \
    using Super = TSuperClass;                              \
    static UClass* StaticClass() {                          \
        static UClass s_ClassInfo(L#TClass, TSuperClass::StaticClass());  \
        return &s_ClassInfo;                                \
    }                                                       \
    virtual UClass* GetClass() const override { return TClass::StaticClass(); }
```

- 클래스마다 **함수-지역 정적** UClass 하나 — 최초 호출 시 생성(magic statics로 스레드 안전),
  전역 초기화 순서 문제 없음
- `s_ClassInfo`가 부모의 StaticClass() 포인터를 보관 → **UClass들이 상속 체인을 형성**
- `GetClass()`는 가상 함수 — 포인터의 정적 타입이 아니라 **실제 객체의 타입**을 반환

### 5.2 Cast의 동작

```cpp
template<typename T, typename U>
T* Cast(U* Obj) {
    if (Obj && Obj->GetClass()->IsChildOf(T::StaticClass()))
        return static_cast<T*>(Obj);
    return nullptr;
}
```

- `IsChildOf`: 내 UClass에서 SuperClass 체인을 따라 올라가며 대상 UClass 포인터와 비교
- dynamic_cast와 달리: vtable/RTTI 데이터 불필요, 체인 길이만큼의 포인터 비교(얕은 상속에선 1~3회),
  단 다중 상속/가상 상속은 지원하지 않음(엔진 클래스 계층은 단일 상속이므로 충분)
- CastChecked = 실패 시 check, ExactCast = 체인 없이 포인터 1회 비교

---

## 6. Gameplay Ability System

### 6.1 데이터 흐름

```
UGameplayEffect (정의: 무엇을 얼마나)          UAbilitySystemComponent (인스턴스: 지금 상태)
  DurationType / Period / MaxStacks    ──적용──▶  m_ActiveEffects: TArray<FActiveGameplayEffect>
  m_Modifiers (속성, 연산, 크기)                     (남은 시간, 도트 타이머, 스택 수)
  m_GrantedTags                                    m_ActiveTags (현재 붙어있는 태그들)
                                                   m_pAttributeSet (HP/MP/ATK...)
```

### 6.2 Tick 한 번에 일어나는 일

1. 각 활성 효과의 `m_Duration -= DeltaTime` (Infinite 제외)
2. `m_Period > 0`이면 `m_PeriodTimer -= DeltaTime`, 0 이하가 될 때마다 Modifier를
   **BaseValue에 직접 적용**(독 도트가 HP를 영구 차감) 후 타이머 리셋
3. Duration 만료 → GrantedTags 제거 + 효과 제거
4. 변화가 있었으면 `RecalculateAttributes()`:
   모든 속성 Current=Base로 리셋 → 활성 효과들의 Modifier를 **Add → Multiply → Override
   순서의 3패스**로 재적용 (버프가 사라지면 자동으로 원상복구되는 이유)

### 6.3 스킬 발동 게이트

`TryActivateAbility` → `CanActivate`:
- `m_ActivationBlockedTags`와 ASC의 현재 태그 교집합 있으면 차단 (스턴, 쿨다운 태그)
- 비용(MP) 충분한지 확인 → 통과 시 CostEffect(Instant) 적용 + CooldownEffect(Duration+태그) 적용

쿨다운 = "Duration 효과가 `Cooldown.X` 태그를 부여하고, 스킬의 BlockedTags에 그 태그가
들어있는 것" — 별도 쿨다운 시스템이 없는 언리얼 GAS 방식 그대로.

---

## 7. Assert / 로깅 정책

| 매크로 | Debug | Release | 용도 |
|---|---|---|---|
| `check(expr)` | 평가+실패 시 중단 | **표현식 자체가 제거됨** | 불변식 검사. 부수효과 금지 |
| `verify(expr)` | 평가+실패 시 중단 | **평가만 함** | 반드시 실행돼야 하는 호출의 결과 검사 |
| `ensure(expr)` | 최초 1회만 브레이크, bool 반환 | 동일 | 복구 가능한 이상 상황을 if와 함께 |

Release에서 check를 제거하는 것은 언리얼 Shipping 관례를 따른 의도적 선택
(핫패스의 검사 비용 제거). 대신 "check 안에 상태 변경 금지" 규약을 테스트 코드까지 강제.

---

## 8. 검증 하니스

- Test 프로젝트: main() 하나에서 Phase 1→7.5+ 순차 실행, 86개 체크포인트
- 스트레스: TMap/TSet 100키×10라운드 처닝, TSparseArray 인덱스 재사용,
  CreateThread 4스레드×20만회 TSharedPtr 복사(원자성), FNamePool 1000개 대량 등록
- 구성 매트릭스: Windows Debug/Release + Linux g++ O0/O2/O2+NDEBUG + valgrind
