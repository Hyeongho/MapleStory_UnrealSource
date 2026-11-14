# 오디오 사용법

현재 구현은 파일·메모리를 디코딩하는 `USoundWave`, WZ 사운드를 추출하는 `FWzSoundLoader`, 재생을 관리하는 `FAudioManager`, 액터에 붙이는 `UAudioComponent`로 나뉜다. 에셋과 재생 인스턴스를 분리하고 에셋을 공유하는 구조다. 이 엔진에는 GC가 없으므로 에셋은 `TSharedPtr`로 보관한다.

## WZ 사운드와 맵 BGM

`WzNativeLib`의 `SoundExports.cs`에 `wz_read_sound`, `wz_map_read_bgm` API를 추가했다. **DLL을 다시 게시하고 Game이 사용하는 WzNativeLib.dll을 교체해야 한다.** 기존 DLL에는 이 API가 없으며 Engine만 다시 빌드해도 WZ 사운드는 추가되지 않는다. 기존 이미지·맵 API의 구조체 크기와 함수 시그니처는 변경하지 않았다.

기본 Game 설정은 다음과 같다.

```cpp
static constexpr bool MAPLE_USE_MAP_BGM = true;
static const char* MAPLE_BGM_WZ_NODE = nullptr;
static const char* MAPLE_ATTACK_SOUND_WZ_NODE = nullptr;
```

`InitAudio()`는 현재 맵의 `info/bgm`을 읽어 Sound 리소스를 한 번 로딩하고 반복 재생한다. `Bgm00/GoPicnic` 같은 값은 `Sound\Bgm00.img\GoPicnic`으로 변환한다. 맵 `info/link`가 있더라도 BGM은 요청한 맵 자신의 info를 사용한다. UOL 링크와 기본 multi BGM 선택은 MapRender2처럼 처리하며, 여러 후보가 있으면 첫 사운드/UOL을 선택한다. 조건에 따른 multi BGM 전환은 아직 구현하지 않았다.

다른 사운드를 지정하려면 실제 WZ 노드 경로를 Game에 넣는다.

```cpp
// 예시 BGM. 실제 WZ에 해당 노드가 있는지 확인한다.
static const char* MAPLE_BGM_WZ_NODE = R"(Sound\Bgm00.img\GoPicnic)";
// 공격 효과음에는 WZ에서 확인한 사운드 노드 경로를 지정한다.
static const char* MAPLE_ATTACK_SOUND_WZ_NODE = nullptr;
```

BGM 선택 우선순위는 일반 파일 경로 → 직접 지정한 WZ 노드 → 맵 info/bgm이다. 효과음은 일반 파일 경로 → 직접 지정한 WZ 노드 순서다. BGM을 끄려면 두 BGM 경로를 nullptr로 두고 `MAPLE_USE_MAP_BGM = false`로 설정한다. 공격 사운드는 무기·스킬마다 다르므로 경로를 자동으로 추측하지 않는다.

다른 액터나 UI에서도 동일한 에셋 로더를 사용할 수 있다.

```cpp
#include "Audio/WzSoundLoader.h"

TSharedPtr<USoundWave> Music = FWzSoundLoader::LoadMapBGM(WzPath, MapPath);
TSharedPtr<USoundWave> Effect = FWzSoundLoader::LoadSound(WzPath, SoundNodePath);
```

`WzPath`는 Sound가 포함된 Base.wz/데이터 폴더 또는 Sound.wz 자체의 경로이며 UTF-8이다. 맵 BGM 자동 조회에는 Map과 Sound가 함께 보이는 통합 루트를 사용한다. 노드 경로는 `Sound\Bgm00.img\GoPicnic`, `Bgm00/GoPicnic` 형식을 모두 지원한다.

```text
Map.wz info/bgm 또는 Game에서 지정한 사운드 경로
  → DLL: Wz_Sound.ExtractSound()로 MP3 / RIFF-WAVE 바이트 추출
  → FWzSoundLoader: DLL 버퍼를 USoundWave::LoadFromMemory에 전달
  → USoundWave: 메모리 스트림에서 공통 PCM으로 디코딩
  → FAudioManager / UAudioComponent: 기존 재생 경로 사용
```

임시 MP3/WAV 파일을 만들지 않는다. DLL 반환 버퍼는 `wz_free()`로 해제하며, PCM 에셋은 재생이 끝날 때까지 엔진이 유지한다. 추출·디코딩은 처음 로딩할 때만 수행한다. 루프에서는 DLL을 다시 호출하거나 버퍼를 다시 할당하지 않는다. 자동 경로 캐시는 없으므로 효과음은 미리 로딩해 공유한다. 맵 전환 기능에서 BGM을 바꾸려면 Game이 새 맵을 로드한 뒤 해당 에셋으로 `PlayBGM()`을 호출한다. 현재 연결 지점은 Game 초기화의 `InitAudio()`다.

