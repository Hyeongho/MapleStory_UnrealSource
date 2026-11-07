#pragma once

#include "Object/UObject.h"
#include "Core/Containers/TArray.h"
#include "Core/SmartPointer/TSharedPtr.h"
#include "Core/Math/FVector2D.h"

class UAnimNotify;

// WZ에서 읽은 프레임 정보. 입력 배열의 텍스처 참조는 호출자가 관리한다.
struct FFlipbookFrame
{
	ID3D11ShaderResourceView* m_pTexture = nullptr;
	FVector2D m_Origin = FVector2D::Zero;
	float m_Duration = 0.1f;
	UAnimNotify* m_pNotify = nullptr; // 비소유
	int32 m_A0 = 255;
	int32 m_A1 = 255;
	bool m_bBlend = false;
	int32 m_Z = 0;
};

// 여러 재생 컴포넌트가 공유하는 불변 애니메이션 데이터다.
// 현재 엔진에는 GC가 없으므로 에셋 수명은 TSharedPtr로 관리한다.
class UFlipbook : public UObject
{
	DECLARE_CLASS(UFlipbook, UObject)

public:
	// 생성 / 소멸
	UFlipbook();
	virtual ~UFlipbook() override;
	UFlipbook(const UFlipbook&) = delete;
	UFlipbook& operator=(const UFlipbook&) = delete;

	// 전체 입력 검사 후 텍스처 참조를 한 번씩 확보한다. 실패하면 빈 포인터를 반환한다.
	static TSharedPtr<UFlipbook> Create(const TArray<FFlipbookFrame>& Frames);

	// 프레임 데이터 조회. 반환 포인터는 이 에셋이 살아 있는 동안 유효하다.
	int32 GetNumFrames() const;
	const FFlipbookFrame* GetFrame(int32 FrameIndex) const;
	double GetTotalDuration() const;

private:
	TArray<FFlipbookFrame> m_Frames;
	double m_TotalDuration = 0.0;
};
