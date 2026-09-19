# Phase 10 — Physics/Collision

`CLAUDE.md`의 Phase 순서를 유지한다. Input 선행 구현 없이 위치·속도·시간을 직접 지정해 검증한다.

## 구조

- `USceneComponent → UPrimitiveComponent → UBoxCollision / UCircleCollision`
- `UActorComponent → URigidbody`: 속도·중력 배율·최대 낙하 속도·Grounded 상태.
- `UWorld`가 `FPhysicsWorld`를 소유하며 Actor Tick 이후 물리를 한 번 갱신한다.
- `AActor::GetComponents<T>`로 월드 안의 도형을 수집한다. 영구 포인터 등록부가 없으므로
  삭제된 Actor/Component가 다음 쿼리에 남지 않는다. 전역 `GWorld`에 의존하지 않는다.

언리얼의 컴포넌트 계층과 월드 소유 구조를 참고한 최소 2D 구현이다.
Chaos 또는 언리얼의 전체 Primitive/BodyInstance/MovementComponent 구현을 복제한 것은 아니다.

## 기본 도형과 강체

- Box는 월드 회전을 반영하는 OBB다. `FOrientedBox2D`가 도형 계산을 담당한다.
- Box–Box는 양쪽 도형의 네 축을 검사하는 SAT(분리축 정리), Box–Circle은
  회전 좌표계의 최근접점, Circle–Circle은 중심 거리로 겹침을 검사한다(경계 접촉 포함).
- 양쪽 도형의 채널 마스크를 모두 만족해야 겹침/정적 차단 발생.
- ObjectMask·IgnoreActor를 받는 유한 선분 Raycast. 가장 가까운 Box/Circle/발판 반환.
  시작점이 도형 내부이면 Time=0, StartPenetrating=true, Normal=0이다.
  Time은 초가 아닌 선분 내 비율이며 실패 시 OutHit을 초기화한다.
- 동적 Box와 정적 Box 사이의 중력·속도·발판 착지·벽/천장 차단 및 벽면 미끄러짐.
- 회전을 고정한 swept OBB로 빠른 평행 이동의 관통 방지. 초기 겹침은 SAT의
  최소 침투 축을 따라 최대 8회 보정한다. 충돌 후 접선 방향 속도를 보존한다.
- 1/120초를 목표로 최대 120회 서브스텝, 시간 버림 없음. 음수/0/비유한 DeltaTime 무시.
- 정지 접촉과 발판 이탈을 반영한 Grounded, 최대 낙하 속도 제한.
  정적 Box는 접촉 법선 Y < -0.5인 면을 지면으로 취급하며, 경사면을 따라 위로
  이동해도 면에서 떨어지는 법선 속도가 없으면 접지를 유지한다.

## 선분 발판과 경사면

- `FFoothold`는 ID와 두 끝점, 충돌 마스크를 가진 월드 좌표 데이터다.
  `FPhysicsWorld`가 복사해 소유하고 `AddFoothold` / `RemoveFoothold` / `ClearFootholds`로 관리한다.
  중복 ID, 음수 ID, 비유한 좌표, 수직·퇴화 선분은 등록하지 않는다.
- Box의 **월드 아래쪽 지지점**으로 착지한다. 회전이 없으면 아래 면 중앙이며,
  회전했으면 가장 아래쪽 꼭짓점이다(수평인 면은 중앙을 선택한다).
  내려오면서 선분을 가로지를 때 착지하며, 위로 점프하거나 아래에서 접근하면 통과한다.
  기존 단일 지점 발판 모델을 유지하므로 Box 전체 모서리로 경사면을 지지하지 않는다.
- 접지 중에는 수평 속도를 유지하고 발밑 높이를 경사면에 맞춘다. 끝점을 공유하는
  선분 사이를 이동하며, 연결된 선분이 없으면 중력을 적용해 떨어진다.
  한 서브스텝의 이동·충돌 반복은 최대 16회이며, 초과한 잔여 이동은 처리하지 않는다.
- 발판은 `WorldStatic` 채널이다. Box와 발판의 충돌 마스크를 모두 만족해야 착지한다.
  `URigidbody::GetCurrentFootholdId()`는 현재 지지하는 선분 ID를 반환한다.
  공중이나 정적 Box 위에서는 `INDEX_NONE`이다. 지지 발판 삭제 시 접지 정보도 해제한다.
