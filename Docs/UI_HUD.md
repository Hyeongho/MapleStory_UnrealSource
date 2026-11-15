# 최소 UI와 HP HUD

Engine은 UI 공통 기능과 렌더링을, Game은 HUD의 디자인과 게임 값 연결을 담당한다. 현재 Game은 제공된 StatusBar3의 배경·HP 채움·프레임을 WZ에서 로딩해 화면 하단의 가로 중앙에 배치한다. 로딩할 수 없으면 같은 위치에 단색 HP바로 대체한다.

2026-11-15 소스 구현 상태이며 빌드·실행은 수행하지 않았다. 기존 브랜치에서 진행하고 Test 프로젝트와 생성된 GameEngine 파일은 수정하지 않았다.

## 구성과 수명

- `UWidget`: 표시 상태·불투명도·부모 조회와 공통 Paint 진입점.
- `UUserWidget`: Game HUD처럼 여러 위젯을 조합하는 루트.
- `UCanvasPanel`: 자식과 슬롯 소유, 슬롯의 ZOrder로 그리기 순서 결정.
- `UCanvasPanelSlot`: 앵커·정렬·오프셋·크기로 자식 배치.
- `UImage`: 이미지 또는 단색 사각형.
- `UProgressBar`: 배경과 0~1 채움 비율. HP가 0이면 채움만 사라진다.
- `FSlateBrush`: 텍스처 참조와 색상. 텍스처가 없으면 엔진의 공통 흰색 1픽셀에 색상을 적용한다.
- `FWzUIBrushLoader`: WZ Canvas를 브러시로 로딩하고 원본 origin·delay를 전달한다.
- `FUIViewport`: 엔진이 소유하는 UI 루트 목록·공통 텍스처·화면 배율.
- `UMapleHUD`: Game의 HP바 구성과 체력 비율 계산.

언리얼의 [Canvas 슬롯](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/UMG/UCanvasPanelSlot)에 있는 앵커·정렬·오프셋·ZOrder 역할을 참고했다. 현재는 UObject 위젯을 기존 DX11 렌더 큐로 직접 그리며, 실제 UMG의 UPanelWidget/UPanelSlot·Slate·GC·Blueprint 전체 계층을 구현한 것은 아니다. 직접 그리기 도구인 UCanvas에 HUD 구성을 모으지 않고 UserWidget과 CanvasPanel/Slot을 구분했다.

이 엔진에는 GC가 없으므로 위젯은 `MakeShared`로 생성한다. GameInstance와 뷰포트가 HUD를 공유 참조하며, Canvas는 슬롯과 자식을 공유 참조한다. 자식의 부모와 슬롯의 Canvas 포인터는 비소유다. 이미 부모/뷰포트에 붙은 위젯의 중복 등록과 순환 부모 관계는 거부한다. 한 위젯을 다른 부모로 옮길 때는 먼저 기존 부모에서 제거한다.

슬롯 반환 포인터는 Canvas가 소유하므로 자식을 제거한 뒤 사용하지 않는다. Canvas의 `GetChildAt()`는 추가 순서를 유지하고 그리기용 포인터만 별도로 정렬한다. 같은 ZOrder는 추가 순서를 따른다. 그리기 중에는 위젯 트리나 뷰포트 목록을 변경하지 않으며, UI API는 게임 스레드에서 사용한다.

## 체력과 렌더링 흐름

```text
입력과 World Tick → 실제 Health 변경
  → UMapleGameInstance::Tick
  → UMapleHUD::UpdateHealth(Health, MaxHealth)
  → UProgressBar::SetPercent

UEngine::Render
  → 월드 렌더 항목 제출
  → FUIViewport가 HUD의 앵커 배치를 계산하고 UI 항목 제출
  → 월드 출력 / Debug 충돌 출력
  → FlushUI에서 카메라 변환 없이 HUD 출력
```

UI는 체력을 변경하지 않고 현재 Health/MaxHealth만 읽는다. 캐릭터가 없으면 HUD를 숨긴다. 비정상 값 또는 MaxHealth가 0 이하인 경우는 빈 게이지로 처리하며, 정상 값은 0~1로 제한한다.

월드 Flush와 FlushUI는 각각 자신이 연 SpriteBatch를 끝내고 반환한다. UI 항목 처리 뒤에도 열린 배치가 있으면 End한 다음 큐를 비워야 다음 프레임의 Begin과 중첩되지 않는다.

`SetPercent()`는 숫자만 갱신한다. 매 프레임 텍스처 AddRef/Release나 리소스 로딩은 하지 않는다. 채움 이미지는 원본의 왼쪽 영역을 잘라 지정된 폭에 맞춘다. 렌더 큐에 소스 RECT를 선택적으로 추가했으며 기존 월드 항목은 기본값인 전체 이미지 출력을 유지한다.

Game 종료 시 HUD를 뷰포트에서 제거한다. Engine은 렌더 큐를 비운 다음 UI 루트·공통 텍스처를 해제하고, 마지막에 DX11 장치를 정리한다.

