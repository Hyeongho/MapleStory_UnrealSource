# CLAUDE.md - MapleStory DX11 2D 엔진 프로젝트

## 프로젝트 개요

DirectX 11 기반 2D 엔진을 STL 없이 언리얼 엔진 아키텍처를 따라 C++17로 직접 구현한다.  
목표 게임: MapleStory 스타일 2D 플랫포머 RPG  
포트폴리오 목적: 넥슨 MapleStory 팀 등 대형 스튜디오 지원

---

## 핵심 원칙

- STL 사용 금지 — std::vector, std::string, std::unordered_map 전부 자체 구현으로 대체
- 예외 처리 금지 — /EHs-c- 컴파일 옵션, check() 매크로로 대체
- RTTI 금지 — /GR- 컴파일 옵션, UClass 기반 Cast<T>() 직접 구현
- 언리얼 네이밍 컨벤션 — TArray, TMap, FString, FName, UObject, AActor
- 단위 테스트 필수 — 각 Phase 완료 시 Tests 프로젝트에서 검증 후 다음 단계 진행

---

## 솔루션 구조

```
MyEngine.sln
├── Engine/          → 정적 라이브러리 (.lib)
│   ├── Core/
│   │   ├── Memory/          (Phase 1)
│   │   ├── Templates/       (Phase 2)
│   │   ├── Containers/      (Phase 3~4)
│   │   ├── String/          (Phase 5)
│   │   ├── Logging/         (Phase 5.5)
│   │   ├── SmartPointer/    (Phase 6)
│   │   └── Math/            (Phase 3.5)
│   ├── Object/              (Phase 7)
│   ├── Timer/               (Phase 7.5)
│   ├── Ability/             (Phase 7.7)
│   ├── Renderer/            (Phase 8)
│   ├── Animation/           (Phase 9)
│   ├── Physics/             (Phase 10)
│   ├── Audio/               (Phase 11)
│   ├── UI/                  (Phase 12)
│   ├── Input/               (Phase 13)
│   ├── Resource/            (Phase 14)
│   ├── World/               (Phase 15)
│   └── AI/                  (Phase 18)
├── Game/            → 실행 파일 (.exe)  Engine 참조
└── Tests/           → 단위 테스트 (.exe)  Engine 참조
```

---

## 컴파일러 설정 (VS 프로젝트 속성)

```
구성 형식:          정적 라이브러리 (.lib)  [Engine]
                    응용 프로그램 (.exe)     [Game, Tests]
C++ 표준:           /std:c++17
예외 처리:          /EHs-c-   (예외 비활성화)
RTTI:               /GR-      (dynamic_cast 비활성화)
경고 수준:          /W4
추가 포함 디렉터리: $(SolutionDir)Engine/
미리 컴파일된 헤더: EnginePCH.h 사용
```

---

## EnginePCH.h (기본 타입 정의)

```cpp
#pragma once
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <malloc.h>
#include <new>
#include <cassert>

using int8   = int8_t;
using int16  = int16_t;
using int32  = int32_t;
using int64  = int64_t;
using uint8  = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

#define INDEX_NONE   -1
#define check(expr)  assert(expr)
#define verify(expr) assert(expr)
```

---

## 전체 구현 로드맵

### [LAYER 1] Engine Core — Phase 0~7.5 (1~6주)

---

### Phase 0 — 프로젝트 세팅 (2~3일) ✅

목표: 솔루션 빌드 성공 + 폴더 구조 확정

- [x] VS 솔루션 생성 (Engine .lib / Game .exe / Tests .exe)
- [x] 폴더 구조 생성
- [x] EnginePCH.h 작성
- [x] 컴파일러 옵션 설정 (/GR- /W4)
- [x] 빈 main.cpp — "Engine Init" 출력 후 빌드 성공 확인
- [x] Git 첫 커밋

완료 기준: `>Engine Init` 출력 후 오류 없이 빌드됨

---

### Phase 1 — Memory 시스템 (3~4일) ✅

파일 위치: `Engine/Core/Memory/`

- [x] `IAllocator.h / .cpp` — 인터페이스 (Malloc / Realloc / Free)
- [x] `FMallocAnsi.h / .cpp` — _aligned_malloc 래핑
- [x] `FMemory.h / .cpp` — GMalloc 전역 포인터 + InitMemory() + MemoryOps
- [x] `MemoryOverride.cpp` — operator new / delete 전역 오버라이드
- [x] Pool Allocator (몬스터·파티클 대량 생성용) — FPoolAllocator
- [x] Stack(Frame) Allocator (프레임 단위 임시 할당) — FStackAllocator
- [x] 메모리 추적 / 릭 감지 (DEBUG 빌드 전용) — FMemoryTracker

완료 기준: `new MyClass()` 호출 시 GMalloc 경유 로그 확인 ✅

---

### Phase 2 — TypeTraits / 유틸리티 (2~3일) ✅

파일 위치: `Engine/Core/Templates/`

- [x] `TypeTraits.h` — TIsPOD, TIsTriviallyCopyable, TEnableIf, TConditional, TDecay, TIsPointer, TIsEnum
- [x] `Utility.h` — MoveTemp, Forward, Swap
- [x] `AndOrNot.h` — TAnd, TOr, TNot

완료 기준: `static_assert(TIsPOD<int>::Value == true)` 통과 ✅

---

### Phase 3 — TArray (4~5일) ✅

파일 위치: `Engine/Core/Containers/`

- [x] Reserve / Grow (x2 성장 전략)
- [x] Add(const T&) / Add(T&&) / Emplace (placement new)
- [x] RemoveAt(index) — O(n) 순서 유지
- [x] RemoveAtSwap(index) — O(1) 순서 파괴
- [x] Find / Contains
- [x] Sort / StableSort
- [x] FilterByPredicate / RemoveAll
- [x] Reset (Capacity 유지) / Empty (메모리 해제)
- [x] 복사 생성자 / 이동 생성자
- [x] POD 분기 최적화 (if constexpr)
- [x] TArrayView (비소유 슬라이스)
- [x] begin / end (범위 기반 for)

완료 기준: 비POD 타입 소멸자 호출 확인 + 메모리 릭 없음 ✅

---

### Phase 3.5 — 수학 라이브러리 (4~5일) ★추가 ✅

파일 위치: `Engine/Core/Math/`

- [x] `FMath.h / .cpp` — Lerp, Clamp, Abs, Min, Max, Sin, Cos, Atan2, Sqrt, Pow, FMod
- [x] `FColor.h / .cpp` / `FLinearColor.h / .cpp` — 피격 깜빡임·스프라이트 틴트
- [x] `FVector2D.h / .cpp` — x, y + 전체 연산자 (+,-,*,/), 내적, 정규화, 크기
- [x] `FIntPoint.h / .cpp` / `FIntRect.h / .cpp` — 타일맵·NavGrid 정수 좌표
- [x] `FRect.h / .cpp` — AABB 충돌 전용 (Left, Top, Right, Bottom)
- [x] `FTransform2D.h / .cpp` — Location(FVector2D) + Rotation(float) + Scale(FVector2D)
- [x] `FMatrix3x3.h / .cpp` / `FMatrix4x4.h / .cpp` — 렌더러 변환 행렬
- [x] `FRandomStream.h / .cpp` — 시드 기반 난수 (드롭 확률·몬스터 스폰)

완료 기준: FVector2D 사칙연산 + FMath::Lerp 단위 테스트 통과 ✅

---

### Phase 4 — TMap / TSet (4~5일) ✅

파일 위치: `Engine/Core/Containers/`

- [x] `HashFunctions.h` — GetTypeHash 특수화 (int32, uint32, int64, wchar_t*, FName)
- [x] `TMap.h / .cpp` — Open Addressing + Rehash (Load Factor 0.75)
- [x] `TSet.h / .cpp`
- [ ] `TSparseArray.h / .cpp` — 언리얼 TMap 내부 구조 (Phase 7.5+ 최적화 시 적용)
- [x] `TMultiMap.h / .cpp` — 한 키에 여러 값 (스킬 태그 시스템)

완료 기준: TMap 1000개 삽입·검색·삭제 + Rehash 동작 확인 ✅

---

### Phase 5 — FString / FName / FText (3~4일) ✅

파일 위치: `Engine/Core/String/`

- [x] `FString.h / .cpp` — wchar_t 기반, TArray 활용, 전체 연산자 (+, ==, !=, +=, *)
- [x] `FString::Printf` / `FString::Format` (데미지 숫자 포맷)
- [x] `FString` 파싱 — Split, Contains, StartsWith, EndsWith, ToInt, ToFloat
- [x] `FName.h / .cpp` — TArray<FString> 선형 탐색 + uint32 인덱스 O(1) 비교
- [x] `FText.h / .cpp` — 다국어 지원 래퍼 (언리얼 3종 문자열 체계)