## Game에서 바로 연결

`Game/Include/GameInstance/UMapleGameInstance.cpp`의 두 경로를 지정한다. 기본값 `nullptr`은 해당 사운드를 사용하지 않는다는 뜻이다.

```cpp
static const wchar_t* MAPLE_BGM_FILE = LR"(D:\MapleAssets\Audio\BGM.mp3)";
static const wchar_t* MAPLE_ATTACK_SOUND_FILE = LR"(D:\MapleAssets\Audio\Attack.wav)";
```

`InitAudio()`가 파일을 한 번 로딩한다. BGM은 0.5초 페이드인 후 반복 재생하며 BGM 분류 볼륨은 0.5다. 효과음은 플레이어의 오디오 컴포넌트에 전달되어 Ctrl 공격이 실제로 시작될 때 한 번 재생된다. 공격이 거부되면 재생하지 않는다. 파일이나 WZ 노드가 없거나 장치 초기화가 실패해도 게임과 공격 애니메이션은 계속 동작한다.

상대 경로는 실행 파일 위치가 아니라 **프로세스 작업 폴더**를 기준으로 한다. 먼저 절대 경로로 확인하는 편이 간단하다. 일반 파일 로딩 자체는 WZ DLL 없이도 사용할 수 있다.

## BGM과 전역 효과음

엔진 초기화 이후 게임 스레드에서 호출한다. 필요한 헤더는 `Engine.h`, `Audio/FAudioManager.h`다.

```cpp
const TSharedPtr<FAudioManager>& Audio = GetEngine().GetAudioManager();
if (Audio && Audio->IsInitialized())
{
    TSharedPtr<USoundWave> Music = USoundWave::LoadFromFile(LR"(D:\MapleAssets\Audio\Town.mp3)");
    Audio->PlayBGM(Music, 1.0f, 0.5f);
    Audio->SetMasterVolume(0.8f);
    Audio->SetBGMVolume(0.5f);
    Audio->SetSFXVolume(1.0f);
}
```

다른 에셋에 `PlayBGM()`을 호출하면 기존 음악을 페이드아웃하면서 새 음악을 페이드인한다. 새 음악을 시작할 수 없으면 기존 음악을 유지한다. 같은 공유 에셋을 다시 넘기면 음악을 처음부터 재시작하지 않는다. 매번 `LoadFromFile()`을 호출하면 서로 다른 에셋이 생기므로 게임이 로딩한 에셋을 보관해 재사용한다.

```cpp
// Effect는 미리 로딩해 보관한 TSharedPtr<USoundWave>다.
FAudioPlayParams Params;
Params.m_Volume = 0.7f;
FAudioHandle First = Audio->PlaySound(Effect, Params);
FAudioHandle Second = Audio->PlaySound(Effect, Params); // 같은 PCM을 공유하며 겹쳐 재생.
Audio->SetPitch(Second, 1.2f);
Audio->Pause(First);
Audio->Resume(First);
Audio->Stop(First);
Audio->StopBGM(0.5f);
```

핸들은 해당 매니저의 재생 인스턴스에만 사용한다. `IsValid()`는 핸들 번호가 있다는 뜻이다. 실제 재생 상태는 `Audio->IsActive()` / `IsPlaying()` / `IsPaused()`로 확인한다. 효과음은 완료되면 자동 정리하므로 핸들을 보관하지 않아도 된다. 매니저가 재생 완료까지 사운드 에셋을 유지한다.

## 액터의 오디오 컴포넌트

필요한 헤더는 `Audio/UAudioComponent.h`다. 컴포넌트는 소유 액터의 기존 위치 컴포넌트에 붙인다.

```cpp
UAudioComponent* pAudio = AddComponent<UAudioComponent>();
pAudio->SetAttachParent(GetFlipbookComponent()); // ACharacter 예제.
pAudio->SetSound(Effect);
pAudio->SetVolume(0.7f);
pAudio->SetPitch(1.0f);
pAudio->SetLooping(false);
pAudio->SetAttenuation(true, 100.0f, 1000.0f);
pAudio->Play();
```

컴포넌트 하나는 재생 하나를 관리한다. `Play()`를 다시 호출하면 자신의 기존 재생을 중단하고 처음부터 시작한다. 같은 액터에서 여러 소리를 겹치려면 컴포넌트를 나누거나 매니저의 `PlaySound()`를 사용한다. `SetSound()`는 다른 에셋으로 바꿀 때 기존 재생을 중단하며 자동 재생하지 않는다. `EndPlay()`와 소멸자는 재생을 중단한다.

볼륨·피치·거리 감쇠 설정은 현재 재생에 즉시 반영한다. 루프·분류 설정은 다음 `Play()`부터 반영한다. `Play(0.5f)`는 페이드인, `FadeOut(0.5f)`는 페이드아웃 후 중단이다. 일시정지 중에는 페이드 시간도 멈춘다. 정지된 음악이나 효과음에 페이드아웃을 요청하면 즉시 중단한다.

