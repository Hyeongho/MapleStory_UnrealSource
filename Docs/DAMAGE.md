# Game 캐릭터와 피격 후 무적

## 역할

- `Game/Include/Character/AMapleCharacter.h/.cpp`: 체력, 피해 처리, 무적 상태, 깜빡임, 아바타 로딩과 이동 상태별 애니메이션 선택.
- `Game/Include/GameInstance/UMapleGameInstance.h/.cpp`: 맵·게임 캐릭터 생성 및 시작 위치 설정. 물리 갱신 뒤 캐릭터의 UpdateAnimation을 호출한다.
- `Game/Include/Controller/AMaplePlayerController.h/.cpp`: 조작과 Debug 피격 확인 입력.
- `ACharacter`, `URigidbody`: 기존 Engine의 이동·발판·점프·로프·사다리 처리.

체력은 `UAttributeSet`의 Health 속성에 보관하고, TakeDamage는 기존 Ability System의 즉시 GameplayEffect로 감소시킨다.
즉시 효과는 기본값을 변경한 뒤 활성 Add → Multiply → Override 효과를 재적용해 현재값을 계산한다.
주기 효과도 기본값을 변경하며 해당 Tick의 효과 처리 종료 시 현재값을 재계산한다.
예를 들어 기본 체력 100에 Multiply -0.5가 적용되어 현재 체력이 50이면,
피해 10은 기본 체력을 90으로 바꾸고 현재 체력을 45로 만든다. 실제 감소량 5를 반환하며 무적이 시작된다.
피해량은 보정된 현재 체력으로 미리 제한하지 않는다. 기본값의 상한·하한은 속성의 기존 Clamp 규칙을 따른다.
ResetHealth도 SetAttributeBaseValue를 통해 기본 체력을 최대값으로 초기화하고 활성 보정을 재적용한다.
따라서 최대 체력이 100이고 Multiply -0.5가 유지 중이면 초기화 후 현재 체력은 50이다.
SetMaxHealth는 기존 기본 체력을 새 상한 안으로 보정하며, 현재 체력을 기본값으로 재사용하지 않는다.
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
Character->ResetHealth();       // 기본 체력을 최대값으로 회복하고 활성 보정은 유지한다. 무적·깜빡임은 해제한다.
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
| F2 | 기본 체력을 최대값으로 초기화하고 활성 보정 유지, 무적·깜빡임 해제 |

1. F1을 누르면 콘솔에 `[Damage] applied`와 HP·무적 시간이 표시되고 캐릭터가 깜빡인다.
2. 1초 이내에 다시 누르면 `[Damage] ignored`가 표시되고 HP가 추가로 감소하지 않는다.
3. 무적이 끝나면 `[Damage] invulnerability ended`가 표시되고 원래 색상으로 복원된다.
4. 이후 F1으로 다시 피해를 받을 수 있으며 F2는 무적 중에도 체력과 표시를 초기화한다.
5. 이동·점프·로프·사다리 및 맵 끝에서의 제자리 걷기가 기존대로 동작하는지 확인한다.

## 검증 방식

사용자 방침에 따라 Test 프로젝트는 더 이상 사용하지 않으며 해당 프로젝트의 코드·설정을 추가하거나 수정하지 않는다.
에이전트는 효과 적용·재계산·반환값·무적 판정의 소스 흐름을 검토하고, 빌드와 실제 게임 확인은 사용자가 수행한다.
이번 보완에서도 빌드·실행은 하지 않았다.