완료 기준: FName 비교 속도가 FString 비교보다 빠름을 측정으로 확인 ✅

---

### Phase 5.5 — 로깅 / 에러처리 (2일) ★추가 ✅

파일 위치: `Engine/Core/Logging/`

- [x] `check(expr)` — 항상 터지는 하드 assert
- [x] `verify(expr)` — 릴리즈에서도 평가, 실패 시 assert
- [x] `ensure(expr)` — 한 번만 터지는 soft assert
- [x] `UE_LOG(Category, Level, Format, ...)` 매크로  
  레벨: Verbose / Log / Warning / Error / Fatal  
  카테고리: LogCore / LogAI / LogUI / LogRenderer / LogPhysics
- [x] 로그 파일 저장 (`logs/engine.log`)
- [x] 예외 없는 에러 전파 전략 (`TResult<T,E>` 패턴) — `Engine/Core/Templates/TResult.h`

완료 기준: `UE_LOG(LogCore, Warning, L"test %d", 42)` 콘솔 + 파일에 기록 ✅

---

### Phase 6 — TSharedPtr / TWeakPtr (3일) ✅

파일 위치: `Engine/Core/SmartPointer/`

- [x] `SharedPointerInternals.h` — FRefCountBlock (SharedCount + WeakCount)
- [x] `TSharedPtr.h / .cpp` — 복사 / 이동 / 소멸
- [x] `TSharedRef.h / .cpp` — null 불가 버전
- [x] `TWeakPtr.h / .cpp` — IsValid() / Pin()
- [x] `MakeShared<T>()` 헬퍼
- [x] 순환 참조 테스트 케이스 (보스↔파츠 참조 구조)

완료 기준: 순환 참조 상황에서 메모리 릭 없음 확인 ✅

---

### Phase 7 — UObject / Cast 시스템 (5~6일) ✅

파일 위치: `Engine/Object/`

- [x] `UClass.h / .cpp` — Name(FName) + SuperClass + IsChildOf() 체인
- [x] `DECLARE_CLASS(TClass, TSuperClass)` 매크로 — StaticClass() + GetClass()
- [x] `Cast<T>(obj)` — 실패 시 nullptr
- [x] `CastChecked<T>(obj)` — 실패 시 check() assert
- [x] `ExactCast<T>(obj)` — 정확히 그 타입만
- [x] `TSubclassOf<T>` — 타입 안전 클래스 레퍼런스 (직업 등록용)
- [x] `UObject.h / .cpp` — 베이스 클래스 (BeginPlay, Tick, EndPlay)
- [x] `AActor.h / .cpp` — AddComponent<T>() / GetComponent<T>() 템플릿
- [x] `UActorComponent.h / .cpp` / `USceneComponent.h / .cpp` (Transform 보유)
- [x] UPROPERTY / UFUNCTION 매크로 기초 (stub)
- [ ] CDO — Class Default Object (아이템·몬스터 기본값) ← Phase 7.5+ 예정
- [ ] UObject 완전 GC (몬스터 사망 후 자동 해제) ← Phase 7.5+ 예정

완료 기준: `Cast<USpriteComponent>(comp)` 정상 동작 확인 ✅

---

### Phase 7.5 — 타이머 시스템 (2~3일) ★추가 ✅

파일 위치: `Engine/Timer/`

- [x] `FTimerHandle.h / .cpp` — 개별 타이머 식별자
- [x] `FTimerManager.h / .cpp` — SetTimer / ClearTimer / PauseTimer / ResumeTimer
- [x] `SetTimerNextFrame` — 지연 실행 (사망 후 N초 뒤 리스폰)
- [x] `GetDeltaTime()` / `GetTimeSeconds()` 전역 접근 (`FTimerManager.h/.cpp`에
  `TickGlobalClock()`과 함께 자유 함수로 구현 — `FTimerManager` 클래스와는
  무관한 파일 스코프 QueryPerformanceCounter 기반 클럭. `Game/Include/main.cpp`
  게임 루프에 실제로 연결 완료: 매 프레임 `TickGlobalClock()` 호출 후
  `GetDeltaTime()`으로 읽은 값을 `GTimerManager->Tick(DeltaTime)`에 그대로
  전달하며, 기존에 루프 안에서 직접 하던 QueryPerformanceCounter 호출은
  제거됨)

완료 기준: 3초 뒤 콜백 정확히 호출 확인 ✅, `main.cpp` 게임 루프에서
매 프레임 `TickGlobalClock()`/`GetDeltaTime()`로 델타타임 공급 확인 ✅

---

### Phase 7.7 — Gameplay Ability System (6~7일) ★추가 ✅

파일 위치: `Engine/Ability/`

**설계 원리:** RTTI 없이 UClass + Cast<T> 기반으로 언리얼 GAS를 직접 구현.  
MapleStory의 패시브·액티브 스킬, 독 도트, 힐, 쿨다운, 장비 스탯, 상태이상을 모두 커버.

#### 핵심 타입 (`AbilityTypes.h`)

```
EGameplayEffectDurationType : Instant / Duration / Infinite
EGameplayModifierOperation  : Add / Multiply / Override
FGameplayEffectModifier     : AttributeName + Operation + Magnitude
FActiveGameplayEffect       : pSpec + Duration + PeriodTimer + StackCount
FGameplayAbilitySpec        : pAbility + Level + bIsActive
```

#### 구현 파일 목록

- [x] `AbilityTypes.h` — 공통 열거형·구조체 (헤더 전용)
- [x] `FGameplayTag.h / .cpp` — 계층 태그 (L"Skill.Attack.Slash", L"Status.Stun")
  - `MatchesParent()` — "Skill.Attack"이 "Skill"의 자식인지 문자열 접두사로 판별
- [x] `FGameplayTagContainer.h / .cpp` — 태그 묶음
  - `HasTag()` / `HasParentTag()` / `HasAnyTag()` / `HasAllTags()`
- [x] `FGameplayAttribute.h` — 단일 속성 (BaseValue + CurrentValue + Min/Max 클램프, 헤더 전용)
- [x] `UAttributeSet.h / .cpp` — `TMap<FName, FGameplayAttribute>` 기반 속성 집합
  - `InitAttribute(Name, Base, Min, Max)` / `GetAttribute()` / `GetCurrentValue()`
- [x] `UGameplayEffect.h / .cpp` — 효과 정의
  - Instant: 즉시 적용 후 소멸 (데미지, 회복)
  - Duration: N초 유지 후 만료 (버프/디버프)
  - Infinite: 명시 제거 전까지 유지 (패시브, 장비 스탯)
  - `m_Period` — 0이면 없음, >0이면 N초마다 Modifier 재적용 (독 도트)
  - `m_MaxStacks` — 중첩 버프 최대 스택 수 (기본 1)
- [x] `UGameplayAbility.h / .cpp` — 스킬 정의
  - `m_pCostEffect` — MP 소모 효과
  - `m_pCooldownEffect` — 쿨다운 태그 부여 효과
  - `m_ActivationBlockedTags` — 스턴 등 차단 조건
  - `virtual CanActivate()` / `ActivateAbility()` / `EndAbility()`
- [x] `UAbilitySystemComponent.h / .cpp` — 캐릭터에 붙는 메인 컴포넌트 (UActorComponent 상속)
  - `SetAttributeSet()` / `GetAttributeCurrentValue()`
  - `ApplyGameplayEffect()` — Instant 즉시 처리, Duration/Infinite 목록 추가
  - `RemoveEffectsWithTag()` — 상태이상 해제 스킬에서 사용
  - `GrantAbility()` / `TryActivateAbility()` / `TryActivateAbilityByTag()`
  - `Tick(DeltaTime)` — Duration 차감, Period 도트 발동, 만료 효과 제거
  - `AddLooseTag()` / `RemoveLooseTag()` — 직접 태그 조작

#### MapleStory 패턴별 구현 방식

| 패턴 | GAS 구현 |
|------|---------|
| 패시브 스킬 (ATK +20% 영구) | Infinite UGameplayEffect, 스킬 습득 시 적용 |
| 독 디버프 (1초마다 HP -50) | Duration + Period UGameplayEffect |
| 힐 포션 (HP 즉시 +500) | Instant UGameplayEffect |
| 스킬 쿨다운 (3초) | Duration + L"Cooldown.Slash" 태그 부여 |
| 상태이상 해제 | RemoveEffectsWithTag(L"Status") 호출 |
| 장비 스탯 (활: ATK +200) | Infinite 효과, 장비 해제 시 제거 |
| 크리티컬 충전 스택 | MaxStacks=5 Duration 효과 |

