# 엔진 실행 구조

Game의 Windows 진입점은 창 제목, 화면 크기, 표시 모드를 설정하고 `FEngineLoop`를 호출한다. 창 생성, 엔진 초기화, 메시지 처리, 프레임 실행 및 종료는 Engine 프로젝트가 담당한다.

## 역할

| 위치 | 역할 |
| --- | --- |
| `Game/Include/main.cpp` | 실행 설정, 게임 인스턴스와 엔진 루프 연결 |
| `Engine/Include/EngineLoop.h/.cpp` | 창 생성, Windows 메시지 처리, 시간 측정, Init/Run/Tick/Exit |
| `Engine/Include/Engine.h/.cpp` | UEngine과 GEngine, 공통 시스템의 생성 및 해제, 월드를 통한 컨트롤러 조회 |
| `Engine/Include/Input/UPlayerInput.h/.cpp` | 키 상태, 누름/해제 전환, 게임별 축 매핑 |
| `Engine/Include/Input/UInputComponent.h/.cpp` | 입력 축과 동작 함수의 바인딩 및 호출 |
| `Engine/Include/Object/APlayerController.h/.cpp` | 플레이어 입력 소유, 입력 처리 순서, 기본 카메라 조작 |
| `Engine/Include/UGameInstance.h/.cpp` | 게임 초기화·갱신·종료를 연결하는 기본 클래스 |
| `Engine/Include/World/UWorld.h/.cpp` | 액터·물리 및 선택적으로 생성한 맵 씬의 관리 |
| `Game/Include/UMapleGameInstance.h/.cpp` | 게임 컨트롤러 생성, WZ 맵 선택, 아바타 생성, 데모 타이머, 카메라 시작 위치 |
| `Game/Include/AMaplePlayerController.h/.cpp` | 게임별 키 매핑 및 동작 바인딩 확장 |

언리얼의 FEngineLoop / UEngine / UGameInstance 역할 분담을 참고했다. 현재 프로젝트의 단일 월드와 2D 맵 씬에 맞춘 구조이며 언리얼의 전체 실행 구조를 구현한 것은 아니다.

## 초기화

1. `FEngineLoop::Init`이 창을 생성한다.
2. `UEngine::Init`이 로거, DirectX 디바이스, 스왑체인, 스프라이트 배치, 렌더링 큐, 카메라, 타이머, 월드를 생성한다. 월드 생성만으로 맵 씬을 만들지는 않는다.
3. `UGameInstance::Init`이 `CreatePlayerController`로 해당 월드에 컨트롤러를 스폰한다. 기본 클래스는 APlayerController, Maple 게임은 AMaplePlayerController를 생성한다. 컨트롤러는 입력 객체 및 컴포넌트를 소유하고 `InitPlayer`에서 카메라 연결, 키 매핑, 함수 바인딩을 준비한다.
4. `UMapleGameInstance::Init`은 맵과 캐릭터를 준비한다. `FMapLoader::LoadMap`은 전달받은 월드에 맵 씬을 생성하고 맵 데이터와 액터를 등록한다.
5. 프레임 시계를 준비한 뒤 창을 표시한다.

## 프레임 순서

1. `UPlayerInput::BeginFrame`으로 키 전환 기록 초기화, Windows 메시지로 키 상태 갱신 및 종료 요청 확인
2. 프레임 시간 측정, `APlayerController::ProcessPlayerInput` → `UInputComponent::ProcessInput` → 바인딩된 축 함수 호출
3. `UGameInstance::PreTick` — 월드 갱신 전에 게임 명령을 준비할 수 있는 훅
4. `UEngine::Tick` — 타이머와 월드 갱신. `UWorld::Tick`이 액터·물리를 처리하고 맵 씬이 있을 때만 맵 애니메이션 및 캐릭터 발판 레이어를 갱신한다.
5. `APlayerController::UpdateCamera` — 이번 프레임의 축 값으로 카메라 이동. 이후 `UGameInstance::Tick`에서 게임 상태 갱신
6. `UEngine::Render` — `UWorld::Render`에 맵·액터 제출을 위임한 뒤 정렬, 월드·UI 출력, Present

카메라 갱신은 배경 반복 범위를 계산하는 렌더링 제출보다 앞에 실행된다. 카메라 시작 위치는 `(100, 300)`이며 이후 방향키로 이동한다. 캐릭터 시작 위치와 2초 간격 애니메이션 전환은 유지했다. 캐릭터 이동과 맵 발판 물리 연동은 아직 추가하지 않았다.

## 키보드 카메라 조작

↑↓←→를 누르고 있는 동안 해당 방향으로 카메라가 초당 400 월드 좌표 단위 이동한다. 이동량에는 DeltaTime을 곱하며, 대각선 속도를 보정한다. 반대 방향키를 함께 누르면 해당 축은 상쇄된다. 창 포커스를 잃으면 키 상태를 초기화한다. 마우스 조작은 사용하지 않는다.

`AMaplePlayerController::SetupInputMappings`에서 `MoveHorizontal`과 `MoveVertical` 축의 키를 바꿀 수 있다. 예를 들어 Left/Right를 A/D, Up/Down을 W/S로 교체하면 WASD 조작이 된다. `SetupInputComponent`는 기본적으로 부모의 카메라 바인딩을 사용한다. 게임 고유 동작은 이 함수에서 바인딩하거나 가상 함수 `MoveHorizontal` / `MoveVertical`을 재정의한다. 컨트롤러의 `SetCameraMoveSpeed`로 기본 카메라 속도를 설정한다.

