# 일반 이미지 파일 로딩

## 역할과 지원 범위

`Engine/Include/Render/TextureLoader.h/.cpp`의 FTextureLoader는 일반 이미지 파일을 GPU의 정적 2D 텍스처로 읽는다.
파일 경로와 표시 위치는 Game에서 지정하고, Engine은 DirectXTK WICTextureLoader로 디코딩·업로드한다.
기존 FWzTextureLoader의 WZ Canvas·아바타 로딩과 동일한 ID3D11ShaderResourceView 포인터를 반환한다.

PNG, JPEG(JPG), BMP, GIF, TIFF, HD Photo 등 Windows WIC가 지원하는 파일을 사용할 수 있다.
추가 형식의 지원 여부는 설치된 WIC 코덱에 따라 달라진다. 모든 확장자를 일괄 지원하는 로더는 아니다.
다중 프레임 GIF/TIFF 등은 첫 프레임을 정적 이미지로 읽는다. 프레임별 애니메이션은 별도의 UFlipbook 구성으로 처리한다.
반환 크기는 생성된 GPU 텍스처의 실제 크기이며, 하드웨어 제한에 따라 원본보다 작아질 수 있다.

## Game 사용 예시

UMapleGameInstance의 Init에서 Engine 초기화가 끝난 뒤 다음처럼 사용할 수 있다.
아래 경로는 실제로 존재하는 이미지 파일의 경로로 바꾼다.

```cpp
#include "Render/TextureLoader.h"
#include "Render/USpriteComponent.h"
#include "World/UWorld.h"

int32 Width = 0;
int32 Height = 0;
ID3D11ShaderResourceView* pTexture = FTextureLoader::LoadTextureFromFile(
    Engine.GetDevice(), LR"(D:\Images\sample.jpg)", &Width, &Height);

if (pTexture)
{
    AActor* pImageActor = Engine.GetWorld().SpawnActor<AActor>();
    USpriteComponent* pSprite = pImageActor->AddComponent<USpriteComponent>();
    pSprite->SetTexture(pTexture);
    pSprite->SetLayer(ELayer::UI);
    pSprite->SetRelativeTransform(FTransform2D(
        FVector2D(200.0f, 150.0f), 0.0f, FVector2D(1.0f, 1.0f)));
    // 텍스처 참조를 Sprite에 이전했으므로 여기서 Release하지 않는다.
}
```

FilePath는 wchar_t 경로이므로 한글 파일명도 전달할 수 있다.
절대 경로와 상대 경로를 모두 사용할 수 있으며, 상대 경로는 프로세스의 현재 작업 폴더를 기준으로 한다.
Visual Studio의 디버깅 작업 디렉터리 설정에 따라 상대 경로의 기준이 달라질 수 있다.

파일 없음, 미지원 형식, 손상된 데이터, 미초기화 그래픽 디바이스 등은 nullptr로 반환하고 출력 크기는 0으로 유지한다.
실패 사유는 `[TextureLoader]` 로그의 경로와 HRESULT로 확인할 수 있다. 기존 텍스처를 유지하려면 성공했을 때만 교체한다.

## 소유권과 표시

반환 포인터는 호출자가 Release할 참조 1개다.
USpriteComponent::SetTexture는 이 참조를 인수하고 교체·소멸 시 해제한다.
여러 스프라이트에 같은 포인터를 전달할 때는 추가 소비자마다 AddRef로 참조를 확보해야 한다.
UFlipbook::Create는 프레임 텍스처의 참조를 별도로 확보하므로, 생성 후 로더에서 받은 참조는 호출자가 Release한다.

파일 로딩은 초기화나 리소스 교체 시에만 호출하고 생성된 텍스처를 재사용한다. 호출할 때마다 새 텍스처가 생성된다.
현재 WZ·SpriteBatch의 UNORM 색상 경로에 맞춰 sRGB 자동 변환을 끄고, 비프리멀티플라이 RGBA 알파를 사용한다.

## 초기화와 검증

UEngine::Init은 COM 초기화를 요청하며 성공한 호출은 Shutdown에서 한 번 해제한다. S_FALSE도 성공으로 처리한다.
이미 다른 apartment로 초기화된 스레드의 RPC_E_CHANGED_MODE는 기존 COM 환경을 사용하고, 타인이 한 초기화를 해제하지 않는다.
Init과 Shutdown은 같은 게임 스레드에서 호출한다. 독립 FDXDevice나 별도 로딩 스레드에서 사용할 경우 해당 스레드가 COM 초기화를 준비해야 한다.

VS 프로젝트와 Render 필터에 새 파일을 등록했다. 빌드·실행은 하지 않았으며 Test 프로젝트는 변경하지 않았다.

구현 참고: [DirectXTK WICTextureLoader](https://github.com/microsoft/DirectXTK/wiki/WICTextureLoader),
[CoInitializeEx](https://learn.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-coinitializeex).
