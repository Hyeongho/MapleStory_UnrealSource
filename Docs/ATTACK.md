# Ctrl 공격 애니메이션

## 입력과 재생 흐름

`AMaplePlayerController`가 Ctrl(VK_CONTROL)을 Attack 액션에 매핑하고 Pressed 이벤트에서 캐릭터의 Attack을 호출한다.
Debug와 Release에서 모두 사용할 수 있으며 키를 누른 상태로 유지해도 자동 반복하지 않는다.

`AMapleCharacter::InitializeAvatar`가 공격 프레임을 공유 UFlipbook 에셋으로 한 번 로딩하고 Attack 상태에 반복 재생 false로 등록한다.
기본 공격 액션은 원본 AvatarCanvas의 기본 액션 목록에 있는 swingO1이다. 실제 WZ와 장비에 맞게 초기화의 마지막 인자를 지정할 수 있다.

```cpp
pPlayerCharacter->InitializeAvatar(Device, WzPath, LoadoutSpec, "swingO1");
```

Attack은 해당 상태로 전환한 뒤 PlayFromStart를 한 번 호출한다. 재생 중인 공격 입력은 무시하며 입력을 예약하지 않는다.
UpdateAnimation은 공격이 재생되는 동안 Idle/Move/Jump 선택을 보류한다. 반복하지 않는 Flipbook이 끝나 IsPlaying이 false가 되면
공격 상태를 해제하고 현재 이동·접지 상태의 모션으로 돌아간다. 다음 공격도 기존 에셋을 0번 프레임부터 재사용한다.

사망했거나 로프·사다리에 매달려 있으면 공격을 시작하지 않는다. 공격 도중 매달리거나 사망하거나 외부에서 애니메이션 상태를 바꾸면 공격을 중단한다.
공격 중에도 기존 이동·점프 입력과 물리 처리는 계속 적용된다. 로딩에 실패한 공격 액션은 경고를 출력하고 재생하지 않는다.
이번 범위는 공격 모션 재생이며 공격 판정·피해 적용·콤보는 포함하지 않는다. 기존 DLL의 액션별 아바타 추출 API를 사용한다.

## 사용자 확인

1. Ctrl을 눌렀을 때 공격 모션이 한 번 재생되고 Idle로 돌아오는지 확인한다.
2. 방향키 이동 또는 공중에서 공격해도 공격 모션을 유지하고 종료 후 Move/Jump로 돌아오는지 확인한다.
3. 공격 중 Ctrl을 다시 눌러도 재시작하지 않고, 종료 후 다시 눌렀을 때 0번 프레임부터 재생되는지 확인한다.
4. 로프·사다리에서는 공격을 시작하지 않고, 공격 중 매달리면 해당 오르기 자세로 바뀌는지 확인한다.
5. 액션을 로딩하지 못하면 `[PlayerAnimation] avatar action unavailable` 로그와 실제 WZ 액션 이름을 확인한다.

빌드·실행은 사용자가 수행한다. Test 프로젝트는 변경하지 않는다.
