# 최소 UI와 HP·MP HUD

Engine은 UI 공통 기능과 렌더링을, Game은 HUD의 디자인과 게임 값 연결을 담당한다. 현재 Game은 제공된 StatusBar3의 배경·HP·MP 채움·프레임을 WZ에서 로딩해 화면 하단의 가로 중앙에 배치한다. 로딩할 수 없으면 같은 하단 여백에 단색 HP·MP바로 대체한다.

2026-11-15 소스 구현 상태이며 빌드·실행은 수행하지 않았다. 기존 브랜치에서 진행하고 Test 프로젝트와 생성된 GameEngine 파일은 수정하지 않았다.

## 구성과 수명

- `UWidget`: 표시 상태·불투명도·부모 조회와 공통 Paint 진입점.
- `UUserWidget`: Game HUD처럼 여러 위젯을 조합하는 루트.
- `UCanvasPanel`: 자식과 슬롯 소유, 슬롯의 ZOrder로 그리기 순서 결정.
- `UCanvasPanelSlot`: 앵커·정렬·오프셋·크기로 자식 배치.
- `UImage`: 이미지 또는 단색 사각형.
- `UProgressBar`: 배경과 0~1 채움 비율. HP가 0이면 채움만 사라진다.
- `UTextBlock`: FText 문구, Windows 글꼴, 가로 정렬·줄바꿈·색상·그림자. 변경 시 생성한 텍스처를 재사용한다.
- `UImageFont`: 문자별 브러시·아틀라스 영역·오프셋·다음 문자까지의 폭을 소유하는 공유 이미지 글꼴.
- `FSlateFontInfo` / `FUITextRenderer`: Game이 고른 글꼴 정보와 뷰포트가 소유하는 텍스트 생성 서비스.
- `FSlateBrush`: 텍스처 참조와 색상. 텍스처가 없으면 엔진의 공통 흰색 1픽셀에 색상을 적용한다.
- `FWzUIBrushLoader`: WZ Canvas를 브러시로 로딩하고 원본 origin·delay를 전달한다.
- `FUIViewport`: 엔진이 소유하는 UI 루트 목록·공통 텍스처·화면 배율.
- `UMapleHUD`: Game의 HP·MP바와 HP 숫자 구성. MP는 현재 표시용이며 실제 마나 값은 아직 연결하지 않았다.

언리얼의 [Canvas 슬롯](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/UMG/UCanvasPanelSlot)에 있는 앵커·정렬·오프셋·ZOrder 역할을 참고했다. 현재는 UObject 위젯을 기존 DX11 렌더 큐로 직접 그리며, 실제 UMG의 UPanelWidget/UPanelSlot·Slate·GC·Blueprint 전체 계층을 구현한 것은 아니다. 직접 그리기 도구인 UCanvas에 HUD 구성을 모으지 않고 UserWidget과 CanvasPanel/Slot을 구분했다.

이 엔진에는 GC가 없으므로 위젯은 `MakeShared`로 생성한다. GameInstance와 뷰포트가 HUD를 공유 참조하며, Canvas는 슬롯과 자식을 공유 참조한다. 자식의 부모와 슬롯의 Canvas 포인터는 비소유다. 이미 부모/뷰포트에 붙은 위젯의 중복 등록과 순환 부모 관계는 거부한다. 한 위젯을 다른 부모로 옮길 때는 먼저 기존 부모에서 제거한다.

슬롯 반환 포인터는 Canvas가 소유하므로 자식을 제거한 뒤 사용하지 않는다. Canvas의 `GetChildAt()`는 추가 순서를 유지하고 그리기용 포인터만 별도로 정렬한다. 같은 ZOrder는 추가 순서를 따른다. 그리기 중에는 위젯 트리나 뷰포트 목록을 변경하지 않으며, UI API는 게임 스레드에서 사용한다.

## 체력과 렌더링 흐름