거리 감쇠는 안쪽 반경까지 원래 볼륨, 바깥 반경까지 선형 감소, 그 밖은 무음이다. 현재 Game은 청취 기준점을 매 프레임 플레이어 위치로 갱신한다. 반경 단위는 맵 좌표와 같으며 좌우 패닝·도플러 효과는 구현하지 않았다. BGM과 플레이어 공격 효과음에는 기본적으로 감쇠를 적용하지 않는다.

## 버퍼와 네이티브 자원 수명

- `USoundWave`: Windows Media Foundation으로 파일 전체를 동기 디코딩하여 48kHz / 스테레오 / 16비트 PCM으로 저장한다. WAV·MP3 등 설치된 Windows 코덱이 지원하는 형식을 사용한다. OGG 등 추가 코덱이 필요한 형식은 지원을 보장하지 않는다.
- 파일 하나의 디코딩 결과는 최대 256MiB다. PCM은 초당 약 192KB를 사용한다. 현재는 디스크 스트리밍과 자동 경로 캐시가 없으므로 로딩 단계에서 에셋을 만들고 재사용한다.
- BGM은 같은 PCM 버퍼를 XAudio2에 한 번 제출하고 무한 루프 플래그로 반복한다. 루프마다 파일 로딩·PCM 할당·에셋 참조 교체를 하지 않는다.
- 보이스는 최대 32개이며 BGM과 페이드 전환 중인 음악도 한도를 사용한다. 한도에 도달하면 신규 재생은 빈 핸들을 반환한다. 자연 종료한 보이스는 재사용한다. 명시적으로 중단한 보이스는 PCM을 안전하게 해제하기 위해 파괴하고 다음 필요 시 다시 만든다.
- 오디오 콜백은 완료·오류 상태만 원자적으로 기록한다. 게임 스레드가 완료를 확인한 뒤 에셋 참조를 해제한다. 명시적 중단은 `DestroyVoice()` 반환 후 해제하므로 버퍼가 사용 중일 때 메모리를 먼저 해제하지 않는다.
- `UEngine`이 COM, Media Foundation과 XAudio2의 수명을 관리한다. 월드와 컴포넌트를 먼저 정리하고 오디오를 종료한 뒤 COM을 해제한다. 컴포넌트는 매니저를 약한 참조로 보관한다.
- 오디오 장치 오류가 발생하면 재생을 정리하고 무음으로 계속 실행한다. 장치 재연결 자동 복구는 없으며 필요하면 게임 스레드에서 매니저를 다시 `Initialize()`해야 한다.
- 공개 API와 파일 로딩은 엔진을 초기화한 게임 스레드에서 호출한다. 임의의 작업 스레드에서 파일 로딩이나 매니저 API를 호출하지 않는다.

네이티브 수명 규칙은 Microsoft의 [OnBufferEnd 문서](https://learn.microsoft.com/en-us/windows/win32/api/xaudio2/nf-xaudio2-ixaudio2voicecallback-onbufferend), 출력 형식 변환은 [Source Reader 문서](https://learn.microsoft.com/en-us/windows/win32/api/mfreadwrite/nf-mfreadwrite-imfsourcereader-setcurrentmediatype)를 기준으로 구성했다.
WZ 입력은 [메모리 스트림](https://learn.microsoft.com/en-us/windows/win32/api/shlwapi/nf-shlwapi-shcreatememstream)과 [Source Resolver의 바이트 스트림 처리](https://learn.microsoft.com/en-us/windows/win32/api/mfidl/nf-mfidl-imfsourceresolver-createobjectfrombytestream)를 사용한다. 처리기 선택에 쓰는 `wz.wav`/`wz.mp3`는 확장자 힌트이며 실제 파일 경로로 열지 않는다.

## 사용자 확인 항목

빌드·실행은 Codex에서 수행하지 않았다. Engine을 빌드해 기존 배포 미러를 갱신한 뒤 Game에서 아래 항목을 확인한다. Test 프로젝트는 사용하지 않는다.

- WAV 또는 MP3 로딩 후 BGM이 두 번째 루프 이후에도 유지되는지.
- 갱신한 DLL로 현재 맵의 info/bgm이 로딩되고 `[WzSound] loaded` 로그가 나오는지.
- MP3 노드·PCM 노드와 UOL 참조 사운드가 메모리에서 재생되는지.
- Ctrl 공격 시작에만 효과음이 나오고, 공격 거부 상태에서는 나오지 않는지.
- 두 효과음의 중첩과 마스터/BGM/SFX 볼륨, 개별 피치·일시정지·재개가 적용되는지.
- 다른 BGM으로 전환할 때 페이드가 동작하고 창을 닫을 때 안전하게 종료되는지.
- 잘못된 파일 경로 또는 출력 장치가 없는 환경에서도 게임이 동작하는지.