- Raycast는 발판의 양쪽 방향에서 검출한다. 발판 적중 시 `FHitResult::m_FootholdId`를
  사용하며 `m_pComponent`는 null이다. 쿼리는 ObjectMask를 적용하며 발판의 응답 마스크와는 별개다.
  `FindOverlaps`는 컴포넌트 도형끼리의 조회로 유지한다.
- 끝점 순서는 무관하다. 연결 판정은 끝점 위치에 기반하며 WZ의 prev/next·레이어 정보는
  아직 사용하지 않는다. 수직 선분 벽은 정적 Box로 표현한다.

```cpp
FPhysicsWorld& Physics = World.GetPhysicsWorld();
const bool bAdded = Physics.AddFoothold(
    FFoothold(1, FVector2D(0, 100), FVector2D(200, 50)));
check(bAdded);
// Box와 URigidbody를 가진 액터가 위에서 내려오면 경사면에 착지한다.
// 접지 후 수평 속도를 설정하면 경사면의 높이를 따라 움직인다.
```

`FindFoothold`의 반환 포인터는 발판 목록을 변경하기 전까지만 사용한다.
이동 발판, 아래로 내려가기 입력, WZ 파싱·연결 정보 복원은 후속 범위다.

## 로프·사다리 영역

- `UClimbableComponent`는 `UBoxCollision`을 상속한 Trigger 영역이다. 기본 크기는
  반너비 6, 반높이 32이며 기본 충돌 대상은 Player다. Rope/Ladder 종류는 현재
  이동 규칙이 같고, 이후 맵 데이터·표현에서 구분할 수 있다.
- Player Box가 영역과 겹친 상태에서 `URigidbody::SetClimbInput()`에 음수(위) 또는
  양수(아래)를 주면 오르기를 시작한다. 입력 범위는 -1~1로 제한한다.
  입력 0은 영역 안에서 정지, `StopClimbing()`은 오르기 종료와 중력 복귀를 뜻한다.
- 오르는 동안 기존 수평·낙하 속도는 버리고, 중력과 단방향 Foothold 충돌은 적용하지 않는다.
  정적 Box의 벽·천장 충돌은 유지한다. 영역 밖으로 나가거나 영역 Actor가 삭제되거나
  충돌 마스크가 더는 맞지 않으면 오르기를 종료한다.
- 영역이 겹치기만 해서는 자동으로 매달리지 않는다. 별도의 Input System 없이
  이동 방향을 직접 설정할 수 있다. X 위치를 로프 중심으로 자동 정렬하지 않는다.
- 회전된 영역의 겹침도 OBB로 검사한다. 오르기 이동 방향은 기존처럼 월드 Y축이다.

```cpp
AActor* LadderActor = World.SpawnActor<AActor>();
UClimbableComponent* Ladder = LadderActor->AddComponent<UClimbableComponent>();
Ladder->SetBoxExtent(FVector2D(6.0f, 50.0f));

// Player Box와 Rigidbody가 있는 액터에서 오르기 명령을 준다.
Body->SetClimbSpeed(100.0f);
Body->SetClimbInput(-1.0f); // 위로 이동
World.Tick(0.1f);
Body->SetClimbInput(0.0f);  // 영역 안에서 정지
Body->StopClimbing();       // 오르기 종료
```

영역의 Begin/End 이벤트, 로프 중심 정렬, 애니메이션 전환 및 WZ 로프·사다리 데이터 연결은
후속 범위다. 이 단계의 소스는 사용자 요청에 따라 빌드·테스트하지 않았다.

## 계약과 제한

- 화면 좌표계: X 오른쪽, Y 아래쪽. 중력은 양의 Y 방향이다.
- Box extent는 반너비/반높이이며 Circle은 반지름을 사용한다.
- Box의 `FTransform2D::m_Rotation`은 라디안이며 회전·스케일을 충돌에 반영한다.
  `CenterOffset`에도 스케일 → 회전 → 이동을 적용한다. 음수 스케일을 지원한다.
  `GetWorldBox()`가 실제 OBB, `GetWorldBounds()`가 이를 감싸는 외접 AABB를 반환한다.
  Circle의 비균일 스케일은 더 큰 절댓값으로 반지름을 확대한다.
- Sweep 중에는 회전과 크기가 고정된다. 각속도·토크·회전 도중의 연속 충돌은 지원하지 않는다.
  외부에서 회전을 바꾸면 다음 쿼리/시뮬레이션은 변경된 자세를 사용한다.
