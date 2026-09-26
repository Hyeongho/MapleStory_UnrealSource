#include "EnginePCH.h"
#include "UMapleGameInstance.h"
#include "Engine.h"
#include "Object/ACharacter.h"
#include "Animation/UFlipbookComponent.h"
#include "Animation/UAnimStateMachine.h"
#include "Render/FCamera2D.h"
#include "Render/WzTextureLoader.h"
#include "Timer/FTimerManager.h"
#include "World/UWorld.h"
#include "World/FMapLoader.h"
#include "World/FMapScene.h"

static const char* MAPLE_WZ_PATH = R"(C:\Nexon\Maple\Data\Base\Base.wz)";
static const char* MAPLE_MAP_PATH = R"(Map\Map\Map2\200000100.img)";
static const char* MAPLE_LOADOUT_SPEC = "2015,12015,53003,65007,,,1054087,,1073816,,,,1703431,,,,,,,,,,,,,,,,";

UMapleGameInstance::UMapleGameInstance() = default;

UMapleGameInstance::~UMapleGameInstance() = default;

bool UMapleGameInstance::Init(UEngine& Engine)
{
	if (!Super::Init(Engine))
	{
		return false;
	}

	// 게임이 맵을 선택하면 해당 월드가 맵 씬을 생성하고 소유한다.
	FMapLoader::LoadMap(Engine.GetDevice(), Engine.GetWorld(), MAPLE_WZ_PATH, MAPLE_MAP_PATH);
	InitPlayer();

	// 구름 13번을 확인하던 기존 카메라 위치를 유지한다.
	Engine.GetCamera().SetLocation(FVector2D(100.0f, 300.0f));
	return true;
}

void UMapleGameInstance::Shutdown()
{
	// 콜백이 게임 인스턴스를 참조하므로 엔진의 타이머가 살아 있을 때 취소한다.
	if (m_AnimDemoToggleHandle.IsValid())
	{
		GetEngine().GetTimerManager().ClearTimer(m_AnimDemoToggleHandle);
	}
	m_pAnimStateMachine = nullptr;
	m_pPlayerCharacter = nullptr;
	m_bMoving = false;
	Super::Shutdown();
}

void UMapleGameInstance::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// 카메라 정책은 게임에서 결정하며, 엔진은 이 갱신 뒤에 렌더링한다.
	GetEngine().GetCamera().SetLocation(FVector2D(100.0f, 300.0f));
}

void UMapleGameInstance::InitPlayer()
{
	UEngine& Engine = GetEngine();
	m_pPlayerCharacter = Engine.GetWorld().SpawnActor<ACharacter>();
	m_pPlayerCharacter->LoadAvatar(Engine.GetDevice(), MAPLE_WZ_PATH, MAPLE_LOADOUT_SPEC, "stand1", 0);
	m_pPlayerCharacter->SetLocation(FVector2D(-300.0f, 100.0f));

	// 물리 연동 전에는 시작 위치 아래의 발판으로 렌더링 레이어만 선택한다.
	if (FMapScene* pMapScene = Engine.GetWorld().GetMapScene())
	{
		const int32 InitialFootholdId = pMapScene->FindFootholdBelow(FVector2D(-300.0f, 100.0f));
		pMapScene->SetCharacterFoothold(*m_pPlayerCharacter, InitialFootholdId);
	}

	m_pPlayerCharacter->AddComponent<UFlipbookComponent>();
	m_pAnimStateMachine = m_pPlayerCharacter->AddComponent<UAnimStateMachine>();
	const bool bHasMove = RegisterAvatarState(FName(L"Move"), "swingT3");
	const bool bHasIdle = RegisterAvatarState(FName(L"Idle"), "stand1");

	if (bHasIdle)
	{
		m_pAnimStateMachine->SetState(FName(L"Idle"));
	}

	else if (bHasMove)
	{
		m_pAnimStateMachine->SetState(FName(L"Move"));
	}

	if (bHasIdle && bHasMove)
	{
		Engine.GetTimerManager().SetTimer(m_AnimDemoToggleHandle, FTimerDelegate::CreateRaw<UMapleGameInstance, &UMapleGameInstance::ToggleAnimDemoState>(this), 2.0f, true);
	}
}

bool UMapleGameInstance::RegisterAvatarState(FName StateName, const char* ActionName)
{
	TArray<FFlipbookFrame> Frames;
	for (int32 i = 0; ; i++)
	{
		FAvatarTexture Frame = FWzTextureLoader::LoadAvatarTexture(GetEngine().GetDevice(), MAPLE_WZ_PATH, MAPLE_LOADOUT_SPEC, ActionName, i);
		if (!Frame.m_pTexture)
		{
			break;
		}
		Frames.Add(FFlipbookFrame{ Frame.m_pTexture, Frame.m_Origin, Frame.m_DelayMs / 1000.0f });
	}

	if (Frames.Num() == 0)
	{
		return false;
	}

	m_pAnimStateMachine->RegisterState(StateName, Frames, true);

	// 상태 머신이 AddRef한 뒤에는 로딩 과정에서 보유하던 참조를 즉시 반납한다.
	for (int32 i = 0; i < Frames.Num(); i++)
	{
		Frames[i].m_pTexture->Release();
	}

	return true;
}

void UMapleGameInstance::ToggleAnimDemoState()
{
	if (!m_pAnimStateMachine || !m_pPlayerCharacter)
	{
		return;
	}

	m_bMoving = !m_bMoving;
	m_pAnimStateMachine->SetState(m_bMoving ? FName(L"Move") : FName(L"Idle"));
	m_pPlayerCharacter->SetFacingRight(m_bMoving);
}