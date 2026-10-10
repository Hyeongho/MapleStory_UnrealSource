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
#include "Physics/PhysicsWorld.h"
#include "Physics/URigidbody.h"

static const char* MAPLE_WZ_PATH = R"(C:\Nexon\Maple\Data\Base\Base.wz)";
static const char* MAPLE_MAP_PATH = R"(Map\Map\Map1\100000000.img)";
static const char* MAPLE_LOADOUT_SPEC = "2015,12015,53003,65007,,,1054087,,1073816,,,,1703431,,,,,,,,,,,,,,,,";

// 실제 클라이언트에서 확인할 ladderRope 배치 번호. INDEX_NONE이면 기본 시작점을 쓴다.
static constexpr int32 MAPLE_START_LADDER_ROPE_INDEX = INDEX_NONE;

// 사다리·로프 점프 직후 다시 매달릴 수 있을 때까지의 시간(초).
static constexpr float MAPLE_CLIMB_REENTRY_DELAY = 0.2f;

// 지면에서 떨어진 직후 첫 점프를 허용하는 시간(초).
static constexpr float MAPLE_COYOTE_TIME = 0.1f;

UMapleGameInstance::UMapleGameInstance() = default;

UMapleGameInstance::~UMapleGameInstance() = default;

bool UMapleGameInstance::Init(UEngine& Engine)
{
	if (!Super::Init(Engine))
	{
		return false;
	}

	// 게임이 맵을 선택하면 해당 월드가 맵 씬을 생성하고 소유한다.
	FMapLoadOptions MapOptions;

	MapOptions.m_ExcludedObjects.Emplace("guide", "tutorial");
	FMapLoader::LoadMap(Engine.GetDevice(), Engine.GetWorld(), Engine.GetCamera(), MAPLE_WZ_PATH, MAPLE_MAP_PATH, MapOptions);

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

void UMapleGameInstance::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const APlayerController* pController = GetEngine().GetWorld().GetFirstPlayerController();
	ACharacter* pCharacter = pController ? pController->GetCharacter() : nullptr;
	if (!pCharacter)
	{
		return;
	}

	UAnimStateMachine* pStateMachine = pCharacter->GetComponent<UAnimStateMachine>();
	UFlipbookComponent* pFlipbook = pCharacter->GetComponent<UFlipbookComponent>();
	const URigidbody* pBody = pCharacter->GetComponent<URigidbody>();
	if (!pStateMachine || !pFlipbook || !pBody)
	{
		return;
	}

	FName StateName(L"Idle");
	if (pBody->IsClimbing())
	{
		StateName = pBody->GetClimbableType() == EClimbableType::Ladder ? FName(L"Ladder") : FName(L"Rope");
	}

	else if (!pBody->IsGrounded())
	{
		StateName = FName(L"Jump");
	}

	else if (FMath::Abs(pBody->GetVelocity().m_X) > FMath::KINDA_SMALL_NUMBER)
	{
		StateName = FName(L"Move");
	}

	// 해당 WZ 액션이 없으면 로딩에 성공한 기본 자세를 유지한다.
	if (!pStateMachine->HasState(StateName))
	{
		StateName = pStateMachine->HasState(FName(L"Idle")) ? FName(L"Idle") : FName(L"Move");
	}

	if (pStateMachine->HasState(StateName))
	{
		pStateMachine->SetState(StateName);
		if (pBody->IsClimbing() && FMath::Abs(pBody->GetVelocity().m_Y) <= FMath::KINDA_SMALL_NUMBER)
		{
			// 자세와 재생 위치를 유지한 채 프레임 진행만 멈춘다.
			pFlipbook->Stop();
		}

		else
		{
			pFlipbook->Play();
		}
	}
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
	m_pPlayerCharacter->SetLocation(FVector2D(698.0f, 257.0f));

	// 시작점 아래의 실제 맵 발판에 발을 놓고, 같은 발판의 렌더링 레이어를 쓴다.
	if (FMapScene* pMapScene = Engine.GetWorld().GetMapScene())
	{
		// 선택한 사다리의 상단 발판에서 시작해 아래 입력으로 바로 내려갈 수 있게 한다.
		if (MAPLE_START_LADDER_ROPE_INDEX != INDEX_NONE)
		{
			const TArray<FMapLadderRopeItem>& LadderRopes = pMapScene->GetLadderRopes();
			bool bFoundStart = false;

			for (int32 i = 0; i < LadderRopes.Num(); i++)
			{
				const FMapLadderRopeItem& LadderRope = LadderRopes[i];
				if (LadderRope.m_Index == MAPLE_START_LADDER_ROPE_INDEX)
				{
					// 실제 시작 높이는 하단 아래에서 가장 가까운 발판을 찾아 보정한다.
					const float BottomY = (float)FMath::Max(LadderRope.m_Y1, LadderRope.m_Y2);
					m_pPlayerCharacter->SetLocation(FVector2D((float)LadderRope.m_X, BottomY));
					bFoundStart = true;

					break;
				}
			}

			if (!bFoundStart)
			{
				UE_LOG(LogCore, Warning, L"[MapLadderRope] start slot %d not found; using default start", MAPLE_START_LADDER_ROPE_INDEX);
			}
		}

		const int32 InitialFootholdId = pMapScene->FindFootholdBelow(m_pPlayerCharacter->GetLocation());
		if (const FFoothold* Foothold = Engine.GetWorld().GetPhysicsWorld().FindFoothold(InitialFootholdId))
		{
			const FVector2D Location = m_pPlayerCharacter->GetLocation();

			m_pPlayerCharacter->SetLocation(FVector2D(Location.m_X, Foothold->GetHeightAtX(Location.m_X)));

			pMapScene->SetCharacterFoothold(*m_pPlayerCharacter, InitialFootholdId);
		}
	}

	m_pPlayerCharacter->EnablePhysics(FVector2D(12.0f, 24.0f));

	if (URigidbody* pBody = m_pPlayerCharacter->GetComponent<URigidbody>())
	{
		pBody->SetClimbReentryDelay(MAPLE_CLIMB_REENTRY_DELAY);
		pBody->SetCoyoteTime(MAPLE_COYOTE_TIME);
	}

	m_pPlayerCharacter->AddComponent<UFlipbookComponent>();
	m_pAnimStateMachine = m_pPlayerCharacter->AddComponent<UAnimStateMachine>();

	const bool bHasMove = RegisterAvatarState(FName(L"Move"), "walk1");
	const bool bHasIdle = RegisterAvatarState(FName(L"Idle"), "stand1");

	RegisterAvatarState(FName(L"Jump"), "jump");
	RegisterAvatarState(FName(L"Ladder"), "ladder");
	RegisterAvatarState(FName(L"Rope"), "rope");

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
		UE_LOG(LogRenderer, Warning, L"[PlayerAnimation] avatar action unavailable: %hs", ActionName);
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