- Actor당 하나의 URigidbody와 첫 번째 **부모 없는 UBoxCollision**을 시뮬레이션한다.
  도형이 없거나 비활성/부모 부착 상태이면 시뮬레이션하지 않는다.
- 동적 Actor끼리의 물리 반응 및 Circle의 Rigidbody 차단은 이번 단계에 포함하지 않는다.
  Circle은 겹침/Raycast 쿼리용이다. URigidbody가 없거나 시뮬레이션이 꺼진 Box는 정적으로 취급한다.
- Trigger 채널의 정적 도형은 Rigidbody를 막지 않으며 겹침 조회에 사용할 수 있다.
- `FindOverlaps`는 현재 겹침 목록만 반환한다. Begin/End 이벤트나 피해 처리는 아직 없다.
- Hit/Overlap의 Component 포인터는 비소유이며 Actor/Component 삭제 후 사용하면 안 된다.
- 최소 이동 보정은 정상적인 발판·벽 배치를 위한 것이다. 완전히 끼인/상충하는 지형의 해결은 보장하지 않는다.
- 현재는 선형 쿼리와 도형 쌍 검사다. 공간 분할, 회전 운동, 마찰, 질량/반발 충돌은 아직 없다.

## 사용 예

```cpp
UWorld World;
AActor* FloorActor = World.SpawnActor<AActor>();
UBoxCollision* Floor = FloorActor->AddComponent<UBoxCollision>();
Floor->SetBoxExtent(FVector2D(100.0f, 2.0f));
Floor->SetRelativeTransform(FTransform2D(FVector2D(0, 100), 0, FVector2D::One));

AActor* Player = World.SpawnActor<AActor>();
UBoxCollision* Box = Player->AddComponent<UBoxCollision>();
Box->SetBoxExtent(FVector2D(5, 5));
Box->SetCollisionObjectType(ECollisionChannel::Player);
URigidbody* Body = Player->AddComponent<URigidbody>();

// 시각 컴포넌트가 있으면 Box에 부착해 물리 위치를 따라가게 한다.
// 이후 위치 변경은 Box의 transform으로 한다. 부착된 자식의 위치를 따로 바꾸지 않는다.
World.Tick(0.5f);
// Box 중심 Y=93: 발밑 98이 Floor 상단 98에 닿는다.
```

## 검증 및 다음 범위

`Test/Include/main.cpp`의 Phase 10 블록에서 기존 check 규칙으로 검사한다.
Debug는 실패 시 중단하고, Release는 실패 수를 집계해 Test 종료 코드에 반영한다.
얇은 발판 고속 착지, 접촉 유지, 위로 이동, 발판 이탈, 속도 제한, 벽 미끄러짐,
천장 차단, 초기 겹침 해소, 마스크, 쿼리, 삭제 후 조회를 검증한다.

첫 단계의 기존 검증 기록(2026-09-19): Windows x64 Debug/Release 전체 솔루션 빌드 및 Test 실행 통과,
Physics 검사 실패 0건, 프로세스 종료 코드 0. `Copy.bat`으로 갱신한 Physics 공개 헤더
6개의 원본/배포 미러 해시 일치 확인. 기존 DirectXTK PDB 누락 링크 경고는 남아 있다.

선분 발판·경사면 코드는 Codex가 빌드·테스트하지 않았고, 이후 사용자가 동작을 확인했다.
`main()`에 단방향 통과, 고속 착지, 양방향 경사 이동, 연결 지점 통과, 발판 이탈·삭제,
등록 거부, 마스크 및 발판 Raycast 검사 코드를 추가했으며 사용자 실행으로 확인해야 한다.
위의 첫 단계 통과 기록은 이번 변경의 검증 결과가 아니다.

OBB 전환 검사도 `main()`에 추가했다: 회전·음수 스케일·중심 오프셋, 외접 AABB 오탐 제거,
Box–Circle, 회전 Raycast, 서로 다른 회전의 고속 충돌, 경사면 접지·이탈,
초기 침투, 회전 Box의 Foothold 착지와 회전 사다리 영역을 다룬다.
OBB 변경 후 빌드·테스트는 사용자 요청에 따라 실행하지 않았다.

다음은 같은 Phase 10의 코요테 타임 및 나머지 항목이다.
Map.wz 파싱, 코요테 타임, i-frame, 넉백, DeathZone은 아직 미구현이다.
현재 Game 데모를 물리 데모로 바꾸지는 않았으며 화면에서의 플레이 검증은 별도다.