```text
입력과 World Tick → 실제 Health 변경
  → UMapleGameInstance::Tick
  → UMapleHUD::UpdateHealth(Health, MaxHealth)
  → UProgressBar::SetPercent / UTextBlock::SetText

UEngine::Render
  → 월드 렌더 항목 제출
  → FUIViewport가 HUD의 앵커 배치를 계산하고 UI 항목 제출
  → 월드 출력 / Debug 충돌 출력
  → FlushUI에서 카메라 변환 없이 HUD 출력
```

UI는 체력을 변경하지 않고 현재 Health/MaxHealth만 읽는다. 캐릭터가 없으면 HUD를 숨긴다. MaxHealth가 비정상이거나 0 이하이면 빈 게이지와 `-- / --`로 처리한다. 비정상 Health는 0, 나머지는 0~MaxHealth로 제한한다. HP 숫자는 `현재값 / 최대값`을 정수로 반올림해 표시하며, 게이지는 원래 실수 비율을 사용한다.

월드 Flush와 FlushUI는 각각 자신이 연 SpriteBatch를 끝내고 반환한다. UI 항목 처리 뒤에도 열린 배치가 있으면 End한 다음 큐를 비워야 다음 프레임의 Begin과 중첩되지 않는다.

`SetPercent()`는 숫자만 갱신한다. 매 프레임 텍스처 AddRef/Release나 리소스 로딩은 하지 않는다. 채움 이미지는 원본의 왼쪽 영역을 잘라 지정된 폭에 맞춘다. 렌더 큐에 소스 RECT를 선택적으로 추가했으며 기존 월드 항목은 기본값인 전체 이미지 출력을 유지한다.

Game 종료 시 HUD를 뷰포트에서 제거한다. Engine은 렌더 큐를 비운 다음 UI 루트·글꼴 서비스·공통 텍스처를 해제하고, 마지막에 DX11 장치와 COM을 정리한다.

## Windows 글꼴로 텍스트 표시

