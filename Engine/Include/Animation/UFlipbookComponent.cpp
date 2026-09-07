#include "EnginePCH.h"
#include "Animation/UFlipbookComponent.h"
#include "Object/AActor.h"
#include "Render/USpriteComponent.h"
#include "Animation/UAnimNotify.h"

UFlipbookComponent::UFlipbookComponent()
{
}

UFlipbookComponent::~UFlipbookComponent()
{
	for (int32 i = 0; i < m_Frames.Num(); i++)
	{
		m_Frames[i].m_pTexture->Release();
	}
}

void UFlipbookComponent::SetFrames(const TArray<FFlipbookFrame>& Frames, bool bLoop)
{
#ifdef _DEBUG
	// 진단용 — Idle<->Move 반복 전환 중 발생하는 크래시 원인 규명 임시 코드.
	// AddRef() 직후 바로 Release()해서 실제 부작용 없이 "현재 참조 카운트"만
	// 확인한다(COM AddRef/Release는 갱신된 카운트를 반환값으로 돌려줌).
	{
		wchar_t Buf[256];
		swprintf_s(Buf, L"[Flipbook] SetFrames: 기존 m_Frames=%d개, 새로 받은 Frames=%d개\n", m_Frames.Num(), Frames.Num());
		OutputDebugStringW(Buf);
	}
#endif

	for (int32 i = 0; i < m_Frames.Num(); i++)
	{
		ID3D11ShaderResourceView* pTex = m_Frames[i].m_pTexture;
		ULONG After = pTex->Release();
#ifdef _DEBUG
		// 진단용 — Idle<->Move 반복 전환 크래시 원인 규명 임시 코드. 관찰용
		// AddRef/Release 별도 호출이 아니라, 실제 코드가 하는 이 Release() 자체의
		// 반환값(=이 호출 직후의 진짜 참조 카운트)을 그대로 찍는다.
		wchar_t Buf[256];
		swprintf_s(Buf, L"[Flipbook]   실제 Release 기존[%d] tex=%p 이 Release 후 refcount=%lu\n", i, (void*)pTex, After);
		OutputDebugStringW(Buf);
#endif
	}

	m_Frames.Reset();

	for (int32 i = 0; i < Frames.Num(); i++)
	{
		ULONG After = Frames[i].m_pTexture->AddRef();
#ifdef _DEBUG
		wchar_t Buf[256];
		swprintf_s(Buf, L"[Flipbook]   실제 AddRef 신규[%d] tex=%p 이 AddRef 후 refcount=%lu\n", i, (void*)Frames[i].m_pTexture, After);
		OutputDebugStringW(Buf);
#endif

		FFlipbookFrame Frame = Frames[i];

		if (Frame.m_Duration <= 0.0f)
		{
			Frame.m_Duration = 0.001f;
		}

		m_Frames.Add(Frame);
	}

	m_bLoop = bLoop;
	m_CurrentFrameIndex = 0;
	m_ElapsedInFrame = 0.0f;
}

void UFlipbookComponent::Play()
{
	m_bPlaying = m_Frames.Num() > 0;
}

void UFlipbookComponent::Stop()
{
	m_bPlaying = false;
}

void UFlipbookComponent::BeginPlay()
{
	m_pTargetSprite = GetOwner() ? GetOwner()->GetComponent<USpriteComponent>() : nullptr;
}

void UFlipbookComponent::Tick(float DeltaTime)
{
	if (!m_bPlaying || m_Frames.Num() == 0)
	{
		return;
	}

	m_ElapsedInFrame += DeltaTime;

	const FFlipbookFrame* pCurrent = &m_Frames[m_CurrentFrameIndex];
	while (m_ElapsedInFrame >= pCurrent->m_Duration)
	{
		m_ElapsedInFrame -= pCurrent->m_Duration;

		if (m_CurrentFrameIndex + 1 < m_Frames.Num())
		{
			m_CurrentFrameIndex++;
		}

		else if (m_bLoop)
		{
			m_CurrentFrameIndex = 0;
		}

		else
		{
			m_bPlaying = false;
			m_ElapsedInFrame = 0.0f;
			break;
		}

		pCurrent = &m_Frames[m_CurrentFrameIndex];

		if (pCurrent->m_pNotify)
		{
			pCurrent->m_pNotify->Notify(GetOwner());
		}
	}

	if (m_pTargetSprite)
	{
		ULONG After = pCurrent->m_pTexture->AddRef();

#ifdef _DEBUG
		// 진단용 — Idle<->Move 반복 전환 크래시 원인 규명 임시 코드. 프레임
		// 인덱스가 바뀔 때만(스팸 방지) 표시 직전 refcount를 찍는다.
		static int32 s_LastLoggedIndex = -1;
		if (m_CurrentFrameIndex != s_LastLoggedIndex)
		{
			s_LastLoggedIndex = m_CurrentFrameIndex;
			wchar_t Buf[256];
			swprintf_s(Buf, L"[Flipbook] Tick 실제 AddRef: frameIndex=%d tex=%p 이 AddRef 후 refcount=%lu\n", m_CurrentFrameIndex, (void*)pCurrent->m_pTexture, After);
			OutputDebugStringW(Buf);
		}
#endif

		m_pTargetSprite->SetTexture(pCurrent->m_pTexture, pCurrent->m_Origin);
	}
}