## StatusBar3 WZ 스킨

사용자가 제공한 `UI.StatusBar3.img.xml`의 다음 노드를 기본 스킨으로 사용한다.

| 역할 | UI\StatusBar3.img 아래 경로 | 원본 origin | 그리기 순서 |
| --- | --- | --- | --- |
| 배경 | mainBar\status\backgrnd | (-2, -24) | 0 |
| HP 채움 | mainBar\status\gauge\hp\layer:0 | (-24, -28) | 2 |
| 프레임 | mainBar\status\layer:cover | (1, 0) | 3 |

이 PNG들은 XML에서 1x1이며 `_outlink`가 `UI/_Canvas/StatusBar3.img/...`를 가리킨다. 해당 1x1 참조 픽셀을 늘리는 방식으로는 원본 UI가 나오지 않는다. 새 DLL은 UOL을 먼저 해석하고, `source / _inlink / _outlink`를 따라 실제 PNG를 읽는다. 크기는 실제 PNG에서, origin과 delay는 UOL 해석 후 원본 Canvas에서 읽는다. 실제 `_Canvas` 데이터는 게임이 여는 WZ에 포함되어 있어야 한다.

**WzNativeLib.dll을 다시 게시·교체하고 Game을 재실행해야 한다.** `wz_read_canvas`의 기존 8개 인자는 유지했으며, 새 `wz_canvas_api_version` export가 2를 반환한다. UI 로더는 구 DLL을 감지하면 로딩을 실패로 처리하여 단색 HP바를 유지한다. DLL 쪽 변경 파일은 `WzExports.cs`, `CanvasResources.cs`다.

GameInstance의 설정으로 리소스를 바꾼다.

```cpp
static constexpr bool MAPLE_USE_WZ_HUD = true;
static const char* MAPLE_HUD_BACKGROUND_NODE = R"(UI\StatusBar3.img\mainBar\status\backgrnd)";
static const char* MAPLE_HUD_HEALTH_NODE = R"(UI\StatusBar3.img\mainBar\status\gauge\hp\layer:0)";
static const char* MAPLE_HUD_FRAME_NODE = R"(UI\StatusBar3.img\mainBar\status\layer:cover)";
```

`MAPLE_USE_WZ_HUD = false`는 기존 단색 디자인이다. `FMapleHUDWzStyle`은 Game 타입이며 경로·하단 위치·ZOrder를 설정한다. 기본 ZOrder 0/2/3은 제공된 XML에서 확인한 값이며 일반 WZ 이미지의 Z를 자동 조회하는 기능은 아니다.

`UMapleHUD::InitializeFromWz()`는 이미지들을 원본 크기로 만들고 `-origin` 위치에 겹친다. 세 이미지의 합집합 경계를 하단 앵커에 맞추므로 상대 위치를 유지한다. HP바는 배경을 투명하게 하고 실제 HP 채움 이미지만 비율에 따라 자르며, 프레임은 그 앞에 그린다. 스킨에 포함된 MP 영역·문구가 보여도 MP 수치나 버튼 동작을 연결한 것은 아니다. 이 작업은 StatusBar3 전체 UI를 자동 생성하지 않는다.

## 일반 WZ UI 이미지

`UImage`는 Canvas 노드 하나를 직접 지정할 수 있다. 원본 크기로 배치하려면 브러시의 크기를 슬롯에 전달한다. 필요한 헤더는 `UI/UImage.h`, `UI/UCanvasPanel.h`다.

```cpp
TSharedPtr<UImage> Image = MakeShared<UImage>();
if (Image->SetBrushFromWz(Device, WzPath,
    R"(UI\StatusBar3.img\mainBar\status\layer:cover)"))
{
    UCanvasPanelSlot* pSlot = Canvas.AddChildToCanvas(Image);
    if (pSlot)
    {
        pSlot->SetPosition(FVector2D(24.0f, 24.0f));
        pSlot->SetSize(Image->GetBrush().GetImageSize());
    }
}
```

예제의 Canvas는 화면 루트에 연결된 Canvas이며 Device/WzPath는 게임이 선택한 장치와 WZ 경로다. 버튼 상태·애니메이션 폴더를 지정하는 대신 `normal/0`처럼 실제 Canvas 프레임까지 지정한다. 현재는 한 프레임의 정적 이미지 로딩이며 버튼 상태 전환과 애니메이션 재생은 후속 작업이다.

HP바나 다른 위젯에 쓰려면 공통 로더를 사용한다.

```cpp
FWzUIBrush Resource;
if (FWzUIBrushLoader::LoadBrush(Device, WzPath, NodePath, Resource))
{
    ProgressBar.SetFillBrush(Resource.m_Brush);
    // Resource.m_Origin으로 여러 조각을 같은 기준점에 배치할 수 있다.
}
```

필요한 헤더는 `UI/WzUIBrushLoader.h`다. 로더와 `SetBrushFromWz()`는 실패할 때 이전 브러시를 유지한다. 참조는 브러시가 소유하며 DLL 입력 픽셀은 GPU 업로드 후 해제한다. WZ 로딩은 초기화·스킨 변경 시에만 하고, Tick과 HP 비율 변경에서는 호출하지 않는다.

