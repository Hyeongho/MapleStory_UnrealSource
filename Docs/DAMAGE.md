# Game 캐릭터와 피격 후 무적

## 역할

- `Game/Include/Character/AMapleCharacter.h/.cpp`: 체력, 피해 처리, 무적 상태, 깜빡임, 아바타 로딩과 이동 상태별 애니메이션 선택.
- `Game/Include/GameInstance/UMapleGameInstance.h/.cpp`: 맵·게임 캐릭터 생성 및 시작 위치 설정. 물리 갱신 뒤 캐릭터의 UpdateAnimation을 호출한다.
- `Game/Include/Controller/AMaplePlayerController.h/.cpp`: 조작과 Debug 피격 확인 입력.
- `ACharacter`, `URigidbody`: 기존 Engine의 이동·발판·점프·로프·사다리 처리.

체력은 `UAttributeSet`의 Health 속성에 보관하고, TakeDamage는 기존 Ability System의 즉시 GameplayEffect로 감소시킨다.
외부에서 들어오는 게임 피해는 `AMapleCharacter::TakeDamage`를 사용한다. Ability System에 직접 효과를 적용하면 이 함수의 무적 판정을 거치지 않는다.
HP가 0이면 IsDead가 참이고 추가 피해를 받지 않는다. 사망 모션·입력 차단·리스폰은 후속 작업이다.

## 기본 설정과 API

기본 최대 체력은 100, 피격 후 무적 시간은 1초, 깜빡임의 각 단계 간격은 0.1초다.
피격 중에는 기존 틴트의 RGB 밝기를 65%로 낮추는 회색 틴트와 원래 색상을 번갈아 적용한다.
컴포넌트 알파와 기존 틴트 알파, WZ 프레임 a0/a1는 유지한다.
무적 시간은 캐릭터 Tick에서 경과 시간만큼 감소한다. 만료·체력 초기화·EndPlay에서 원래 틴트를 복원한다.
타이머 콜백에 캐릭터 포인터를 보관하지 않는다.

```cpp
Character->SetMaxHealth(150.0f); // 현재 체력은 새 최대값 이하로 보정한다.
Character->ResetHealth();       // 새 최대 체력까지 회복하고 무적·깜빡임을 해제한다.
Character->SetInvulnerabilityDuration(1.0f);
Character->SetDamageBlinkInterval(0.1f);

const float AppliedDamage = Character->TakeDamage(10.0f, DamageCauser);
```

TakeDamage는 실제 감소한 체력을 반환한다. 0 이하·비정상 피해량, 무적 상태와 HP 0에서는 0을 반환한다.
무적 중 거부한 피격은 남은 시간을 초기화하거나 연장하지 않는다.
무적 시간 설정은 다음 피격부터 적용하며 0이면 피격 후 무적을 사용하지 않는다.
같은 애니메이션이 루프를 돌거나 다른 상태로 바뀌어도 깜빡임을 위한 텍스처 재로딩·참조 갱신은 없다.

## 실제 클라이언트 확인

Debug 구성에서만 다음 입력을 등록한다. Release에도 피해 처리 API는 유지한다.

| 키 | 동작 |
|---|---|
| F1 | 현재 조종 중인 AMapleCharacter에 피해 10 적용 |
| F2 | 체력을 최대값으로 초기화하고 무적·깜빡임 해제 |

1. F1을 누르면 콘솔에 `[Damage] applied`와 HP·무적 시간이 표시되고 캐릭터가 깜빡인다.
2. 1초 이내에 다시 누르면 `[Damage] ignored`가 표시되고 HP가 추가로 감소하지 않는다.
3. 무적이 끝나면 `[Damage] invulnerability ended`가 표시되고 원래 색상으로 복원된다.
4. 이후 F1으로 다시 피해를 받을 수 있으며 F2는 무적 중에도 체력과 표시를 초기화한다.
5. 이동·점프·로프·사다리 및 맵 끝에서의 제자리 걷기가 기존대로 동작하는지 확인한다.

## 검사 소스

현재 브랜치의 `Test/Include/main.cpp`는 기존 충돌·OBB·발판·로프·사다리와 공유 Flipbook·맵 경계 검사를 보존한다.
main에 빠졌던 검사 복원을 위해 이 파일 전체와 Test 프로젝트 설정을 함께 반영해야 한다.
Test 프로젝트는 Game 캐릭터를 검사하기 위해 `Game/Include/Character/AMapleCharacter.cpp`를 함께 컴파일한다.

추가 검사 범위: 첫 피해, 무적 중 연속 피해 거부, 만료, 큰 DeltaTime, 잘못된 입력, 최대 체력 보정,
무적 시간 0, HP 0, 무적 중 체력 초기화, 종료 시 색상 복원, 회색 틴트와 기존 투명도 보존.
이번 변경의 빌드와 테스트는 실행하지 않았다.
