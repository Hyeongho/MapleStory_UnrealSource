# Visual Studio 필터 구성

필터는 솔루션 탐색기의 표시 분류다. 이번 정리는 실제 소스 파일을 이동하지 않으며 기존 include와 빌드 경로를 유지한다. 같은 클래스의 헤더와 cpp는 같은 필터에 둔다. Test 프로젝트와 생성된 GameEngine 배포 디렉터리는 변경하지 않는다.

Engine의 큰 분류 안에서 다음처럼 역할을 나눈다.

```text
00. Runtime   → Engine / Launch / GameInstance / PCH / Deployment
01. Core      → 기존 Memory / Templates / Containers / Math / String / Logging / SmartPointer
02. Object    → TypeSystem / Actors / Components
03. Timer
04. Ability   → Types / Attributes / Tags / Effects / Abilities / Components
05. Render    → Device / Camera / Pipeline / Components / Loaders / Effects
                Loaders → Images / Map / Spine
06. World     → World / Map
07. Animation → Flipbook / StateMachine / Notify / Spine
08. Physics   → Shapes / Components / World / Terrain
                Components → Collision / Body / Climbing
09. Input     → Types / PlayerInput / Bindings
10. Audio     → Assets / Playback / Components / WZ
```

오디오 필터는 다음 파일을 포함한다.

- Assets: USoundWave
- Playback: FAudioManager, AudioTypes
- Components: UAudioComponent
- WZ: WzSoundLoader

Game은 다음처럼 실행 설정과 게임플레이 코드를 나눈다.

```text
01. Runtime  → Launch / GameInstance
02. Gameplay → Character / Controller
```

수정 파일은 `Engine/Include/Engine.vcxproj.filters`, `Game/Include/Game.vcxproj.filters`다. 새 파일은 vcxproj와 filters 양쪽에 등록한다. 파일을 찾기 어렵다면 솔루션 탐색기의 ‘모든 파일 표시’ 대신 위 기능별 필터를 기준으로 탐색한다. 이미 VS에 프로젝트가 열려 있다면 외부 파일 변경 알림에서 다시 로드한다.