`UTextBlock`의 문구는 `FText`, 글꼴은 `FSlateFontInfo`로 설정한다. Windows에 설치된 글꼴을 사용하며 한글도 DirectWrite의 글꼴 대체를 통해 처리한다. WZ 숫자 이미지나 `.spritefont` 파일은 필요하지 않다. 내부는 [Direct2D의 WIC 비트맵 렌더 타깃](https://learn.microsoft.com/en-us/windows/win32/api/d2d1/nf-d2d1-id2d1factory-createwicbitmaprendertarget%28iwicbitmap_constd2d1_render_target_properties_id2d1rendertarget%29)에 흰색 글자를 그린 뒤 DX11 텍스처로 업로드하는 방식이다. 투명 배경에서는 회색조 안티앨리어싱을 사용하고, 기존 SpriteBatch의 NonPremultiplied 블렌딩에 맞게 WIC에서 일반 BGRA 알파로 변환한다.

```cpp
// UI/UTextBlock.h, UI/UCanvasPanel.h가 필요하다.
TSharedPtr<UTextBlock> Label = MakeShared<UTextBlock>();
FSlateFontInfo Font;
Font.m_FontFamily = L"Malgun Gothic";
Font.m_Size = 16.0f;
Label->SetFont(Font);
Label->SetText(FText(L"캐릭터 정보"));
Label->SetJustification(ETextJustify::Center);
Label->SetShadowColorAndOpacity(FLinearColor::Black);
UCanvasPanelSlot* pSlot = Canvas.AddChildToCanvas(Label);
if (pSlot)
{
    pSlot->SetPosition(FVector2D(24.0f, 24.0f));
    pSlot->SetSize(FVector2D(200.0f, 32.0f));
}
```

Canvas는 화면 루트에 연결되어 있어야 한다. 세로 정렬은 중앙이며, 가로는 Left/Center/Right를 선택한다. 기본은 줄바꿈 없이 슬롯 영역에서 잘린다. 여러 줄은 `SetAutoWrapText(true)`로 설정한다. 자동 크기·리치 텍스트·부모 Canvas 클리핑은 아직 지원하지 않는다.

문구·글꼴·정렬·줄바꿈·슬롯 크기·UI 배율이 달라질 때만 텍스처를 다시 만든다. 같은 `SetText()` 호출, 위치·색상·불투명도·그림자 변경은 다시 만들지 않는다. 그림자와 본문은 같은 텍스처를 참조하며 매 Paint에서 브러시 복사나 텍스처 참조 교체를 하지 않는다. 장치 재초기화 시 세대를 비교해 캐시를 갱신한다. 생성 실패 시 이전 수치를 남기지 않고, 같은 상태에서 매 프레임 재시도하지 않는다. 명시적인 재시도는 `InvalidateTextLayout()`을 사용한다. 글꼴 초기화가 실패해도 이미지·게이지는 계속 표시한다.

Game의 `UMapleHUD::CreateGaugeText()`에서 Windows 기본 글꼴(12 UI 단위, 굵은 Segoe UI), 색상, 그림자를 바꿀 수 있다. WZ에서는 각 게이지의 원점과 폭에 맞추고 프레임보다 앞(ZOrder 4)에 숫자를 배치한다. 글자 행이 잘리지 않도록 텍스트 영역 높이는 최소 20이며 게이지 중앙을 유지한다. 단색 모드도 게이지와 같은 앵커·오프셋을 사용한다. WZ 스킨을 다시 구성해도 기존 수치와 비율을 복사한다.

MP는 아직 실제 마나 속성이 없으므로 `-- / --`로 표시하며 기존 게이지 기본 채움 100%를 유지한다. 마나 시스템이 준비되면 Game에서 `HUD.UpdateMana(CurrentMana, MaxMana)`를 호출한다. 이 API는 비율과 숫자를 함께 갱신하며 마나 소모나 회복을 직접 구현하지 않는다. 현재는 GameInstance에서 이 함수를 호출하지 않는다.

## 문자 이미지로 텍스트 표시

`UImageFont`에 문자와 이미지를 등록한 뒤 `UTextBlock::SetImageFont()`로 지정한다. 이미지는 초기화 때 로딩하고 여러 TextBlock이 같은 글꼴을 공유한다. 문구를 바꾸면 문자 배치만 갱신하며 GPU 텍스처를 새로 만들거나 로딩하지 않는다. 같은 문구·영역·글꼴이면 배치도 재사용한다. 글꼴의 문자를 교체하거나 행 높이를 바꾸면 변경 번호로 배치를 갱신한다. 그리기 중에는 글꼴을 수정하지 않는다.

일반 이미지 파일을 쓰는 예제다. PNG뿐 아니라 기존 FTextureLoader가 지원하는 형식을 사용할 수 있다. 아래 경로는 Game에서 준비한 문자 이미지 경로로 바꾼다.

```cpp
// UI/UImageFont.h, UI/UTextBlock.h
TSharedPtr<UImageFont> ImageFont = MakeShared<UImageFont>();
bool bLoaded = true;
for (int32 Digit = 0; Digit < 10; Digit++)
{
    wchar_t FilePath[260] = {};
    swprintf_s(FilePath, LR"(D:\MapleAssets\UI\digits\%d.png)", Digit);
    if (!ImageFont->LoadGlyphFromFile(Device, (wchar_t)(L'0' + Digit), FilePath))
    {
        bLoaded = false;
        break;
    }
}
TSharedPtr<UTextBlock> Label = MakeShared<UTextBlock>();
if (bLoaded)
{
    ImageFont->SetLineHeight(18.0f); // 실제 이미지 높이에 맞춘다.
    ImageFont->SetSpaceAdvance(4.0f);
    Label->SetImageFont(ImageFont);
    Label->SetImageFontScale(1.0f); // 원본 이미지 크기
}
Label->SetText(L"1203");
Label->SetJustification(ETextJustify::Center);
// 위 Windows 글꼴 예제처럼 Canvas 슬롯에 추가하고 크기를 지정한다.
```

`LoadGlyphFromWz(Device, L'0', WzPath, NodePath)`는 WZ Canvas를 같은 방식으로 등록하고 `-origin`을 문자 오프셋으로 사용한다. 이미지마다 폭이 다르면 기본 전진 폭은 해당 이미지 폭이다. 로더의 마지막 Advance 인자를 지정하면 문자 간격을 조절할 수 있다. 일반 이미지의 Offset은 행 시작점 기준이며 `SetLineHeight()`로 정한 행 영역에 맞게 지정한다. `SetImageFontScale()`은 행 높이·이미지 크기·간격을 함께 확대하며, Windows용 `FSlateFontInfo::m_Size`와는 별개다.

한 이미지에 여러 문자가 들어 있는 아틀라스는 기존 `FSlateBrush`와 픽셀 RECT를 지정한다. 아래 예제의 AtlasBrush에는 미리 텍스처를 지정해야 한다.

```cpp
const RECT ZeroRegion = { 0, 0, 8, 12 };
ImageFont->SetGlyph(L'0', AtlasBrush, 9.0f, FVector2D::Zero, &ZeroRegion);
ImageFont->SetLineHeight(12.0f);
```

색상은 원본 이미지에 Tint를 곱한다. 흰색 Tint이면 원래 색상을 유지한다. 가로 정렬·그림자·줄바꿈을 지원하고 각 문자 이미지는 TextBlock 슬롯에서 잘린다. 이미지 글꼴의 자동 줄바꿈은 문자 단위이며, CR/LF는 줄바꿈, 미등록 공백은 지정한 폭, 미등록 탭은 공백 4개 폭으로 처리한다. 나머지 미등록 문자는 등록된 `?` 이미지로 대체하며, `?`도 없으면 빈 폭을 남긴다. 이미지가 없는 임의의 한글·기호를 자동 생성하거나 문자 조합·커닝을 수행하지 않는다. Windows 글꼴로 돌아가려면 `Label->SetImageFont(TSharedPtr<UImageFont>())`를 호출한다.

기본 WZ HUD는 GameInstance의 `MAPLE_HUD_NUMBER_FOLDER`에서 `0~9`를 읽고 `MAPLE_HUD_NUMBER_SEPARATOR`를 화면의 `/` 문자에 연결한다. StatusBar3의 구분자 Canvas 이름은 역슬래시이며 경로는 `UI/StatusBar3.img/mainBar/status/gauge/number/\`다. DLL이 정규화하기 전에 마지막 `/\` 또는 `\\`를 구분자와 실제 노드 이름으로 나눠 보존하고, `_outlink` 대상도 같은 규칙으로 찾는다.

**WZ HUD의 이미지 숫자는 Canvas API 3인 새 WzNativeLib.dll을 게시·교체해야 한다.** DLL이 2이면 기존 WZ 게이지는 유지하고 숫자는 Windows 글꼴로 표시한다. 숫자나 구분자 하나라도 로딩에 실패하면 숫자 전체를 Windows 글꼴로 대체한다. `[UIImageFont] HUD digits loaded` 로그로 로딩 여부를 확인한다. `MAPLE_HUD_NUMBER_FOLDER = nullptr`로 설정하면 Windows 숫자만 사용한다. 일반 파일·아틀라스 이미지 글꼴에는 DLL 변경이 필요 없다.

HP와 값이 연결된 MP는 같은 이미지 글꼴을 공유한다. 실제 값이 없는 MP의 `-- / --`는 Windows 글꼴을 유지하므로 없는 하이픈 이미지 때문에 글자가 사라지지 않는다. 원본 WZ 숫자에는 별도 그림자를 겹치지 않는다. 장치를 새로 만드는 경우에는 다른 이미지 자원과 마찬가지로 Game에서 이미지 글꼴도 다시 로딩한다.

## StatusBar3 WZ 스킨

사용자가 제공한 `UI.StatusBar3.img.xml`의 다음 노드를 기본 스킨으로 사용한다.

| 역할 | UI\StatusBar3.img 아래 경로 | 원본 origin | 그리기 순서 |
| --- | --- | --- | --- |
| 배경 | mainBar\status\backgrnd | (-2, -24) | 0 |
| HP 채움 | mainBar\status\gauge\hp\layer:0 | (-24, -28) | 2 |
| MP 채움 | mainBar\status\gauge\mp\layer:0 | (-24, -44) | 2 |
| 프레임 | mainBar\status\layer:cover | (1, 0) | 3 |
| HP·MP 숫자 | Game에서 구성한 UTextBlock | 각 게이지 중앙 | 4 |

이 PNG들은 XML에서 1x1이며 `_outlink`가 `UI/_Canvas/StatusBar3.img/...`를 가리킨다. 해당 1x1 참조 픽셀을 늘리는 방식으로는 원본 UI가 나오지 않는다. 새 DLL은 UOL을 먼저 해석하고, `source / _inlink / _outlink`를 따라 실제 PNG를 읽는다. 크기는 실제 PNG에서, origin과 delay는 UOL 해석 후 원본 Canvas에서 읽는다. 실제 `_Canvas` 데이터는 게임이 여는 WZ에 포함되어 있어야 한다.

**WzNativeLib.dll을 다시 게시·교체하고 Game을 재실행해야 한다.** `wz_read_canvas`의 기존 8개 인자는 유지하며 현재 `wz_canvas_api_version`은 3이다. UI 이미지 로더는 2 이상, WZ HUD의 특수 구분자를 포함한 이미지 숫자는 3 이상을 요구한다. 버전 1이면 단색 HP·MP바, 버전 2이면 WZ 게이지와 Windows 숫자를 사용한다. DLL의 기존 Canvas 링크 구현 파일은 `WzExports.cs`, `CanvasResources.cs`이고 이번 이미지 글꼴에서 변경한 DLL 파일은 `CanvasResources.cs`다.

GameInstance의 설정으로 리소스를 바꾼다.

```cpp
static constexpr bool MAPLE_USE_WZ_HUD = true;
static const char* MAPLE_HUD_BACKGROUND_NODE = R"(UI\StatusBar3.img\mainBar\status\backgrnd)";
static const char* MAPLE_HUD_HEALTH_NODE = R"(UI\StatusBar3.img\mainBar\status\gauge\hp\layer:0)";
static const char* MAPLE_HUD_MANA_NODE = R"(UI\StatusBar3.img\mainBar\status\gauge\mp\layer:0)";
static const char* MAPLE_HUD_FRAME_NODE = R"(UI\StatusBar3.img\mainBar\status\layer:cover)";
```

`MAPLE_USE_WZ_HUD = false`는 단색 HP·MP 디자인이다. `FMapleHUDWzStyle`은 Game 타입이며 경로·하단 위치·ZOrder를 설정한다. 기본 ZOrder 0/2/3은 제공된 XML에서 확인한 값이며 일반 WZ 이미지의 Z를 자동 조회하는 기능은 아니다. X 기본 오프셋은 0이고 하단 여백은 24 UI 단위다.

`UMapleHUD::InitializeFromWz()`는 이미지들을 원본 크기로 만들고 `-origin` 위치에 겹친다. 배경·HP·MP·프레임 네 이미지의 합집합으로 크기를 계산하므로 MP가 다른 부품 밖으로 돌출되어도 중앙과 하단 여백에 포함된다. 게이지 배경은 투명하고 프레임은 게이지 앞에 그린다. HP는 실제 체력 비율에 따라 자르고, MP는 현재 표시용 기본값 100%를 사용한다. 이 작업은 StatusBar3 전체 UI나 실제 마나 시스템을 구현하지 않는다.

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

단색 모드의 디자인은 `Game/Include/UI/UMapleHUD.cpp`의 `Initialize()`에서 바꾼다. 패널은 320x64이며 위쪽에는 빨간 HP, 아래쪽에는 파란 MP를 각각 별도 슬롯에 배치한다. 두 슬롯의 생성 실패를 각각 검사한다. 가로는 화면 중앙, 세로는 하단에서 24 UI 단위 떨어진 위치이며 어두운 배경과 테두리를 사용한다. WZ 모드는 `InitializeFromWz()`가 실제 이미지 크기와 원점을 사용하며 기본 위치는 같다.

```cpp
pSlot->SetAnchors(FAnchors(0.5f, 1.0f));
pSlot->SetAlignment(FVector2D(0.5f, 1.0f));
pSlot->SetPosition(FVector2D(0.0f, -24.0f));
pSlot->SetSize(FVector2D(320.0f, 64.0f));
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
- `MAPLE_USE_WZ_HUD = false`로 위쪽 빨간 HP·아래쪽 파란 MP가 겹치지 않는지 확인한다. F1/F2는 빨간 HP만 갱신하며 현재 MP 표시값은 유지한다.
- MP 이미지가 다른 부품보다 넓거나 아래로 돌출되는 스킨에서도 네 부품 전체가 중앙 배치와 하단 여백 계산에 포함되는지 확인한다.
- Debug 실행 시 F1로 피해를 주면 붉은 HP바가 줄어들고 F2로 초기화하면 다시 채워지는지 확인한다. 무적 시간 동안 추가 피해 입력은 기존 규칙대로 무시된다.
- WZ와 단색 모드에서 HP 숫자가 게이지와 함께 갱신되고 MP는 `-- / --`로 남는지 확인한다. 0 HP도 `0 / 최대값`으로 표시되어야 한다.
- 위 텍스트 예제로 한글 글꼴, 정렬, 그림자와 UI 배율을 확인한다. 글꼴 초기화 오류는 `[UIText]` 로그에 표시된다.
- API 3 DLL에서 `[UIImageFont] HUD digits loaded` 로그와 HP 숫자의 원본 이미지 출력을 확인한다. F1/F2로 숫자가 바뀌어도 숫자 로딩 로그가 반복되지 않아야 한다.
- 일반 파일·아틀라스 예제로 같은 글자의 반복, 가변 폭, 이미지 배율, 좌/중앙/우 정렬과 슬롯 경계 자르기를 확인한다. 등록하지 않은 문자의 `?` 대체 또는 빈 폭도 확인한다.
- 카메라 이동·줌에도 HP바가 화면 하단의 가로 중앙에 남는지 확인한다.
- 시작 해상도를 바꿔 앵커·배율을 확인하고, 체력 0·전체 체력과 종료 시 자원 해제를 확인한다.

## HUD 전체 깜박임 진단

2026-11-15 사용자가 배경·게이지·숫자가 함께 간헐적으로 깜박인다고 보고했다. 정지 상태에서도 발생하는지는 아직 확인하지 못했다. 소스 비교에서 main의 UI 렌더 순서와 배포 헤더 일치는 확인했으나 실제 깜박임 원인은 확정하지 않았다. 이 보고를 숫자 이미지 로딩 실패나 텍스처 수명 문제로 단정하지 않는다.

Debug에서 상태가 바뀌는 순간만 다음 로그를 출력한다. 렌더링 동작이나 HUD 표시 정책은 변경하지 않았다.

- `[MapleHUD] hidden: player unavailable`: Tick에서 조작 중인 Maple 캐릭터를 얻지 못해 HUD를 숨긴 시점이다. 컨트롤러와 조작 대상 포인터를 함께 기록한다.
- `[MapleHUD] visible`: 캐릭터를 다시 찾아 숨겨진 HUD를 표시한 시점이다.
- `[UIFrame] no UI entries`: 직전에는 있었던 UI 항목이 현재 FlushUI에서 0개가 된 프레임이다.
- `[UIFrame] UI entries available`: 첫 UI 출력 또는 빈 프레임 뒤 UI 항목이 다시 생긴 시점이다.

깜박임과 hidden/visible 로그가 겹치면 플레이어 조회·Possess 수명을 조사한다. UI 항목만 사라지면 위젯 표시와 제출 경로를 확인한다. 깜박여도 전환 로그가 없으면 UI 항목은 유지되는 것으로 보고, 영상과 실행 중 그래픽 상태를 추가 확인한다. 이 로그는 GPU가 실제 화면에 그렸다는 검증은 아니다. 코드는 Codex 작업 브랜치에 추가했으며 main은 읽기만 했다. 빌드·실행은 하지 않았다.

사용자가 제공한 `2026-11-15 16-59-55.mp4`의 약 3.55초에서 HUD 영역이 잠깐 흰색 사각형으로 덮이고 바로 정상 이미지로 돌아오는 프레임을 확인했다. 해당 프레임의 주변 맵은 유지된다. 함께 제공된 로그에서는 WZ 부품·숫자·구분자가 정상 크기로 로딩되고 첫 프레임 UI 항목은 15개였으며, 숨김이나 빈 UI 프레임 기록은 없다. 이 자료만으로 흰색 대체 브러시, 블렌딩 또는 GPU 자원 문제 중 하나를 원인으로 확정할 수는 없다.

추가 Debug 진단으로 `FDXDevice::LogDebugMessages()`가 `ID3D11InfoQueue`의 경고·오류를 기존 파일 로그에 전달한다. `UEngine::Render()`의 UI 출력 뒤에 호출하며, 같은 메시지 ID는 한 번만 기록한다. 초기화 시 `[D3D11] debug message logging enabled`이면 수집이 활성화된 것이다. 디버그 레이어를 사용할 수 없으면 `[D3D11] debug layer unavailable`을 기록하고 기존 렌더링을 계속한다. 다음 실행 로그에서 `[D3D11] id=...`를 확인한다. 디버그 레이어가 경고하지 않는 그래픽 오류도 있을 수 있으며, 경고가 없다고 깜박임이 해결됐다는 뜻은 아니다. 이번 변경은 진단 연결이며 깜박임 수정 완료로 기록하지 않는다.

추가 사용자 로그 `engine_2026-11-15_17-28-23.log`에서는 Direct3D 메시지 수집이 활성화됐고 경고·오류는 없었다. 사용자는 Debug에서만 발생하는 것 같다고 설명했다. 이에 Debug 전용 충돌 표시를 월드·HUD의 SpriteBatch에서 분리했다. 원인이 이 배치라고 확정한 것은 아니며, 아래 변경 후 사용자 재현 확인이 필요하다.

- `Render/FDebugDraw.h / .cpp`: 엔진이 소유하는 Debug 전용 PrimitiveBatch와 BasicEffect. 별도 정점·인덱스 버퍼에 색상 삼각형을 제출하며 텍스처는 사용하지 않는다. 선분 두께와 접촉 빨강/비접촉 초록을 유지하고 비정상 좌표는 건너뛴다.
- `Engine.h / .cpp`: 월드 → 전용 충돌 디버그 배치 → HUD 순서를 유지한다. Debug 렌더러는 장치보다 먼저 해제한다.
- `PhysicsWorld.h / .cpp`: DrawDebug가 FDebugDraw에 선분을 전달한다.
- `SpriteBatch.h / .cpp`: 충돌 표시용 흰색 텍스처와 DrawDebugLine 경로를 제거했다. 기존 월드·HUD 이미지 출력은 유지한다.
- `Engine.vcxproj / .filters`: 새 FDebugDraw 헤더·소스를 렌더 파이프라인 필터에 등록한다. main 수동 반영 시 기존 다른 등록을 덮어쓰지 말고 두 항목을 추가한다.

`FEngineInitParams::m_bDrawCollisionDebug` 기본값은 true다. Game의 초기화 설정에서 다음 한 줄을 넣으면 Debug 빌드의 충돌 표시만 제외한다. Direct3D 디버그 레이어·Debug 게임 로직·기존 파일 로그는 계속 사용할 수 있다. Release에서는 이 옵션을 사용하지 않는다.

```cpp
FEngineInitParams Params;
Params.m_bDrawCollisionDebug = false;
```

분리한 충돌 표시를 켠 상태에서 재현 여부를 먼저 확인하고, 계속 발생하면 false로 바꿔 충돌 패스 자체를 제외해 비교한다. 이 코드는 Codex 기존 브랜치에만 반영했고 main·Test·생성된 GameEngine에는 쓰지 않았다. 빌드·실행은 하지 않았으며 실제 해결 여부는 사용자 확인 대기다.

현재는 최소 HP·MP 표시와 텍스트·HP 숫자까지 소스에 구현했다. 실제 마나 수치 연동·경험치·마우스 히트 테스트·버튼·인벤토리와 UI 클리핑/자동 크기는 후속 작업이다. Hidden과 Collapsed는 현재 절대 배치 Canvas에서 모두 그리지 않으며 흐름 레이아웃의 공간 예약 차이는 아직 없다.
