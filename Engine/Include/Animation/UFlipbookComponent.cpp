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

		for (int32 i = 0; i < m_Frames.Num(); i++)
		{
			ULONG Ref = m_Frames[i].m_pTexture->AddRef();
			m_Frames[i].m_pTexture->Release();
			swprintf_s(Buf, L"[Flipbook]   해제 예정 기존[%d] tex=%p 현재refcount=%lu\n", i, (void*)m_Frames[i].m_pTexture, Ref);
			OutputDebugStringW(Buf);
		}

		for (int32 i = 0; i < Frames.Num(); i++)
		{
			ULONG Ref = Frames[i].m_pTexture->AddRef();
			Frames[i].m_pTexture->Release();
			swprintf_s(Buf, L"[Flipbook]   새로 받을 incoming[%d] tex=%p 현재refcount=%lu\n", i, (void*)Frames[i].m_pTexture, Ref);
			OutputDebugStringW(Buf);
		}
	}
#endif

	for (int32 i = 0; i < m_Frames.Num(); i++)
	{
		m_Frames[i].m_pTexture->Release();
	}
	m_Frames.Reset();

	for (int32 i = 0; i < Frames.Num(); i++)
	{
		Frames[i].m_pTexture->AddRef();

		FFlipbookFrame Frame = Frames[i];
		if (Frame.m_Duration <= 0.0f)
		{
			// WZ 딜레이(외부 데이터) 값이 0/음수로 들어올 가능성에 대한 방어 —
			// Duration이 0 이하로 남으면 Tick()의 프레임 전환 while 루프가
			// 절대 안 끝나는 무한 루프가 된다(m_ElapsedInFrame이 매 반복 그대로라
			// 조건이 계속 참). 화면에 티도 안 날 만큼 짧은 최소값으로만 올려둔다.
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

		// 이 프레임으로 "전환되는" 순간 정확히 한 번 발동. 이 while 루프 바디는
		// 위 두 분기(++ 또는 0으로 wrap) 중 하나를 실제로 거쳐야만 여기 도달하므로
		// (정지 분기는 이 줄 전에 break) — 한 Tick 안에서 DeltaTime이 커서 여러
		// 프레임을 건너뛰면 건너뛴 프레임 각각의 Notify가 빠짐없이 발동하고,
		// 반대로 같은 프레임에 계속 머무르는 Tick에서는 while 조건 자체가 거짓이라
		// 이 줄이 전혀 실행되지 않아 반복 발동하지 않는다.
		if (pCurrent->m_pNotify)
		{
			pCurrent->m_pNotify->Notify(GetOwner());
		}
	}

	if (m_pTargetSprite)
	{
#ifdef _DEBUG
		// 진단용 — Idle<->Move 반복 전환 크래시 원인 규명 임시 코드. 프레임
		// 인덱스가 바뀔 때만(스팸 방지) 표시 직전 refcount를 찍는다.
		static int32 s_LastLoggedIndex = -1;
		if (m_CurrentFrameIndex != s_LastLoggedIndex)
		{
			s_LastLoggedIndex = m_CurrentFrameIndex;
			ULONG Ref = pCurrent->m_pTexture->AddRef();
			pCurrent->m_pTexture->Release();
			wchar_t Buf[256];
			swprintf_s(Buf, L"[Flipbook] Tick: frameIndex=%d tex=%p 표시 직전 refcount=%lu (표시 후 +1)\n", m_CurrentFrameIndex, (void*)pCurrent->m_pTexture, Ref);
			OutputDebugStringW(Buf);
		}
#endif
		pCurrent->m_pTexture->AddRef();
		m_pTargetSprite->SetTexture(pCurrent->m_pTexture, pCurrent->m_Origin);
	}
}