완료 기준:
- 독 도트 1초마다 HP 감소 확인 ✅
- 쿨다운 중 재발동 차단 확인 ✅
- 패시브 Infinite 효과 ATK 영구 증가 확인 ✅
- 상태이상 차단 및 해제 확인 ✅
- FMemoryTracker 릭 없음 확인 ✅

---

### Phase 7.5+ — Core 최적화 (LAYER 1 완료 후) ★예정

LAYER 1 (Phase 0~7.5) 전체 검증 완료 후 언리얼 엔진 실제 구조에 맞게 일괄 최적화한다.

**Phase 1 — FMallocBinned (소형 객체 Bin 할당자)** ✅

- 현재: `_aligned_malloc` 래핑 (FMallocAnsi) — 모든 크기를 동일하게 처리
- 목표: 크기 클래스 버킷 방식 — 16 / 32 / 64 / 128 / 256 / 512B 등 Bin 단위 풀링
- 변경 파일: `Engine/Core/Memory/FMallocBinned.h / .cpp`
- 효과: 소형 객체 외부 단편화 제거, 스레드 로컬 캐시로 O(1) 할당

**Phase 3 — TInlineAllocator\<N\>** ✅

- 현재: TArray가 항상 힙 할당
- 목표: 첫 N개 원소를 스택(인스턴스 내부)에 저장 — 초과 시 힙으로 이관
- 변경 파일: `Engine/Core/Containers/TArray.h` (Allocator 템플릿 파라미터 추가)
- 효과: 소형 배열 힙 할당 완전 제거 (예: `TArray<FName, TInlineAllocator<4>>`)

**Phase 4 — TMap / TSet → TSparseArray + 해시 버킷 분리** ✅

- 현재: Open Addressing 선형 프로빙 — Deleted 슬롯 누적, Rehash 비용
- 목표: TSparseArray(연속 메모리) + 해시 버킷 인덱스 체인 (언리얼 실제 구조)
- 변경 파일: `Engine/Core/Containers/TSparseArray.h`, `TMap.h`, `TSet.h`
- 효과: Deleted 슬롯 없음, 반복 캐시 효율 개선, 삭제 후 공간 재사용

**Phase 5 — FNameEntry 인라인 저장 + FNamePool** ✅

- 현재: `TArray<FString>` 선형 탐색 — FString은 별도 힙(`m_pData` 포인터) 보유
- 목표:
  - `FNameEntry { wchar_t Name[NAME_SIZE]; }` — 문자열을 구조체 내부에 인라인 저장
  - `FNamePool`: `TMap<uint32, uint32>` (키 = 문자 내용 djb2 해시, 값 = 엔트리 인덱스)
  - 포인터 기반 해시 문제 원천 제거 (MSVC Debug 버그 재발 불가)
- 변경 파일: `Engine/Core/String/FName.h / .cpp`
- 참고: `FName::ToString()`은 MSVC Debug 인라인 코드생성 버그 회피를 위해
  `FName.cpp`에 `noinline`으로 out-of-line 정의됨 (헤더 전용 구현 금지 원칙과도 일치)

**Phase 6 — TSharedPtr 원자적 참조 카운트** ✅

- 현재: 단순 `int32` 증감 (단일 스레드 한정)
- 목표: `FReferenceControllerBase` — SharedCount + WeakCount 원자적(Atomic) 연산
- 변경 파일: `Engine/Core/SmartPointer/SharedPointerInternals.h`
- 효과: 멀티스레드 안전 공유 소유권 (Phase 16+ 병렬 AI·렌더링 대비)

완료 기준: LAYER 1 단위 테스트 전체 통과 후 최적화 브랜치 별도 생성 — Phase 1·3·4·5·6 ✅ 전부 완료 (Debug/Release 양쪽 전체 86개 테스트 PASSED 확인)

---

### [LAYER 2] Engine Systems — Phase 8~15 (7~14주)

---

### Phase 8 — Renderer (DX11) (1.5주)

파일 위치: `Engine/Include/Render/` (로드맵 문서엔 `Renderer`로 적혀있지만
실제 스캐폴딩된 폴더명은 `Render` — 기존 폴더를 그대로 사용)

- [x] `DXDevice.h / .cpp` — ID3D11Device 초기화 (Debug 레이어 미설치 시
  플래그 없이 재시도하는 폴백 포함)
- [x] `DXSwapChain.h / .cpp` — SwapChain + Present (레거시
  `DXGI_SWAP_CHAIN_DESC` 경로, 리사이즈는 다음 단계로 보류)
- [x] `SpriteBatch.h / .cpp` — DirectXTK 연동 (`SpriteSortMode_Deferred`로
  `FRenderQueue`가 CPU에서 정렬한 순서 그대로 그리도록 함) + 파일 없이
  코드로 텍스처를 만드는 `CreateSolidColorTexture`/`CreateCheckerboardTexture`
  플레이스홀더 헬퍼(Resource Manager/WZ 로딩 이전 임시)
- [x] `RenderQueue.h / .cpp` — Z-Order 정렬 렌더링 (`TArray::StableSort`로
  동일 ZOrder는 제출 순서 유지)
- [x] `FCamera2D.h / .cpp` — 월드↔스크린 좌표 변환, `GetViewMatrix()`
  (플레이어 추적 스크롤은 아직 미구현 — `SetLocation`만 있고 자동 추적
  로직 없음)
- [x] Parallax Scrolling — 배경 원근 스크롤링 (`FRenderQueueEntry::m_ParallaxFactor`
  추가 — 기본 1.0은 카메라와 완전히 같이 움직이는 기존 동작 그대로,
  1보다 작으면 `Flush()`가 `GCamera2D->GetLocation() * (1 - ParallaxFactor)`만큼만
  덜 움직여서 더 멀리 있는 배경처럼 느리게 스크롤됨. 여러 렌더 패스
  없이 기존 `Flush()` 안에서 위치만 보정하는 방식이라 배경 레이어를
  여러 겹(하늘/먼 산/가까운 산 등) 깔아도 한 번에 그려짐)
- [x] 레이어 렌더링 — 배경 / 오브젝트 / 이펙트 / UI (`RenderQueue.h`에
  `ELayer` enum 추가, 정렬 키를 `(Layer, ZOrder)` 2단으로 변경.
  `Flush()`는 월드 좌표 레이어(Background/Object/Effect)만, 신규
  `FlushUI()`는 UI 레이어만 화면 좌표(항등 변환)로 그림 — `main.cpp`가
  프레임마다 `Begin(카메라 행렬)/Flush/End` 다음에
  `Begin()/FlushUI/End`를 추가로 호출)
- [x] 스프라이트 틴트 / 피격 깜빡임 (틴트 파이프라인 자체는 이미
  `SubmitSprite`→`FRenderQueueEntry::m_Tint`→`SpriteBatch::DrawSprite`
  로 다 연결돼 있어서, 시간에 따라 틴트를 바꿔주는 `FHitFlash`
  유틸리티만 추가. `Trigger(Duration, FlashColor)` 이후 매 프레임
  `Update(DeltaTime)` → `GetTint()`가 `FlashColor`에서 `White`로
  서서히 Lerp — DirectXTK 틴트가 곱연산이라 완전한 흰색 실루엣 플래시는
  안 되고 색이 옅어지며 돌아오는 방식만 가능. 아직 게임 루프에 Actor가
  없어서 `UActorComponent`가 아니라 독립 클래스로 만듦 — 나중에 몹
  액터가 생기면 그 컴포넌트가 그대로 갖다 쓰면 됨)
- [x] 데미지 숫자 팝업 렌더링 (`FDamagePopup` — `FHitFlash`와 같은 패턴의
  독립 시간 기반 유틸리티. `Spawn()` 이후 `Update(DeltaTime)`을 거치면
  `GetPosition()`이 위로 떠오르는 좌표를, `GetTint()`가 서서히 투명해지는
  알파를 돌려줌. 여러 개 동시 표시용 풀링은 아직 아무도 안 써서 만들지
  않음, 호출자(나중의 몹 액터) 책임)
  — 실제 숫자 글리프는 `FDamageFont`(신규)가 담당: 처음엔 DirectXTK
  SpriteFont(`.spritefont` 에셋 필요, Phase 14 `UFont.h/.cpp` 몫)로
  미루려 했는데, 사용자가 실제 `Etc.wz/DamageSkin.img` XML을 확인해준
  덕에 몹 프레임 로딩 때와 같은 `_outlink`/`_Canvas` 패턴임을 알게 돼
  이미 검증된 `wz_read_canvas`로 진짜 데미지 숫자 이미지(스킨 0
  기본 "NoRed0" 스타일 0~9)를 그대로 로드하는 쪽으로 바꿈 — 별도 폰트
  에셋/도구 없이 완료. `WzTextureLoader::LoadCanvasTexture`에
  optional Width/Height out 파라미터를 추가해 글리프 폭을 얻고,
  `SubmitNumber()`가 정수를 자릿수로 쪼개 각 글리프의 `origin.y`로
  베이스라인을, 전체 폭 절반만큼 밀어서 가운데 정렬을 맞춰 나란히
  제출한다. 크리티컬 색 분기(`NoCri0` 등 다른 스타일)·다른 스킨 선택은
  Phase 12(UI)에서 게임플레이 훅과 함께 다룰 예정)
