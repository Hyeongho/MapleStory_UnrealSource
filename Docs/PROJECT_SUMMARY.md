# MapleStory DX11 2D 엔진 — 프로젝트 진행 정리

> 언리얼 엔진 아키텍처를 STL/예외/RTTI 없이 C++17로 직접 재구현하는 포트폴리오 엔진.
> 목표 게임: MapleStory 스타일 2D 플랫포머 RPG.

---

## 1. 프로젝트 제약과 목표

| 제약 | 내용 | 이유 |
|---|---|---|
| STL 금지 | std::vector/string/unordered_map 등 전부 자체 구현 | 컨테이너/할당자 내부 동작의 완전한 이해 증명 |
| 예외 금지 | `/EHs-c-`, check() 매크로 + TResult<T,E>로 대체 | 게임 엔진 관례 (언리얼도 예외 비활성) |
| RTTI 금지 | `/GR-`, UClass 기반 Cast<T>() 직접 구현 | dynamic_cast 없이 타입 시스템 구축 |
| 네이밍 | TArray, TMap, FString, FName, UObject, AActor | 언리얼 컨벤션 준수 |
| 검증 | Phase마다 단위 테스트 후 다음 단계 진행 | 현재 86개 테스트, Debug/Release 양쪽 통과 |

---

## 2. 완료된 시스템 (LAYER 1: Phase 0 ~ 7.7 + Core 최적화 5종)

### 2.1 Memory (Phase 1 → 최적화 Phase 1)

- `IAllocator` 인터페이스 + 전역 `GMalloc` 포인터 + `FMemory` 정적 진입점 — 언리얼과 동일한 할당자 교체 가능 구조
- 전역 `operator new/delete` 오버라이드(+C++14 sized delete)로 모든 할당이 GMalloc 경유
- **FMallocBinned**: 16~512B 크기 클래스 Bin 풀링. 64KB 정렬 페이지에서 블록을 잘라 쓰고,
  `ptr & ~(PAGE_SIZE-1)` 포인터 마스킹만으로 페이지 헤더를 O(1) 역추적 → 블록별 헤더 오버헤드 0
- 대형/과잉정렬 할당은 페이지 단위로 폴스루
- 부가: FPoolAllocator(몬스터/파티클 대량 생성), FStackAllocator(프레임 임시), FMemoryTracker(Debug 릭 감지)

### 2.2 Containers (Phase 3~4 → 최적화 Phase 3·4)

- **TArray\<T, AllocatorType\>**: placement new + 명시적 소멸자, `if constexpr` POD memcpy 분기, ×2 성장.
  **TInlineAllocator\<N\>** — 첫 N개를 인스턴스 내부(스택)에 저장, 초과 시 힙 이관, Shrink 시 복귀.
  N=0일 땐 빈 베이스 클래스(EBO)라 기존 TArray 크기 불변
- **TSparseArray\<T\>**: 원소가 수명 내내 안정적인 int32 인덱스 유지. 삭제 슬롯은 침습적 프리 리스트로
  연결되어 다음 Add가 재사용 — 톰스톤 없음. 성장 시 인덱스 보존 재배치
- **TSet/TMap**: 원소는 TSparseArray에 저장, pow2 버킷 배열(int32)이 해시→첫 원소 인덱스,
  충돌은 원소 내 `m_HashNext` 인덱스 체인 — **언리얼 실제 내부 구조와 동일한 알고리즘**.
  삭제 = 체인 unlink + 슬롯 반납 O(1), Rehash는 원소 이동 없이 버킷 재링크만
- TMultiMap(TMap<K, TArray<V>> 래퍼), TArrayView(비소유 슬라이스)

### 2.3 String (Phase 5 → 최적화 Phase 5)

- **FString**: wchar_t 동적 버퍼, Printf/Format/Split/파싱, 전체 연산자
- **FName + FNamePool**: 문자열 인터닝. `FNameEntry{wchar_t[64]}` 인라인 저장 +
  djb2 콘텐츠 해시 → 인덱스 맵(TMultiMap 체인 + wcscmp로 충돌 안전).
  비교는 uint32 인덱스 비교 = O(1). 등록/검색 O(1)
- FText: 다국어 지원 래퍼 (언리얼 3종 문자열 체계)

### 2.4 SmartPointer (Phase 6 → 최적화 Phase 6)

- **FRefCountBlock**: SharedCount + WeakCount 분리 (UE `FReferenceControllerBase` 대응).
  객체는 SharedCount==0에 파괴, 블록은 WeakCount==0에 해제 — 수명 이원화