## 배치와 디자인 설정

단색 모드의 디자인은 `Game/Include/UI/UMapleHUD.cpp`의 `Initialize()`에서 바꾼다. HP 패널은 320x32이며 가로는 화면 중앙, 세로는 하단에서 24 UI 단위 떨어진 위치다. 붉은 채움, 어두운 배경, 3단위 테두리를 사용한다. WZ 모드는 `InitializeFromWz()`가 실제 이미지 크기와 원점을 사용하며 기본 위치는 같다.

```cpp
pSlot->SetAnchors(FAnchors(0.5f, 1.0f));
pSlot->SetAlignment(FVector2D(0.5f, 1.0f));
pSlot->SetPosition(FVector2D(0.0f, -24.0f));
pSlot->SetSize(FVector2D(320.0f, 32.0f));
```

고정 앵커에서는 Offsets의 Left/Top이 위치, Right/Bottom이 크기다. 한 축에서 최소·최대 앵커가 다르면 그 축은 늘어나며 Left/Top/Right/Bottom을 여백으로 사용한다. `FAnchors(0, 0, 1, 1)`과 `FMargin(3)`은 부모 가장자리에서 각각 3만큼 들어온 영역이다. 정렬은 계산한 사각형의 피벗이다.

뷰포트는 실제 백버퍼 크기와 기준 해상도 1366x768의 작은 쪽 비율로 균일 배율을 계산한다. 논리 화면 전체에 앵커를 적용하므로 화면 비율이 달라도 하단 고정을 유지한다. 기준 해상도는 `GetEngine().GetUIViewport().SetReferenceResolution()`으로 Game에서 바꿀 수 있다. 이 작업은 스왑체인이나 Win32 창 리사이즈 기능을 추가하지 않는다.

## 이미지 사용

WZ 이미지 로더와 일반 파일 로더가 반환한 2D SRV를 브러시에 지정할 수 있다. 아래는 Game 초기화 코드에서 사용할 수 있는 예제다. `Device`는 엔진의 DX11 장치다.

```cpp
TSharedPtr<UImage> Image = MakeShared<UImage>();
ID3D11ShaderResourceView* pTexture = FTextureLoader::LoadTextureFromFile(
    Device, LR"(D:\MapleAssets\UI\panel.png)");
if (pTexture)
{
    Image->SetTexture(pTexture); // 위젯의 브러시가 별도 참조를 확보한다.
    pTexture->Release();         // 로더가 반환한 호출자 참조를 해제한다.
}
```

예제에는 `UI/UImage.h`, `Render/TextureLoader.h`가 필요하다. 생성한 Image는 Canvas 슬롯에 추가해야 표시된다. WZ도 `FWzTextureLoader::LoadCanvasTexture()`로 읽은 SRV를 같은 방식으로 지정한다. 브러시 참조 방식은 참조를 넘겨받는 기존 USpriteComponent::SetTexture와 다르므로 호출자 참조를 반드시 구분한다.

게이지 이미지는 `FSlateBrush`의 `SetTexture()`와 흰색 Tint를 지정한 뒤 `UProgressBar::SetFillBrush()`로 전달한다. UMapleHUD의 Initialize에서 기본 색상 대신 지정할 수 있다. UImage 단독의 좌표는 좌상단이며 WZ origin을 자동 적용하지 않는다. StatusBar3를 조합하는 InitializeFromWz는 원본 origin을 명시적으로 적용한다. 현재 브러시는 단일 2D 이미지와 전체 이미지의 왼쪽 채움 영역만 지원하며 9-slice/아틀라스 UV는 아직 없다.

## 사용자 확인

- Engine을 빌드해 기존 배포 헤더·라이브러리를 갱신한 뒤 Game을 빌드한다. `.vcxproj`와 `.filters`도 함께 반영해야 새 UI cpp가 링크된다.
- 새 DLL 게시·교체 후 `[UIWz] loaded` 로그의 실제 이미지 크기와 원점을 확인한다. 구 DLL이나 링크 대상 누락은 단색 모드로 대체되는지 확인한다.
- Debug 실행 시 F1로 피해를 주면 붉은 HP바가 줄어들고 F2로 초기화하면 다시 채워지는지 확인한다. 무적 시간 동안 추가 피해 입력은 기존 규칙대로 무시된다.
- 카메라 이동·줌에도 HP바가 화면 하단의 가로 중앙에 남는지 확인한다.
- 시작 해상도를 바꿔 앵커·배율을 확인하고, 체력 0·전체 체력과 종료 시 자원 해제를 확인한다.

현재는 최소 HP바다. 텍스트·HP 숫자·MP/경험치·마우스 히트 테스트·버튼·인벤토리와 UI 클리핑/자동 크기는 후속 작업이다. Hidden과 Collapsed는 현재 절대 배치 Canvas에서 모두 그리지 않으며 흐름 레이아웃의 공간 예약 차이는 아직 없다.