- [x] 화면 페이드인·아웃 (맵 이동 연출) (`FScreenFade` — `FadeOut()`/`FadeIn()`
  이후 매 프레임 `Update(DeltaTime)`을 거치면 `GetAlpha()`가 서서히
  변한다. `FHitFlash`/`FDamagePopup`과 달리 `IsActive()`(지금 변하는
  중인지)와 `GetAlpha()`(현재 표시할 값, 페이드 끝나도 도착값 유지)를
  분리 — 화면 페이드는 아웃이 끝나도 다음 인이 호출되기 전까지 계속
  검게 덮여있어야 해서, 렌더링 여부는 `GetAlpha() > 0`으로 판단한다.
  실제 렌더링은 `FSpriteBatch::CreateSolidColorTexture`로 만든 1x1
  단색 텍스처를 화면 크기로 Scale해서 `ELayer::UI` +
  `FScreenFade::SCREEN_FADE_ZORDER`(INT32_MAX로 예약)로 제출 — 다른
  UI 요소가 몰라도 항상 최상단에 그려지도록 구조적으로 보장)

Phase 8 렌더러 항목 전체 완료.

★ WZ 병행 작업 (Phase 8 시작 시, 아직 미착수):
- [x] Canvas → 픽셀 변환 (WzPng) 구현 — **C# DLL 브리지 확장 방식**으로 완료  
  `wz_test.cpp`가 순수 C++ 파서가 아니라 C# Native AOT DLL(`WzNativeLib.dll`,
  `WzComparerR2.WzLib` 재사용)을 `LoadLibraryA`로 부르는 얇은 래퍼임을
  확인하고, 처음부터 C++로 새로 짜는 대신(zlib 벤더링 + BGRA4444/RGB565/
  DXT3·DXT5·BC7 디코더 전부 재구현) 기존 `Wz_Png.ExtractPng()`를 감싸는
  새 export `wz_read_canvas`를 추가하는 쪽을 택했다(WzComparerR2 저장소
  `claude/dx11-2d-engine-fr8yv` 브랜치, `WzExports.cs`). BGRA8888 raw
  픽셀을 네이티브로 넘기면, 엔진 쪽 `Engine/Include/Render/WzTextureLoader.h/.cpp`가
  이를 `ID3D11ShaderResourceView`로 업로드한다(`DXGI_FORMAT_B8G8R8A8_UNORM`,
  `DXDevice`가 이미 `D3D11_CREATE_DEVICE_BGRA_SUPPORT`로 생성돼 있어
  채널 스왑 불필요). `main.cpp`는 실제 WZ Canvas 로드를 먼저 시도하고
  실패하면 체커보드 placeholder로 폴백한다.  
  참고: 이 방식은 CLAUDE.md 하단 "면접 어필 포인트"의 "WZ 파서 직접
  C++ 이식" 항목과는 어긋난다(실제 디코딩은 C# 코드가 수행) — 필요하면
  나중에 순수 C++ 구현(zlib 벤더링 + 포맷별 디코더 이식)으로 교체
  가능하도록 `FWzTextureLoader`의 인터페이스는 그대로 두고 내부 구현만
  바꾸면 되는 구조로 분리해뒀다.  
  **로컬 빌드·배치 필요** (Claude Code가 대신할 수 없음):
  1. `/home/user/WzComparerR2`에서 `WzTest/WzNativeLib` 프로젝트를
     `dotnet publish -r win-x64 -p:NativeLib=Shared -c Release`로 빌드.
  2. 결과물 `WzNativeLib.dll`(`bin/Release/net8.0/win-x64/publish/`)을
     `Game/Bin/`(`MapleStory.exe`와 같은 폴더)에 복사.
  3. `Game/Include/main.cpp`의 `TestWzPath`/`TestCanvasNodePath` 상수를
     로컬에 있는 실제 WZ 파일 경로/Canvas 노드 경로로 수정.
  4. 1차 검증은 엔진 빌드 전에 `wz_test.exe`로 먼저 해볼 수 있음 —
     `wz_test.exe WzNativeLib.dll "<wz경로>" "" "" "<canvas 노드 경로>" canvas.bmp`
     실행 후 생성된 `canvas.bmp`를 이미지 뷰어로 열어 디코딩 결과 확인.
- [x] STL → 엔진 컨테이너 교체 (wz_test.cpp) — **조사 결과 해당 없음으로
  판명, 실제 교체는 하지 않음.** `wz_test.cpp`(`WzComparerR2` 저장소
  `WzTest/wz_test.cpp`)는 자기 자신의 CMake 타겟(`WzTest/CMakeLists.txt`)
  으로만 빌드되는 독립 진단 실행 파일이라 `Engine.lib`/`Game.exe`(이
  엔진의 "STL 금지" 정책이 적용되는 범위)에 전혀 링크되지 않는다.
  실제 STL 사용도 `std::vector`/`std::unordered_map`/`std::unique_ptr`는
  0건이고 `std::string`만 CLI 인자·파일 경로 처리용으로 소량 쓰인다 —
  애초에 정책 적용 대상이 아니었다.

완료 기준: 스프라이트 하나를 화면에 Z-Order 맞게 출력 — **Windows/Visual
Studio 로컬 빌드·시각 검증 완료** (DirectXTK 별도 빌드 등 아래 로컬 환경
설정을 거쳐 정상 출력 확인됨)

**DirectXTK 설치 방식**: NuGet 패키지(`directxtk_desktop_2019`,
`directxtk_desktop_win10`)는 둘 다 deprecated 상태였고, vcpkg는 사용자
환경에서 `vcpkg` 명령어가 PATH에 없어 막혔고, git 서브모듈 + 프로젝트
참조 방식은 솔루션 구조가 복잡해진다는 이유로 보류했다. 최종적으로
**DirectXTK를 솔루션 밖에서 별도로 빌드한 뒤, 그 결과물(.lib)과
헤더만 파일로 가져다 놓는 방식**을 사용한다 — 별도 패키지 관리자도,
서브모듈도, 추가 프로젝트도 없이 순수하게 "미리 빌드된 라이브러리
링크"만 하면 된다. 벤더 폴더(`ThirdParty/DirectXTK/Inc/`,
`ThirdParty/DirectXTK/Lib/`, 각 폴더의 `README.txt`)와
`Engine.vcxproj`/`Game.vcxproj`의 `IncludePath`/`LibraryPath` 설정은
이미 커밋되어 있음 — 아래 빌드·복사만 사용자가 직접 하면 됨.

**DirectXTK.lib 배치 방식**: Debug/Release용 `.lib`를 하위 폴더로
나누지 않고 `ThirdParty/DirectXTK/Lib/` 한 폴더에 파일명으로만
구분해서 넣는다 — `EnginePCH.h`가 `Engine.lib`/`Engine_Debug.lib`로
구분하는 것과 동일한 관례. `SpriteBatch.h`가
`#ifdef _DEBUG`로 `DirectXTK_Debug.lib`/`DirectXTK.lib`를 갈라 링크한다.

**로컬 환경 설정 필요** (Claude Code가 대신할 수 없음 — 실제 빌드는
사용자의 Windows 머신에서만 가능):
1. `https://github.com/microsoft/DirectXTK`를 아무 곳에나 clone(또는
   release zip 다운로드) — 이 저장소 안에 넣을 필요 없음, 빌드 재료일
   뿐.
2. 그 폴더의 `DirectXTK_Desktop_2022.sln`을 Visual Studio로 열어
   Debug|x64로 빌드 → 결과물 `DirectXTK.lib`의 이름을
   **`DirectXTK_Debug.lib`로 바꿔서**
   `ThirdParty/DirectXTK/Lib/DirectXTK_Debug.lib`로 복사.
3. Release|x64로 다시 빌드 → 결과물 `DirectXTK.lib`는 **이름 그대로**
   `ThirdParty/DirectXTK/Lib/DirectXTK.lib`로 복사.