- **암묵적 weak 참조 패턴**: WeakCount를 1로 초기화(공유 그룹 몫) — 순환 참조 파괴 중
  블록이 먼저 해제되는 이중 해제를 원천 차단
- 원자적 카운트: MSVC `_Interlocked*` / GCC `__atomic` 컴파일러 분기 래퍼
- TSharedPtr / TWeakPtr(IsValid/Pin) / TSharedRef(null 불가) / MakeShared

### 2.5 Object 시스템 (Phase 7)

- **UClass**: FName + SuperClass 포인터 + IsChildOf 체인
- **DECLARE_CLASS 매크로**: 함수-지역 정적 UClass(`StaticClass()`) + 가상 `GetClass()` +
  `using Super` — 언리얼 GENERATED_BODY가 생성하는 코드의 최소 골격
- **Cast\<T\>/CastChecked/ExactCast**: RTTI 없이 IsChildOf 체인으로 다운캐스트
- TSubclassOf(타입 안전 클래스 레퍼런스), AActor의 AddComponent\<T\>/GetComponent\<T\>

### 2.6 Timer (Phase 7.5)

- FTimerManager: SetTimer(루프/1회)/ClearTimer/Pause/Resume/SetTimerNextFrame — 언리얼 API 표면 동일

### 2.7 Logging / Assert (Phase 5.5)

- UE_LOG(카테고리, 레벨, 포맷) + DEFINE_LOG_CATEGORY + 파일 로깅
- **assert 정책** (언리얼 Shipping 관례 채택):
  - `check(expr)` — Debug 전용. Release(NDEBUG)에서는 표현식 자체가 평가 안 됨 → 부수효과 금지
  - `verify(expr)` — 항상 평가. Debug에선 실패 시 assert, Release에선 평가만
  - `ensure(expr)` — 한 번만 발화, 값 반환으로 if문에서 사용 가능
- TResult<T,E> — 예외 없는 에러 전파

### 2.8 Gameplay Ability System (Phase 7.7)

- FGameplayTag 계층 태그(`L"Skill.Attack.Slash"`, MatchesParent) + FGameplayTagContainer
- UAttributeSet(TMap<FName, FGameplayAttribute>, Base/Current + Min/Max 클램프)
- UGameplayEffect: Instant/Duration/Infinite + Period(도트) + MaxStacks + Granted/Blocked 태그
- UGameplayAbility: Cost/Cooldown 이펙트 + ActivationBlockedTags + CanActivate/Activate/End
- UAbilitySystemComponent: Apply/RemoveEffect, Grant/TryActivateAbility, Tick(만료/도트), LooseTag

MapleStory 패턴 매핑: 패시브=Infinite, 독 도트=Duration+Period, 포션=Instant,
쿨다운=Duration+태그, 장비 스탯=Infinite(해제 시 제거), 스택 버프=MaxStacks

---

## 3. 언리얼 실제 구조와의 대조 검증 결과

전 서브시스템을 UE5 실제 소스 구조와 대조 검증함 (◎ 구조 동일 / ○ 의도적 축소 / △ 설계 차이):

| 서브시스템 | 판정 | 핵심 근거 |
|---|---|---|
| TArray + TInlineAllocator | ◎ | 할당자 정책 파라미터, 인라인 스필 — UE ContainerAllocationPolicies와 동일 원리 |
| TSparseArray | ◎ | 프리 리스트 인덱스 재사용 = UE FreeListLink. (UE는 비트 배열, 저희는 슬롯 내 bool) |
| TSet/TMap | ◎ | 원소 내 체인(m_HashNext=UE HashNextId) + 버킷 배열 = UE Set.h 알고리즘 |
| FName/FNamePool | ◎ | 인라인 엔트리 + 풀 + 인덱스 비교 = UE NameTypes 핵심 |
| FRefCountBlock | ◎ | UE FReferenceControllerBase(ThreadSafe)와 동일 설계 |
| DECLARE_CLASS/Cast | ◎ | GENERATED_BODY 최소 골격 + IsChildOf 캐스트 |
| FMallocBinned | ○ | Bin 풀링 + 페이지 마스킹 일치. UE는 스레드 캐시 등 추가 |
| GAS | ○ | 개념 계층 전부 대응. 예측/네트워킹 미구현(싱글플레이 목표) |
| ensure/UE_LOG/Timer | ○ | 의미론 일치, 내부 단순화 |
| TMultiMap | △ | UE는 셋에 중복 키 저장, 저희는 TMap<K,TArray<V>> 래퍼 |

