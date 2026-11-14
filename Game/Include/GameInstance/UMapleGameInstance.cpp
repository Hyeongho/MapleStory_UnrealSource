#include "EnginePCH.h"
#include "UMapleGameInstance.h"
#include "Engine.h"
#include "../Controller/AMaplePlayerController.h"
#include "../Character/AMapleCharacter.h"
#include "Render/FCamera2D.h"

#include "World/UWorld.h"
#include "World/FMapLoader.h"
#include "World/FMapScene.h"

#include "Physics/PhysicsWorld.h"
#include "Physics/URigidbody.h"

#include "Audio/FAudioManager.h"
#include "Audio/WzSoundLoader.h"

static const char* MAPLE_WZ_PATH = R"(C:\Nexon\Maple\Data\Base\Base.wz)";
static const char* MAPLE_MAP_PATH = R"(Map\Map\Map4\410017300.img)";
static const char* MAPLE_LOADOUT_SPEC = "2015,12015,53003,65007,,,1054087,,1073816,,,,1703431,,,,,,,,,,,,,,,,";

// 사용할 오디오 파일의 절대 경로 또는 실행 작업 폴더 기준 경로를 지정한다.
// 예: LR"(D:\MapleAssets\Audio\BGM.mp3)". nullptr이면 해당 사운드를 사용하지 않는다.
static const wchar_t* MAPLE_BGM_FILE = nullptr;
static const wchar_t* MAPLE_ATTACK_SOUND_FILE = nullptr;

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

	InitAudio();

	// 시작 위치만 지정하며 이후에는 방향키 입력으로 카메라를 움직인다.
	Engine.GetCamera().SetLocation(FVector2D(100.0f, 300.0f));

	return true;
}

void UMapleGameInstance::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const APlayerController* pController = GetEngine().GetWorld().GetFirstPlayerController();

	AMapleCharacter* pCharacter = pController ? Cast<AMapleCharacter>(pController->GetCharacter()) : nullptr;

	if (pCharacter)
	{
		pCharacter->UpdateAnimation();
	}
}

APlayerController* UMapleGameInstance::CreatePlayerController(UWorld& World)
{
	return World.SpawnActor<AMaplePlayerController>();
}

void UMapleGameInstance::InitPlayer()
{
	UEngine& Engine = GetEngine();

	AMapleCharacter* pPlayerCharacter = Engine.GetWorld().SpawnActor<AMapleCharacter>();

	pPlayerCharacter->InitializeAvatar(Engine.GetDevice(), MAPLE_WZ_PATH, MAPLE_LOADOUT_SPEC);
	pPlayerCharacter->SetLocation(FVector2D(-2.0f, -23.0f));

	// 시작점 아래의 실제 맵 발판에 발을 놓고, 같은 발판의 렌더링 레이어를 쓴다.
	if (FMapScene* pMapScene = Engine.GetWorld().GetMapScene())
	{
		// 선택한 사다리의 하단 발판에서 시작해 위 입력으로 바로 올라갈 수 있게 한다.
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
					pPlayerCharacter->SetLocation(FVector2D((float)LadderRope.m_X, BottomY));
					bFoundStart = true;

					break;
				}
			}

			if (!bFoundStart)
			{
				UE_LOG(LogCore, Warning, L"[MapLadderRope] start slot %d not found; using default start", MAPLE_START_LADDER_ROPE_INDEX);
			}
		}

		const int32 InitialFootholdId = pMapScene->FindFootholdBelow(pPlayerCharacter->GetLocation());

		if (const FFoothold* Foothold = Engine.GetWorld().GetPhysicsWorld().FindFoothold(InitialFootholdId))
		{
			const FVector2D Location = pPlayerCharacter->GetLocation();
			pPlayerCharacter->SetLocation(FVector2D(Location.m_X, Foothold->GetHeightAtX(Location.m_X)));
			pMapScene->SetCharacterFoothold(*pPlayerCharacter, InitialFootholdId);
		}
	}

	pPlayerCharacter->EnablePhysics(FVector2D(12.0f, 24.0f));

	if (URigidbody* pBody = pPlayerCharacter->GetComponent<URigidbody>())
	{
		pBody->SetClimbReentryDelay(MAPLE_CLIMB_REENTRY_DELAY);
		pBody->SetCoyoteTime(MAPLE_COYOTE_TIME);
	}

	// 카메라는 캐릭터를 화면 중심에 두고 맵의 VR 경계 안에서 따라간다.
	if (APlayerController* pController = Engine.GetWorld().GetFirstPlayerController())
	{
		pController->Possess(pPlayerCharacter);
	}
}

void UMapleGameInstance::InitAudio()
{
}
