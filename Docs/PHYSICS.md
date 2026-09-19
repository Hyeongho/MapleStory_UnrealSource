# Phase 10 — 첫 단계 Physics/Collision

`CLAUDE.md`의 Phase 순서를 유지한다. Input 선행 구현 없이 위치·속도·시간을 직접 지정해 검증한다.

## 구조

- `USceneComponent → UPrimitiveComponent → UBoxCollision / UCircleCollision`
- `UActorComponent → URigidbody`: 속도·중력 배율·최대 낙하 속도·Grounded 상태.
- `UWorld`가 `FPhysicsWorld`를 소유하며 Actor Tick 이후 물리를 한 번 갱신한다.
- `AActor::GetComponents<T>`로 월드 안의 도형을 수집한다. 영구 포인터 등록부가 없으므로
  삭제된 Actor/Component가 다음 쿼리에 남지 않는다. 전역 `GWorld`에 의존하지 않는다.

언리얼의 컴포넌트 계층과 월드 소유 구조를 참고한 최소 2D 구현이다.
Chaos 또는 언리얼의 전체 Primitive/BodyInstance/MovementComponent 구현을 복제한 것은 아니다.

## 이번 단계에서 지원하는 것

- Box–Box, Circle–Circle, Box–Circle 겹침 검사(경계 접촉 포함).
- 양쪽 도형의 채널 마스크를 모두 만족해야 겹침/정적 차단 발생.
- ObjectMask·IgnoreActor를 받는 유한 선분 Raycast. 가장 가까운 Box/Circle 반환.
  시작점이 도형 내부이면 Time=0, StartPenetrating=true, Normal=0이다.
  Time은 초가 아닌 선분 내 비율이며 실패 시 OutHit을 초기화한다.
- 동적 Box와 정적 Box 사이의 중력·속도·발판 착지·벽/천장 차단 및 벽면 미끄러짐.
- swept AABB로 빠른 이동의 관통 방지. 초기 겹침은 최대 8회 최소 이동 보정.
- 1/120초를 목표로 최대 120회 서브스텝, 시간 버림 없음. 음수/0/비유한 DeltaTime 무시.
- 정지 접촉과 발판 이탈을 반영한 Grounded, 최대 낙하 속도 제한.

## 계약과 제한

- 화면 좌표계: X 오른쪽, Y 아래쪽. 중력은 양의 Y 방향이다.
- Box extent는 반너비/반높이이며 Circle은 반지름을 사용한다.
- 도형은 월드 XY 축에 정렬되어 있다. 회전은 무시하고 스케일과 CenterOffset은 적용한다.
  Circle의 비균일 스케일은 더 큰 절댓값으로 반지름을 확대한다.
- Actor당 하나의 URigidbody와 첫 번째 **부모 없는 UBoxCollision**을 시뮬레이션한다.
  도형이 없거나 비활성/부모 부착 상태이면 시뮬레이션하지 않는다.
- 동적 Actor끼리의 물리 반응 및 Circle의 Rigidbody 차단은 이번 단계에 포함하지 않는다.
  Circle은 겹침/Raycast 쿼리용이다. URigidbody가 없거나 시뮬레이션이 꺼진 Box는 정적으로 취급한다.
- Trigger 채널의 정적 도형은 Rigidbody를 막지 않으며 겹침 조회에 사용할 수 있다.
- `FindOverlaps`는 현재 겹침 목록만 반환한다. Begin/End 이벤트나 피해 처리는 아직 없다.
- Hit/Overlap의 Component 포인터는 비소유이며 Actor/Component 삭제 후 사용하면 안 된다.
- 최소 이동 보정은 정상적인 발판·벽 배치를 위한 것이다. 완전히 끼인/상충하는 지형의 해결은 보장하지 않는다.
- 현재는 선형 쿼리와 도형 쌍 검사다. 공간 분할, 회전 강체, 마찰, 질량/반발 충돌은 아직 없다.

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

2026-09-19 검증: Windows x64 Debug/Release 전체 솔루션 빌드 및 Test 실행 통과,
Physics 검사 실패 0건, 프로세스 종료 코드 0. `Copy.bat`으로 갱신한 Physics 공개 헤더
6개의 원본/배포 미러 해시 일치 확인. 기존 DirectXTK PDB 누락 링크 경고는 남아 있다.

다음은 같은 Phase 10 안의 선분 Foothold·경사면·단방향 발판 및 나머지 항목이다.
Map.wz 파싱, 로프/사다리, 코요테 타임, i-frame, 넉백, DeathZone은 아직 미구현이다.
현재 Game 데모를 물리 데모로 바꾸지는 않았으며 화면에서의 플레이 검증은 별도다.
