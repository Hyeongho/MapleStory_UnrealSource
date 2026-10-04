#include "EnginePCH.h"
#include "UMapleGameInstance.h"
#include "Engine.h"
#include "AMaplePlayerController.h"
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
	FMapLoader::LoadMap(Engine.GetDevice(), Engine.GetWorld(), Engine.GetCamera(), MAPLE_WZ_PATH, MAPLE_MAP_PATH);

	InitPlayer();

	// 시작 위치만 지정하며 이후에는 방향키 입력으로 카메라를 움직인다.
	Engine.GetCamera().SetLocation(FVector2D(100.0f, 300.0f));

	return true;
}

void UMapleGameInstance::Shutdown()
{
	if (m_pPlayerCharacter)
	{
		if (APlayerController* pController = GetEngine().GetWorld().GetFirstPlayerController())
		{
			pController->UnPossess();
		}
	}

	m_pAnimStateMachine = nullptr;
	m_pPlayerCharacter = nullptr;

	Super::Shutdown();
}

APlayerController* UMapleGameInstance::CreatePlayerController(UWorld& World)
{
	return World.SpawnActor<AMaplePlayerController>();
}

void UMapleGameInstance::InitPlayer()
{
	UEngine& Engine = GetEngine();
	m_pPlayerCharacter = Engine.GetWorld().SpawnActor<ACharacter>();
	m_pPlayerCharacter->LoadAvatar(Engine.GetDevice(), MAPLE_WZ_PATH, MAPLE_LOADOUT_SPEC, "stand1", 0);
	m_pPlayerCharacter->SetLocation(FVector2D(-300.0f, 50.0f));

	// 물리 연동 전에는 시작 위치 아래의 발판으로 렌더링 레이어만 선택한다.
	if (FMapScene* pMapScene = Engine.GetWorld().GetMapScene())
	{
		const int32 InitialFootholdId = pMapScene->FindFootholdBelow(FVector2D(-300.0f, 100.0f));
		pMapScene->SetCharacterFoothold(*m_pPlayerCharacter, InitialFootholdId);
	}

	m_pPlayerCharacter->AddComponent<UFlipbookComponent>();
	m_pAnimStateMachine = m_pPlayerCharacter->AddComponent<UAnimStateMachine>();

	const bool bHasMove = RegisterAvatarState(FName(L"Move"), "walk1");
	const bool bHasIdle = RegisterAvatarState(FName(L"Idle"), "stand1");

	if (bHasIdle)
	{
		m_pAnimStateMachine->SetState(FName(L"Idle"));
	}

	else if (bHasMove)
	{
		m_pAnimStateMachine->SetState(FName(L"Move"));
	}

	// 카메라는 캐릭터를 화면 중심에 두고 맵의 VR 경계 안에서 따라간다.
	if (APlayerController* pController = Engine.GetWorld().GetFirstPlayerController())
	{
		pController->Possess(m_pPlayerCharacter);
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