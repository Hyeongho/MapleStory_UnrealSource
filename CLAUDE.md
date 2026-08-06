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
- [x] `GetDeltaTime()` / `GetTimeSeconds()` 전역 접근

완료 기준: 3초 뒤 콜백 정확히 호출 확인 ✅

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
- [ ] Parallax Scrolling — 배경 원근 스크롤링
- [ ] 레이어 렌더링 — 배경 / 오브젝트 / 이펙트 / UI
- [ ] 스프라이트 틴트 / 피격 깜빡임
- [ ] 데미지 숫자 팝업 렌더링
- [ ] 화면 페이드인·아웃 (맵 이동 연출)

★ WZ 병행 작업 (Phase 8 시작 시, 아직 미착수):
- [ ] Canvas → 픽셀 변환 (WzPng) 구현  
  BGRA4444 / BGRA8888 / BC3·BC7 압축 해제 → ID3D11Texture2D 업로드
- [ ] STL → 엔진 컨테이너 교체 (wz_test.cpp)  
  std::vector → TArray / std::string → FString / std::unordered_map → TMap / std::unique_ptr → TSharedPtr

완료 기준: 스프라이트 하나를 화면에 Z-Order 맞게 출력 — 코드 작성 완료,
**Windows/Visual Studio에서 사용자 빌드·시각 검증 대기 중** (git
서브모듈로 받은 DirectXTK 초기화 등 로컬 환경 설정 필요, 아래 참고)

**DirectXTK 설치 방식**: NuGet 패키지(`directxtk_desktop_2019`,
`directxtk_desktop_win10`)는 둘 다 deprecated 상태였고, vcpkg는 사용자
환경에서 `vcpkg` 명령어가 PATH에 없어 막혔다. 최종적으로 **git
서브모듈 + 프로젝트 참조** 방식을 사용한다 — DirectXTK 저장소
(`https://github.com/microsoft/DirectXTK`)를 솔루션 루트에 서브모듈로
추가하고, 그 안의 `DirectXTK_Desktop_2022.vcxproj`를 `MapleStory.sln`의
4번째 프로젝트로 등록해 Engine·Game이 `ProjectReference`로 참조한다.
NuGet도 vcpkg 도구 설치도 필요 없음 — Visual Studio가 솔루션을 빌드할
때 DirectXTK도 함께 빌드해서 결과물을 자동으로 링크한다. `.gitmodules`,
`MapleStory.sln`의 4번째 프로젝트 항목, `Engine.vcxproj`/
`Game.vcxproj`의 `ProjectReference`·`IncludePath` 설정은 이미 커밋되어
있음 — 아래 서브모듈 초기화만 사용자가 직접 하면 됨.

**로컬 환경 설정 필요** (Claude Code가 대신할 수 없음 — 서브모듈
초기화는 사용자의 로컬 clone에서만 가능):
1. 이 브랜치를 받은 뒤 서브모듈 내용을 내려받음(최초 1회, 또는 새로
   clone한 경우):
   ```
   git submodule update --init --recursive
   ```
2. Visual Studio에서 `MapleStory.sln`을 열어 "DirectXTK" 프로젝트가
   솔루션 탐색기에 정상적으로 보이는지 확인(02. Engine 폴더 아래 위치)
3. Engine, Game 프로젝트의 예외 처리를 `/EHsc`(또는 `/EHa`)로 활성화
   (DirectXTK 내부가 예외를 던지므로 필요 — 엔진 자체 코드는 여전히
   `check()`/`verify()`만 사용)
4. 빌드 — DirectXTK 프로젝트가 먼저 빌드되고(`ProjectReference`로
   빌드 순서·자동 링크가 보장됨), 그다음 Engine, Game 순서로 진행됨.
   **DirectXTK는 처음엔 소스부터 컴파일되므로 첫 빌드는 평소보다 오래
   걸릴 수 있음**
5. d3d11.lib/dxgi.lib는 `DXDevice.h`의 `#pragma comment(lib, ...)`가
   자동 링크하므로 수동 설정 불필요

---

### Phase 9 — Animation 시스템 (1주)

파일 위치: `Engine/Animation/`

- [ ] `UFlipbookComponent.h / .cpp` — 스프라이트 시트 프레임 재생
- [ ] `UAnimStateMachine.h / .cpp` — Idle→Move→Attack→Dead 상태 전환
- [ ] `UAnimNotify.h / .cpp` — 특정 프레임에 이벤트 발생
- [ ] 스프라이트 시트 JSON 파싱 (TexturePacker 포맷)
- [ ] 애니메이션 블렌딩 (이동 중 공격 전환)
- [ ] 역방향 재생 (Reverse)
- [ ] 스킬 이펙트 애니메이션 클립

★ WZ 병행 작업:
- [ ] FrameAnimator (딜레이 기반 프레임 전환)
- [ ] WZ 애니메이션 프레임 딜레이 → UFlipbookComponent 연동

완료 기준: 캐릭터 이동 시 Walk 애니메이션 자동 전환

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

- [ ] `UWorld.h / .cpp` / `ULevel.h / .cpp` / `UGameMode.h / .cpp` / `UGameInstance.h / .cpp`
- [ ] 타일맵 로더 / 포털 시스템 / 스폰 포인트 / 낙사 구역

★ WZ 병행:
- [ ] Map.wz → MapData 파싱 → ULevel 타일맵 로더 연결

완료 기준: JSON 맵 파일 로드 후 타일 렌더링 확인

---

### [LAYER 3] Gameplay Framework — Phase 16~22 (15~22주)

Phase 16 캐릭터, Phase 17 스킬, Phase 18 몬스터/AI, Phase 19 인벤토리,  
Phase 20 퀘스트/NPC, Phase 21 Save/Load, Phase 22 디자인 패턴

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