4. DirectXTK 저장소의 `Inc` 폴더 전체를
   `ThirdParty/DirectXTK/Inc/`로 복사(빌드 없이 파일 복사만).
5. Engine, Game 프로젝트의 예외 처리를 `/EHsc`(또는 `/EHa`)로 활성화
   (DirectXTK 헤더가 예외를 쓰므로 필요 — 엔진 자체 코드는 여전히
   `check()`/`verify()`만 사용)
6. `MapleStory.sln`을 Engine → Game 순서로 빌드.
7. d3d11.lib/dxgi.lib는 `DXDevice.h`의 `#pragma comment(lib, ...)`가
   자동 링크하므로 수동 설정 불필요

---

### Phase 9 — Animation 시스템 (1주)

파일 위치: `Engine/Include/Animation/` (다른 Phase와 동일하게 로드맵
문서의 `Engine/Animation/`이 아니라 실제 스캐폴딩된 `Engine/Include/`
하위 경로 사용)

- [x] `UFlipbookComponent.h / .cpp` — 스프라이트 시트(정확히는 지금은
  WZ 아바타 합성 텍스처 시퀀스) 프레임 재생. `UActorComponent` 상속,
  `SetFrames(TArray<FFlipbookFrame>, bLoop)`로 프레임 목록을 받아
  `Tick(DeltaTime)`이 경과 시간에 따라 프레임을 넘기면서 형제
  `USpriteComponent`(`GetOwner()->GetComponent<USpriteComponent>()`로
  `BeginPlay()`에서 캐싱)의 텍스처를 교체한다. `FFlipbookFrame`은
  텍스처(`ID3D11ShaderResourceView*`)+원점+프레임당 표시 시간(초)
  묶음 — 딜레이는 아직 호출자가 지정(아래 "WZ 애니메이션 프레임 딜레이
  → UFlipbookComponent 연동" 참고, 이번엔 미착수).
  **소유권 규칙**: `ID3D11ShaderResourceView`는 COM 레퍼런스 카운트
  객체이고 `USpriteComponent::SetTexture()`는 넘겨받은 레퍼런스를
  소유권째 가져가 다음 교체 시 `Release()`를 소비한다 — 같은 프레임을
  루프마다 반복해서 넘겨야 하므로, `SetFrames()`는 각 텍스처를
  `AddRef()`해서 컴포넌트 수명 동안 자체 보관하고, `Tick()`이 매번
  프레임을 넘기기 직전에 그 프레임 텍스처를 한 번 더 `AddRef()`한다
  (`AddRef` 없이 그대로 넘기면 두 번째 루프에서 이미 해제된 포인터를
  다시 쓰는 use-after-free가 됨).
  `Game/Include/main.cpp`에서 실제 사용 데모: `pPlayerCharacter`에
  `"walk1"` 액션의 프레임을 `FrameIndex` 0부터 실패(`nullptr`)할
  때까지 순차 로드해(`LoadAvatarTexture`가 프레임 개수를 미리 알려주지
  않아서 런타임 추론) `UFlipbookComponent`에 태우고 `Play()` — WZ에
  `walk1` 데이터가 없으면 조용히 폴백(기존 `stand1` 정적 프레임 유지).
- [x] `UAnimStateMachine.h / .cpp` — Idle→Move→Attack→Dead 상태 전환.
  `UFlipbookComponent`의 형제 컴포넌트(`GetComponent<UFlipbookComponent>()`로
  `BeginPlay()`에서 캐싱 — `UFlipbookComponent`가 `USpriteComponent`를
  찾는 것과 동일한 패턴), `TMap<FName, FAnimStateData>`(프레임 배열+루프
  여부)로 상태를 등록해두고 `SetState(FName)` 한 번으로 형제의
  `SetFrames()`+`Play()`를 대신 호출해준다. 같은 상태로 다시
  `SetState()`하면 no-op(매번 애니메이션 재시작 방지), 미등록 이름이면
  `ensure()` 경고 후 무시.
  **소유권 규칙**: `RegisterState()`가 넘겨받은 텍스처를 `AddRef()`해서
  "지금 활성 상태인지와 무관하게, 등록된 모든 상태에 대해 동시에"
  컴포넌트 수명 동안 보관한다 — `SetState()`는 이 보관본을 그대로
  `UFlipbookComponent::SetFrames()`에 넘길 뿐이고, `SetFrames()`가 자기
  예전 프레임을 알아서 `Release()`하므로 "떠나는 상태"를 위한 해제
  로직이 따로 필요 없다.
  **`AddComponent` 순서 주의**: `UFlipbookComponent`를 `UAnimStateMachine`보다
  먼저 붙여야 한다 — 반대 순서면 `UAnimStateMachine::BeginPlay()`
  시점에 형제가 아직 없어서 캐시가 영구히 `nullptr`로 굳는다(이전
  라운드에 고친 "스폰 후 `AddComponent`는 즉시 `BeginPlay()` 호출"
  버그와 맞물리는 지점 — `main.cpp`에 주석으로 명시).
  `main.cpp` 데모: `"Idle"`(stand1 1프레임)/`"Move"`(walk1 시퀀스) 두
  상태를 등록하고, Input(Phase 13)이 아직 없어서 `FTimerManager`로
  2초마다 토글 — Phase 13에서 이 타이머 트리거 블록만 실제 입력으로
  바꾸면 되고 `SetState()` API 자체는 그대로 재사용(걷기 애니메이션
  데모와 마찬가지로 삭제 대상 스모크 테스트가 아니라 영구 데모).
- [x] `UAnimNotify.h / .cpp` — 특정 프레임에 이벤트 발생. 실제 언리얼과
  동일하게 `UObject`를 상속해 `Notify(AActor*)`를 오버라이드하는
  서브클래스 방식(이미 있는 `UClass`/`Cast<T>` 인프라 재사용, Phase 7.7의
  `UGameplayEffect`/`UGameplayAbility`와 같은 선례) — 멀티캐스트 큐나
  AnimInstance 개념 없이 가상 함수 하나뿐(소비할 전투/피격 판정
  시스템이 아직 없어서 그 이상은 투기적 코드). `FFlipbookFrame`에
  비소유 `UAnimNotify* m_pNotify` 필드 추가, `UFlipbookComponent::Tick()`이
  프레임 전환 `while` 루프에서 새 프레임으로 넘어가는 시점(루프
  랩어라운드 포함)마다 정확히 한 번 `Notify()`를 호출 — 같은 프레임에
  머무르는 틱에서는 `while` 조건 자체가 거짓이라 재호출 없음. 이번
  세션 데모에는 등록한 알림이 없음(콤보/공격 판정 등 실제 소비처가
  생기면 그때 서브클래스를 만들어 씀) — 인프라만 갖춰둠.
- [x] `USpriteComponent` 좌우 반전(FlipHorizontal) ★추가 — Phase 8/Renderer
  소속이지만 "오른쪽으로 걸어갈 때 오른쪽을 봐야 하는데 지금은 항상
  왼쪽만 보고 있다"는 질문으로 이번에 같이 처리. 조사 결과 WZ 아바타
  데이터는 한쪽 방향만 원본으로 갖고 있고(`ActionFrame.Flip`은 좌우
  방향 스위치가 아니라 같은 프레임 안 파츠 재사용용 저작 힌트 —
  `AvatarCanvas`에 "반대 방향으로 그려줘" 스위치 자체가 없음, 조사
  완료), 좌우 반전은 원래부터 렌더러 몫이라는 게 확정됐다.
  `SpriteBatch::DrawSprite`가 DirectXTK `Draw()`를 항상 `Scale` 벡터를
  그대로 흘려보내며 호출하고 있어서(`SpriteBatch.cpp`), 별도 배관 없이
  `USpriteComponent`에 `bool m_bFlipHorizontal`+`SetFlipHorizontal(bool)`만
  추가하면 됐다. 단, `Render()`가 DirectXTK 쪽 Origin을 항상 `(0,0)`으로
  고정해서 피벗 보정을 Position 쪽에서 미리 하고 있었기 때문에(`Location - m_Origin`),
  단순히 `Scale.X`만 `-1`로 뒤집으면 피벗이 어긋나 반전할 때마다 캐릭터가
  옆으로 튀는 버그가 된다 — 반전 시엔 `Position.X = Location.X + Origin.X`
  (빼기 대신 더하기)로 보정 방향도 같이 뒤집어야 피벗이 월드 좌표에
  고정된다(`USpriteComponent.cpp` 주석에 유도 과정 기록). `ACharacter::SetFacingRight(bool)`가
  이 위로 얇게 얹힘 — 실제 이동 방향에 따라 호출하는 건 Phase 13(Input)
  몫, 지금은 `main.cpp`의 Idle/Move 타이머 데모가 같이 토글해서 육안
  확인.
- [ ] 스프라이트 시트 JSON 파싱 (TexturePacker 포맷)
- [ ] 애니메이션 블렌딩 (이동 중 공격 전환)
- [ ] 역방향 재생 (Reverse)
- [ ] 스킬 이펙트 애니메이션 클립

★ WZ 병행 작업:
- [ ] FrameAnimator (딜레이 기반 프레임 전환) — 지금은 `UFlipbookComponent`가
  이 역할을 겸하고 있어서 별도 클래스는 미착수(아래 항목으로 충분히
  커버됨).
- [x] WZ 애니메이션 프레임 딜레이 → UFlipbookComponent 연동 —
  `wz_read_avatar`(`WzTest/WzNativeLib/WzExports.cs`)에 `int* outDelayMs`
  출력 파라미터를 추가했다. `AvatarCanvas.CreateFrame()`이 내부적으로
  쓰는 `ActionFrame`(딜레이 포함)은 그 메서드 밖으로 안 나오므로,
  별도로 `AvatarCanvas.GetActionFrames(actionName)`(`AvatarCanvas.cs:667`,
  액션 전체 프레임을 다시 훑어서 각각의 WZ `"delay"` 프로퍼티를
  `LoadActionFrameDesc`로 채워 돌려주는 기존 public 메서드 — 새로
  만들 필요 없이 그대로 재사용)를 호출해 `[frameIndex].AbsoluteDelay`
  (`ActionFrame.cs:22-26`, 음수 delay 값을 `Math.Abs`로 정규화한 것)를
  꺼내 넘긴다. 프레임 인덱스가 범위를 벗어나면 WZ 쪽 기본 폴백과 동일한
  120ms를 유지.
  C++ 쪽 `FAvatarTexture`(`WzTextureLoader.h`)에 `int32 m_DelayMs = 120`
  필드 추가, `main.cpp`의 walk1 로딩 루프가 하드코딩했던 `0.15f` 대신
  `Frame.m_DelayMs / 1000.0f`를 그대로 `FFlipbookFrame::m_Duration`에
  사용하도록 교체. `UFlipbookComponent::SetFrames()`에는 방어 코드
  추가 — WZ 딜레이가 0 이하로 들어오면 `Tick()`의 프레임 전환
  `while` 루프가 절대 안 끝나는 무한 루프가 되므로, 그런 경우만
  화면에 안 티 나는 최소값(0.001초)으로 클램프.
  **로컬 재빌드 필요**: DLL export 시그니처가 바뀌었으므로
  `WzNativeLib.dll`을 다시 `dotnet publish`해서 `Game/Bin/`에
  교체해야 함 — Phase 8에 적어둔 빌드 절차와 동일.

완료 기준: 캐릭터 이동 시 Walk 애니메이션 자동 전환 — `UFlipbookComponent`
자체는 완료(위 데모로 프레임 순환 재생 확인 가능), "이동 시"(Input
연동)는 Phase 13(입력 시스템)이 아직 없어서 이번 범위 밖 — 지금은
스폰 직후부터 계속 반복 재생하는 상태로 대체 검증.

**버그 노트 — `AddComponent<T>()`가 스폰 후 추가된 컴포넌트에
`BeginPlay()`를 안 부르던 문제**: 위 데모를 처음 붙였을 때 캐릭터가
전혀 안 걸었다. 원인은 `AActor::AddComponent<T>()`(`Engine/Include/Object/AActor.h`)가
컴포넌트를 만들기만 하고 `BeginPlay()`는 안 불러준다는 것 —
`ACharacter` 생성자에서 붙는 `USpriteComponent`는 `SpawnActor`가
액터 생성 직후 한 번 돌리는 `BeginPlay()` 일괄 전파(`AActor::BeginPlay()`)를
받지만, `UFlipbookComponent`처럼 그 이후(스폰 뒤)에
`AddComponent<T>()`로 추가된 컴포넌트는 그 일괄 전파를 받을 기회 자체가
없어서 `BeginPlay()`가 영원히 안 불렸다 — `UFlipbookComponent::BeginPlay()`가
캐싱해야 할 `m_pTargetSprite`가 계속 `nullptr`이라 `Tick()`이 내부
프레임 인덱스는 넘기면서도 실제 `SetTexture()`는 절대 안 불렀던 것.
`AActor`에 `bool m_bHasBegunPlay`를 추가해서, `AddComponent<T>()`가
액터가 이미 `BeginPlay`를 마친 상태면 새 컴포넌트에 즉시
`BeginPlay()`를 호출해주도록 수정(생성자 시점에 붙는 컴포넌트는
기존 일괄 전파 경로 그대로라 이중 호출 없음). 스폰 이후에 컴포넌트를
런타임으로 붙이는 모든 미래 호출 지점(Phase 10 피격 반응 컴포넌트,
Phase 18 몬스터 AI 컴포넌트 등)에 공통으로 영향 가는 엔진 공용 API
버그였어서 `main.cpp` 개별 호출 지점이 아니라 `AActor.h/.cpp`에서
근본 수정.

**버그 노트 — 아바타 파츠 레이어 순서(z/ZIndex)가 실제로는 항상 깨져서
그려지고 있던 문제 (WzComparerR2 저장소)**: 무기가 팔 앞에 그려져야
할지 뒤에 그려져야 할지가 스윙 계열 액션에서 항상 반대로 나온다는
리포트로 발견. `WzTest/WzNativeLib/WzExports.cs`의
`canvas.LoadZ(root.FindNodeByPath(@"Base\zmap.img"))` 호출이
`extractImage` 인자를 안 넘겨서(기본값 `false`) `zmap.img`의 자식
노드(z-순서 이름 목록)가 하나도 파싱 안 된 채로 넘어가고 있었다 —
노드 자체는 `null`이 아니라서 `AvatarCanvas.LoadZ()`는 성공(`true`)을
반환하지만, 실제로는 `this.ZMap`이 빈 리스트로 남는다.
`AvatarCanvas.GenerateLayer()`가 각 파츠의 문자열 `Skin.Z` 값을
`ZMap.IndexOf()`로 찾는데 `ZMap`이 비어있으면 전부 못 찾아서(`-1`)
거의 모든 문자열 Z 레이어가 같은 `ZIndex`로 뭉개지고, 그 상태로
정렬하면 사실상 원래 삽입 순서에 가깝게 무너진다 — 이게 파츠별
레이어 순서가 깨지는 증상의 근본 원인이었다. 아바타 Body/Head/Face/Hair
파츠 로딩에서 이미 한 번 겪었던 것과 정확히 같은 클래스의 버그
(`FindNodeByPath`의 `extractImage` 기본값이 `false`라 `.img` 경계에서
내부 트리가 안 열림)인데, 그때는 `zmap.img` 쪽을 놓쳤다.
`root.FindNodeByPath(@"Base\zmap.img", true)`로 수정(WzComparerR2
`claude/dx11-2d-engine-fr8yv` 브랜치, 커밋 `a7e356e`) — **역시 DLL
재빌드 필요**.

**후속 — 위 수정만으로는 부족했음, 근본 원인 최종 확정**: `extractImage`
수정 후 재빌드해도 무기가 여전히 프레임에 상관없이 항상 같은 자리에
그려진다는 재보고를 받아 진단 로그(`wz_avatar_debug.log`, 무기
`Skin.Z`/`ZIndex`/`ZMap` 크기를 매 호출마다 기록)를 추가해서 원인을
더 파봤다. 로그 결과 `ZMap.Count=0`이 여전히 100% 재현됐는데, 이번엔
`"Base\zmap.img"`라는 **경로 자체**가 이 브리지 구조와 안 맞았을
가능성을 의심했다 — 실제 WzComparerR2 GUI의
`PluginManager.FindWz("Base\\...")`는 `"Base"`를 트리 안 폴더가
아니라 개별 .wz 파일이 등록된 레지스트리 키로 취급하는데, 우리
`PluginManagerShim`은 경로 전체를 `CurrentRoot` 하나의 트리 안 폴더
경로로 취급하기 때문에, KMST 병합 WZ 구조에서는 안 맞을 수 있었다.
`"Base\zmap.img"`가 실패하면 접두사 없이 `"zmap.img"`로 재시도하는
폴백 + 정밀 진단 로그를 추가(커밋 `731a685`)한 뒤 사용자가 재빌드·
재실행해서 받은 로그로 **최종 확정**: `triedPath=zmap.img (Base\
접두사 실패 후 폴백) ... ZMap.Count=184`, `resolvedZMapIndex`가
`swingT3` 프레임마다 실제로 다름(68/81/106). 즉 `"Base\zmap.img"`는
이 사용자의 KMST 병합 WZ 구조(원본 파일명 폴더 계층 없이 `zmap.img`가
루트에 바로 있는 형태)에서 **애초에 틀린 경로**였고, `extractImage`
수정은 필요조건이었지만 그것만으로는 부족했다 — 접두사 없는 폴백이
진짜 결정타. 화면에서도 스윙 프레임에 따라 무기가 팔 앞/뒤로 정상
전환되는 것까지 사용자가 육안 확인 완료.

---

### Phase 10 — Physics / Collision (1주)

파일 위치: `Engine/Physics/`

- [ ] `UBoxCollision.h / .cpp` — AABB 충돌
- [ ] `UCircleCollision.h / .cpp` — Circle 충돌
- [ ] `PhysicsWorld.h / .cpp` — 충돌 감지 루프
- [ ] `URigidbody.h / .cpp` — 중력, 속도
- [ ] Raycast
- [ ] 충돌 레이어 마스크 (플레이어/적/지형/투사체)
- [ ] 플랫폼 판정 — 땅·벽·천장 (메이플 핵심)
- [ ] 경사면 처리
- [ ] 로프·사다리 충돌 영역
- [ ] 낙하 판정 / 코요테 타임 (점프 관용치)
- [ ] 무적 프레임 (i-frame, 피격 후 무적)
- [ ] 넉백 물리 (보스 스킬 피격)
- [ ] 낙사 구역 (DeathZone)

★ WZ 병행 작업:
- [ ] Map.wz Foothold 데이터 파싱 → PhysicsWorld 충돌 데이터 연동

완료 기준: 캐릭터가 발판 위에 서고 벽에 막힘 확인

---

### Phase 11 — Audio 시스템 (4~5일)

파일 위치: `Engine/Audio/`

- [ ] `FAudioManager.h / .cpp` — XAudio2 래핑
- [ ] `UAudioComponent.h / .cpp`
- [ ] BGM 루프 재생 + 페이드인·아웃
- [ ] SFX 중첩 재생
- [ ] 사운드 풀 (동시 재생 한도, 타격음 대량 발생 대응)
- [ ] 볼륨 / 피치 실시간 제어
- [ ] 거리 기반 감쇠 (2D 포지셔닝)

완료 기준: 맵 이동 시 BGM 페이드 전환 확인

---

### Phase 12 — UI / HUD 시스템 (1.5주)

파일 위치: `Engine/UI/`

- [ ] `UWidget.h / .cpp` — UI 베이스
- [ ] `UCanvas.h / .cpp` — HUD
- [ ] `UTextBlock.h / .cpp` / `UImage.h / .cpp`
- [ ] UI 앵커 / 레이아웃 시스템 (해상도 대응)
- [ ] HP / MP 바 / 경험치 바
- [ ] 퀵슬롯 (스킬바 F1~F8)
- [ ] 인벤토리 창 (96칸 그리드) / 장비 창 / 스탯 창
- [ ] 데미지 숫자 팝업 (크리티컬 노란색)
- [ ] 버프 아이콘 목록 HUD / 미니맵
- [ ] NPC 대화 말풍선 / 채팅 창 / 퀘스트 트래커
- [ ] 이름표 / 레벨 표시 (캐릭터 머리 위)

완료 기준: HP바 실시간 감소 확인

---

### Phase 13 — Input 시스템 (5일) ★수정

파일 위치: `Engine/Input/`

#### 기본 구조
- [ ] `InputSystem.h / .cpp` — 키보드·마우스 입력 메인
- [ ] `InputAction.h / .cpp` — Enhanced Input 스타일 바인딩
- [ ] `InputMapping.h / .cpp` — 키 → 액션 매핑 테이블
- [ ] 키 리맵핑 설정 (설정 창 연동)
- [ ] 입력 버퍼링 (스킬 선입력 처리)
- [ ] 콤보 입력 감지 (↓↓ 스킬 등)

#### 수식 키 (Modifier Key) 처리

**ALT (VK_MENU)**  
ALT는 `WM_SYSKEYDOWN`으로 수신됨. Windows 기본 동작 차단 필요.

```
ALT 단독     → 메뉴바 활성화 (SC_KEYMENU) — 반드시 차단
ALT + F4     → 창 종료 — 정책 결정 필요
ALT + Enter  → DirectX 전체화면 전환 — 반드시 차단
ALT + Tab    → 창 전환 — 포커스 로스트 처리로 대응
ALT + Space  → 창 시스템 메뉴 — 차단 권장
```

- [ ] `WM_SYSKEYDOWN` / `WM_SYSKEYUP` WndProc 처리
- [ ] `SC_KEYMENU` ALT 단독 메뉴바 차단
- [ ] ALT + Enter 전체화면 전환 차단
- [ ] `VK_LMENU` / `VK_RMENU` 좌우 ALT 구분

**CTRL (VK_CONTROL)**
- [ ] `VK_LCONTROL` / `VK_RCONTROL` 좌우 구분
- [ ] 채팅 창 활성 상태일 때 CTRL 조합 선택적 허용

**SHIFT (VK_SHIFT)**
- [ ] `VK_LSHIFT` / `VK_RSHIFT` 좌우 구분
- [ ] `GetKeyState()` 폴링과 메시지 방식 병행 사용

**포커스 로스트 대응**
- [ ] `WM_KILLFOCUS` → 전체 키 상태 초기화
- [ ] `WM_SETFOCUS` → `GetKeyState` 폴링으로 재동기화

```cpp
struct FModifierKeyState {
    bool bLeftAlt = false;  bool bRightAlt = false;
    bool bLeftCtrl = false; bool bRightCtrl = false;
    bool bLeftShift = false; bool bRightShift = false;
    bool IsAltDown()   const { return bLeftAlt   || bRightAlt;   }
    bool IsCtrlDown()  const { return bLeftCtrl  || bRightCtrl;  }
    bool IsShiftDown() const { return bLeftShift || bRightShift; }
};
```

완료 기준: ALT 점프 입력 시 메뉴바 활성화 없이 정상 동작, ALT+Tab 후 복귀 시 키 상태 정상 초기화

---

### Phase 14 — Resource Manager (4일)

파일 위치: `Engine/Resource/`

- [ ] `FResourceManager.h / .cpp` — 경로 기반 로드 + 캐싱
- [ ] `UTexture.h / .cpp` — DDS / PNG 지원
- [ ] `UFont.h / .cpp` — 비트맵 폰트·한글 지원
- [ ] `USoundWave.h / .cpp`
- [ ] 참조 카운트 자동 해제 / 비동기 로드 / 리소스 패키지 (pak)

★ WZ 병행:
- [ ] `FWzResourceProvider.h / .cpp` — WZ를 Resource Manager 소스로 등록

완료 기준: 같은 텍스처 두 번 로드 시 캐시 히트 확인

---

### Phase 15 — World / Level / GameMode (5일)

파일 위치: `Engine/World/`

- [x] `UWorld.h / .cpp` — **게임 루프 프레임워크 부분만 앞당겨 완료**
  (아래 "게임 루프 프레임워크 + 게임 오브젝트 그릇" 메모 참고). 스폰된
  액터를 `TArray<AActor*>`로 들고 있다가 `Tick(DeltaTime)`/`Render(FRenderQueue&)`
  한 번 호출로 전부 순회·전파하는 최소 컨테이너 — 언리얼의 `SpawnActor<T>()`처럼
  `AActor::AddComponent<T>()`와 동일한 Malloc+placement-new 패턴으로
  `SpawnActor<T>()`를 제공하고, 스폰 직후 `BeginPlay()`까지 자동 호출.
  `FindActorById(uint32)`는 지금 쓰는 곳은 없지만 나중에 네트워크
  리플리케이션을 붙일 때 "메시지 ID → 로컬 액터" 역참조 자리로 미리
  만들어둠(`AActor::GetActorId()`도 같은 이유로 Phase 7의 `AActor`에
  이번에 추가됨, 순증 카운터).
- [ ] `ULevel.h / .cpp` / `UGameMode.h / .cpp` / `UGameInstance.h / .cpp` —
  아직 미착수(멀티 레벨 전환, 세이브 연동 등은 지금 필요 없어서 보류)
- [ ] 타일맵 로더 / 포털 시스템 / 스폰 포인트 / 낙사 구역

★ WZ 병행:
- [ ] Map.wz → MapData 파싱 → ULevel 타일맵 로더 연결

완료 기준: JSON 맵 파일 로드 후 타일 렌더링 확인

---

### 게임 루프 프레임워크(UWorld) + 게임 오브젝트 그릇(ACharacter/USpriteComponent) ★선행 완료

Phase 8(렌더러)에서 만든 것들(`SpriteBatch`/`RenderQueue`/`WzTextureLoader`/
아바타 합성)이 전부 `main.cpp`가 전역 변수로 직접 호출하는 테스트
코드였고, Phase 7에서 만들어둔 `AActor`/`UActorComponent`/`USceneComponent`
프레임워크는 실제로 쓰이지 않고 있었다. Phase 16(캐릭터) 본 작업(스탯/
스킬/인벤토리)을 통째로 앞당기는 대신, 지금 있는 렌더링을 담을 최소한의
"그릇"만 먼저 만들었다:

- `USpriteComponent`(`Engine/Include/Render/`) — `USceneComponent` 상속,
  텍스처 하나(`ID3D11ShaderResourceView*`, 소유)+Origin/ZOrder/Layer/Tint/
  ParallaxFactor를 들고 있다가 `Render(FRenderQueue&)`가
  `GetWorldTransform().m_Location - Origin`으로 발밑 정렬해서 제출한다.
  `Cast<USpriteComponent>(comp)`(Phase 7 완료 기준에서 이미 예시로 든
  이름)로 실제 존재하는 클래스가 됨.
- `ACharacter`(`Engine/Include/Object/`) — `AActor` 상속, 생성자에서
  `AddComponent<USpriteComponent>()`. `LoadAvatar(...)`가
  `FWzTextureLoader::LoadAvatarTexture(...)` 결과를 그 컴포넌트에 싣는다.
- `UActorComponent`/`AActor`에 `virtual void Render(FRenderQueue&)` 가상
  함수 추가 — `Tick()`과 같은 전파 패턴(`AActor::Render`가 `m_Components`를
  순회). `Object/` 폴더가 `Render/` 폴더(d3d11.h 등)를 몰라도 되도록
  `class FRenderQueue;` 전방 선언만 사용.
- `main.cpp`는 이제 `pWorld->SpawnActor<ACharacter>()`로 캐릭터를 만들고,
  매 프레임 `pWorld->Tick(DeltaTime)` / `pWorld->Render(*pRenderQueue)`
  두 줄이면 끝 — 앞으로 몬스터/이펙트 액터가 늘어나도 `main.cpp`를 더
  안 건드리고 `pWorld->SpawnActor<T>()`만 호출하면 됨.

**`ACharacter`는 플레이어/몬스터/NPC 공통 베이스로 설계됐다** — 언리얼도
`ACharacter` 자체는 "누가 조종하든 상관없는" 범용 캐릭터 껍데기이고,
플레이어냐 몬스터냐는 어떤 `AController`(`APlayerController`/
`AAIController`)가 빙의(Possess)하는지로 갈린다(조종 주체와 물리적
실체를 별개 클래스 축으로 분리). 지금 엔진엔 Controller/Input
시스템(Phase 13)이 아직 없어서 그 구분이 필요 없으므로, 나중에
플레이어 전용 서브클래스(`APlayerCharacter`)나 몬스터 전용 서브클래스
(`AMonster`)가 필요해지는 시점은 각각 Phase 13(입력)과 Phase 18(AI)
— 그 전까지는 `ACharacter`를 그대로 상속(또는 직접 사용)해서 쓴다.
`AActor`→`APawn`→`ACharacter` 3단 구조 중 `APawn` 계층(Controller가
빙의할 수 있다는 개념)도 지금은 `ACharacter`가 겸하고 있고, Controller
개념이 실제로 필요해지는 Phase 13에서 `AActor`와 `ACharacter` 사이에
끼워 넣을 예정 — 지금 미리 만들지 않음(투기적 코드 방지).

---

### [LAYER 3] Gameplay Framework — Phase 16~22 (15~22주)

Phase 16 캐릭터, Phase 17 스킬, Phase 18 몬스터/AI, Phase 19 인벤토리,  
Phase 20 퀘스트/NPC, Phase 21 Save/Load, Phase 22 디자인 패턴

(위 "게임 루프 프레임워크 + 게임 오브젝트 그릇"이 `ACharacter`라는
이름과 렌더링 컴포넌트 하나만 앞당겨 만든 것 — 스탯/스킬/인벤토리 등
Phase 16 본 작업은 아직 미착수.)

---

### [LAYER 4] 이펙트·마무리 — Phase 23~24

Phase 23 파티클/이펙트, Phase 24 소셜/마무리

★ 최종 목표: 메이플 기본 플레이 루프 완성 (포트폴리오 완성)

---

## WZ 파서 통합 계획

기준 브랜치: `claude/convert-wz-parser-cpp-cdJ2V`  
작업 디렉토리: `/home/user/WzComparerR2`  
C++ 구현 위치: `WzTest/wz_test.cpp`

### 현재 완료 상태
- 1단계 — WZ 헤더 + 디렉토리 트리 파싱 ✅
- 2단계 — IMG 노드 내부 파싱 ✅

### WZ 작업 병행 타임라인

| 엔진 Phase | WZ 작업 |
|-----------|---------|
| Phase 7.5 완료 후 | STL → 엔진 컨테이너 일괄 교체 |
| Phase 8 Renderer | Canvas → 픽셀 변환 (WzPng), DirectXTex 연동 |
| Phase 9 Animation | FrameAnimator → UFlipbookComponent 연동 |
| Phase 10 Physics | Foothold 데이터 → PhysicsWorld 연동 |
| Phase 14 Resource | WzNode → 엔진 타입 변환, FWzResourceProvider 통합 |
| Phase 15 World | MapData 파싱 → ULevel 타일맵 로더 연결 |

---

## 주차별 계획

| 주차 | Phase | 작업 내용 |
|------|-------|----------|
| 1주 | 0 | VS 솔루션 생성, 폴더 구조, EnginePCH.h, 첫 커밋 |
| 1~2주 | 1·2 | Memory + TypeTraits |
| 2~3주 | 3 | TArray 완성 + 단위 테스트 |
| 3~4주 | 3.5 | 수학 라이브러리 |
| 4~5주 | 4·5 | TMap·TSet + FString·FName |
| 5~6주 | 5.5·6·7 | 로그 + SmartPtr + UObject |
| 6~7주 | 7.5·7.7 | Timer + Gameplay Ability System |
| 7~8주 | 8·9 | Renderer + Animation (WZ Canvas 변환 병행) |
| 8~9주 | 10·11 | Physics + Audio (WZ Foothold 병행) |
| 10~12주 | 12~15 | UI + Input + Resource + World (WZ 통합) |
| 13~15주 | 16·17 | 캐릭터 + 스킬 |
| 15~18주 | 18~24 | 몬스터·인벤·퀘스트·세이브·이펙트 |

---

## Claude Code 작업 지침

### Git 운영 규칙
- **main 브랜치 커밋·머지는 사용자가 직접 수행**
- Claude는 작업 브랜치(`claude/dx11-2d-engine-fr8yv`)에만 커밋·푸시
- 사용자가 main에 푸시 후 알리면 → `git fetch origin main && git merge origin/main` 으로 작업 브랜치 최신화

### 코딩 규칙
- **클래스는 헤더(.h)와 소스(.cpp)를 반드시 함께 생성** — 헤더 전용 구현 금지
- **멤버 변수에 `m_` 접두사 필수** (예: `m_Size`, `m_pData`)

### 파일 생성 요청 방식
```
"Engine/Core/Memory/IAllocator.h 작성해줘.
STL 사용 금지, C++17, /GR- /EHs-c- 옵션 기준.
EnginePCH.h가 PCH로 포함되어 있어."
```

### Phase 완료 체크 방식
각 Phase 완료 시 `Test` 프로젝트에서 반드시 단위 테스트 실행 후 다음 단계 진행.  
빌드 오류 발생 시 오류 메시지 전체를 Claude Code에 붙여넣기.

### 설계 이슈 발생 시
구조 결정·리뷰·누락 확인은 claude.ai 채팅에서 상담 후 진행.  
Claude Code는 파일 생성·빌드 오류 수정·리팩터링 전담.

---

## 면접 어필 포인트

- placement new 활용한 TArray
- 커스텀 얼로케이터 (GMalloc → IAllocator 교체 가능 구조)
- POD 분기 최적화 (if constexpr + TIsPOD)
- FName O(1) 비교 (uint32 인덱스)
- UClass 기반 Cast<T>() — RTTI 없이 동작
- Gameplay Ability System 직접 구현 — 태그·속성·효과·스킬 계층 (언리얼 GAS 패턴)
- WZ 파서 직접 C++ 이식 (AES 복호화, 분할 파일 merge)