향후 캐릭터 이동으로 전환할 때는 컨트롤러의 `SetCameraInputEnabled(false)`로 기본 카메라 이동을 끄고, 같은 축의 콜백에서 캐릭터 이동 명령을 전달한다. 카메라의 캐릭터 추적은 월드 갱신 후 실행되는 `UpdateCamera`를 재정의하여 구현할 수 있다. 현재 캐릭터 이동이나 추적은 구현하지 않았다.

입력 흐름은 `Windows 키 메시지 → UPlayerInput의 키 상태와 축 해석 → UInputComponent의 바인딩 → APlayerController의 동작 함수`다. 입력이 없을 때도 축 콜백에 0을 전달한다. 바인딩을 호출하는 동안 바인딩 목록을 변경하면 다음 입력 처리부터 반영한다.

`BindAxis<컨트롤러타입, &컨트롤러타입::함수>(축이름, this)`는 기존 FTimerDelegate와 같은 멤버 함수 래퍼 방식을 사용한다. 델리게이트는 대상 객체를 소유하지 않는다. 컨트롤러는 자신에게 바인딩하고 종료할 때 바인딩을 해제한다. 다른 객체에 바인딩하는 경우에는 그 객체를 해제하기 전에 바인딩을 제거해야 한다.

이번 범위는 단일 로컬 플레이어와 축 바인딩이다. APlayerController는 현재 AActor를 직접 상속하며, 언리얼의 AController/APawn 계층, Possess, 입력 컴포넌트 우선순위 스택, 액션 바인딩 및 Enhanced Input은 아직 구현하지 않았다.

## 종료와 소유권

`FEngineLoop`는 UEngine을 소유하며 UGameInstance는 참조만 한다. 따라서 게임 인스턴스는 엔진 루프보다 먼저 선언해 루프가 소멸할 때도 살아 있어야 한다. UEngine은 엔진 시스템과 월드를, UWorld는 컨트롤러를 포함한 액터 및 맵 씬을 소유한다. 각 액터는 자신의 컴포넌트를 소유하며 컨트롤러는 UPlayerInput도 소유한다. UEngine의 GetPlayerInput은 조회만 위임하고 입력 객체를 생성하거나 해제하지 않는다.

`UWorld::CreateMapScene`은 처음 필요할 때만 씬을 생성하며 `GetMapScene`은 씬이 없으면 nullptr를 반환한다. `DestroyMapScene`은 맵에서 생성한 액터와 씬 자원을 정리하고 캐릭터처럼 외부에서 만든 액터는 유지한다. 유지한 캐릭터의 이전 맵 레이어는 해제한다. 맵 씬은 생성 시 소유 월드에 연결되므로 다른 월드를 Clear에 잘못 전달할 수 없다.

맵 씬이 없는 월드도 액터 갱신, 물리와 액터 렌더링을 그대로 사용할 수 있다. 월드 소멸 시에도 맵 씬을 먼저 정리한 뒤 나머지 액터를 해제한다.

`Exit`은 게임의 `Shutdown`으로 데모 타이머와 참조를 정리하고, 엔진 소유 자원을 정리한 뒤 창과 창 클래스를 해제한다. 초기화가 중간에 false를 반환해도 같은 종료 경로를 사용한다. 반복 Exit/Shutdown 호출은 이미 해제한 자원을 다시 해제하지 않는다.

아바타 프레임을 상태 머신에 등록한 뒤 로딩 과정의 COM 참조를 바로 반납한다. Debug 메모리 집계는 게임과 엔진의 종료 처리 뒤에 수행하며 프로세스 전역 객체의 정적 소멸이 끝난 시점은 아니다.

## 적용

새 소스와 헤더는 Engine/Game의 vcxproj 및 filters에 등록했다. Game은 기존대로 Engine 공개 헤더를 참조한다. GameEngine 배포 미러는 Engine 빌드 시 Copy.bat으로 갱신하며 직접 편집하지 않았다.

이번 변경에서는 빌드, 테스트, 프로그램 실행을 하지 않았다.

## 커밋 문구

```text
refactor: 엔진 초기화와 메인 루프를 Engine 프로젝트로 이관

- FEngineLoop에서 창 생성, 메시지 처리, 프레임 실행과 종료 관리
- UEngine에서 렌더링·카메라·타이머·월드의 수명 관리
- UWorld에서 맵 씬을 필요할 때만 생성하고 갱신·렌더링·해제 관리
- UGameInstance 훅과 UMapleGameInstance로 게임 초기화 및 카메라 정책 분리
- Game main을 실행 설정과 엔진 루프 호출로 축소
- 데모 타이머 취소와 아바타 프레임의 로컬 텍스처 참조 정리
- 신규 소스 및 헤더를 Visual Studio 프로젝트에 등록
- UGameInstance를 월드 폴더에서 엔진 기본 클래스 위치로 이동
- 컨트롤러가 UPlayerInput과 UInputComponent를 소유하고 Game에서 키와 동작 설정
- UInputComponent의 축 바인딩과 APlayerController의 입력·카메라 처리 추가
- 방향키로 카메라만 이동하고 매 프레임 카메라를 고정하던 코드 제거
```