**알려진 정합성 메모** (인지하고 있으며 필요 시 정리 예정):
1. `BeginPlay/Tick/EndPlay`가 UObject에 선언됨 — 실제 UE는 AActor/UActorComponent 전용
2. `ensure`가 Release에서도 `__debugbreak()` 실행 — UE Shipping은 평가만 하고 브레이크 없음

---

## 4. 트러블슈팅 기록 (실전 디버깅 사례)

### 사례 1 — TSharedPtr 순환 참조 Release 힙 손상 (0xC0000374)

- **증상**: Debug 통과, Release에서만 순환 참조 테스트가 힙 손상으로 사망
- **원인**: 순환 참조 파괴 시 Deleter 체인 안에서 TWeakPtr가 FRefCountBlock을 먼저 해제 →
  바깥 TSharedPtr가 해제된 블록을 읽고 다시 해제 (이중 해제).
  Debug는 해제 메모리를 0xDD로 채워 우연히 감춰졌고, Release에서만 드러남
- **해결**: std::shared_ptr와 동일한 **암묵적 weak 참조 패턴** — WeakCount를 1로 초기화하고
  마지막 Shared 해제 시에만 그 몫을 반납. 블록은 양쪽 카운트가 모두 0일 때만 해제

### 사례 2 — MSVC Debug 인라인 코드생성 버그 (FName::ToString)

- **증상**: `FString R = Name.ToString();`이 Debug에서 접근 위반. 복사 원본의 m_pData만
  null인데 m_Length는 유효 — "이미 소멸자가 돈 임시 객체" 모양
- **진단 과정**: RTC 끄기 → 재현 / 할당자 교체 → 재현 / 함수 내부 재구성 → 재현.
  세 번의 변경에도 크래시가 1비트도 안 변함 + 콜스택에 ToString 프레임이 한 번도 없음
  → **컴파일러가 /Od에서도 이 초소형 함수를 인라인해 소스 수정이 반영 안 되고 있었음**
- **해결**: ToString을 .cpp로 분리 + `__declspec(noinline)` — 진짜 함수 호출 경계(x64 ABI
  hidden-pointer 반환)를 강제하자 즉시 해결. "헤더 전용 구현 금지" 규칙의 실증 사례
- **교훈**: 코드를 바꿨는데 증상이 완전히 동일하면 "내 수정이 생성 코드에 반영되고 있는가"부터 의심

### 사례 3 — Release에서 check() 안의 부수효과 증발 (0xC0000005)

- **증상**: Release에서만 `Sparse.RemoveAt(1)`이 null 근처 주소(0x14) 쓰기 위반
- **원인**: `check(Sparse.Add(100) == 0)` — Release(NDEBUG)에서 assert는 표현식을
  **아예 컴파일에서 제거** → Add가 전부 증발한 빈 배열에 RemoveAt 실행.
  0x14 = null 베이스 + 슬롯1의 m_NextFree 오프셋과 정확히 일치
- **해결**: 상태 변경은 독립 문장으로, check는 순수 검사만. verify는 Release에서도
  평가되도록 재정의. 검증 파이프라인에 NDEBUG(Release 등가) 구성 상시 추가
- **교훈**: assert 계열 매크로에 부수효과를 넣지 말 것 — 구성에 따라 코드가 사라진다

---

## 5. 검증 방법론

- **단위 테스트 86개** — Phase마다 작성, 회귀 겸용 (컨테이너 처닝, 순환 참조,
  멀티스레드 참조 카운트 스트레스(CreateThread 4스레드×20만회) 포함)
- **양쪽 구성 검증**: Windows Debug + Release 모두 통과 확인
- **교차 플랫폼 검증**: Linux g++로 3구성(O0 / O2 / O2+NDEBUG) 빌드·실행 +
  valgrind memcheck (누수/이중해제/미초기화 접근 0건) — MSVC와 다른 컴파일러/힙으로
  이식성·메모리 안전성 이중 확인

---

## 6. 다음 로드맵

- **Phase 8 — Renderer (DX11)**: DXDevice/SwapChain/SpriteBatch/RenderQueue/FCamera2D,
  WZ Canvas→텍스처 병행
- Phase 9 Animation → 10 Physics(Foothold) → 11 Audio → 12 UI → 13 Input → 14 Resource → 15 World
- LAYER 3: 캐릭터/스킬(GAS 활용)/몬스터 AI/인벤토리/퀘스트/세이브
