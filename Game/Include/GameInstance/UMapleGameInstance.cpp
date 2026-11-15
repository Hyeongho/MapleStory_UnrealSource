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

#include "UI/FUIViewport.h"
#include "../UI/UMapleHUD.h"

static const char* MAPLE_WZ_PATH = R"(C:\Nexon\Maple\Data\Base\Base.wz)";
static const char* MAPLE_MAP_PATH = R"(Map\Map\Map0\000010000.img)";
static const char* MAPLE_LOADOUT_SPEC = "2015,12015,53003,65007,,,1054087,,1073816,,,,1703431,,,,,,,,,,,,,,,,";

// 사용할 오디오 파일의 절대 경로 또는 실행 작업 폴더 기준 경로를 지정한다.
// 예: LR"(D:\MapleAssets\Audio\BGM.mp3)". nullptr이면 해당 사운드를 사용하지 않는다.
// WZ 사운드 선택은 Game에서 한다. BGM 노드를 지정하지 않으면 현재 맵의 info/bgm을 사용한다.
static constexpr bool MAPLE_USE_MAP_BGM = true;
static const char* MAPLE_BGM_WZ_NODE = nullptr;
static const char* MAPLE_ATTACK_SOUND_WZ_NODE = nullptr;

// 실제 클라이언트에서 확인할 ladderRope 배치 번호. INDEX_NONE이면 기본 시작점을 쓴다.
static constexpr int32 MAPLE_START_LADDER_ROPE_INDEX = INDEX_NONE;

// 사다리·로프 점프 직후 다시 매달릴 수 있을 때까지의 시간(초).
static constexpr float MAPLE_CLIMB_REENTRY_DELAY = 0.2f;

// 지면에서 떨어진 직후 첫 점프를 허용하는 시간(초).
static constexpr float MAPLE_COYOTE_TIME = 0.1f;

// 제공된 StatusBar3 XML의 기본 스킨. false면 기존 단색 HP바를 사용한다.
static constexpr bool MAPLE_USE_WZ_HUD = true;
static const char* MAPLE_HUD_BACKGROUND_NODE = R"(UI\StatusBar3.img\mainBar\status\backgrnd)";
static const char* MAPLE_HUD_HEALTH_NODE = R"(UI\StatusBar3.img\mainBar\status\gauge\hp\layer:0)";
static const char* MAPLE_HUD_MANA_NODE = R"(UI\StatusBar3.img\mainBar\status\gauge\mp\layer:0)";
static const char* MAPLE_HUD_FRAME_NODE = R"(UI\StatusBar3.img\mainBar\status\layer:cover)";

// nullptr로 바꾸면 HUD 숫자를 Windows 글꼴로 표시한다.
static const char* MAPLE_HUD_NUMBER_FOLDER = R"(UI/StatusBar3.img/mainBar/status/gauge/number)";
// 마지막 역슬래시는 경로 구분자가 아니라 WZ Canvas의 실제 이름이다.
static const char* MAPLE_HUD_NUMBER_SEPARATOR = R"(UI/StatusBar3.img/mainBar/status/gauge/number/\)";

UMapleGameInstance::UMapleGameInstance() = default;

UMapleGameInstance::~UMapleGameInstance()
{
	Shutdown();
}

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

	InitHUD();

	// 시작 위치만 지정하며 이후에는 방향키 입력으로 카메라를 움직인다.
	Engine.GetCamera().SetLocation(FVector2D(100.0f, 300.0f));

	return true;
}

void UMapleGameInstance::Shutdown()
{
	if (m_HUD && GEngine && GEngine->IsInitialized())
	{
		GEngine->GetUIViewport().RemoveWidget(m_HUD.Get());
	}

	m_HUD.Reset();

	Super::Shutdown();
}

void UMapleGameInstance::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const APlayerController* pController = GetEngine().GetWorld().GetFirstPlayerController();

	AMapleCharacter* pCharacter = pController ? Cast<AMapleCharacter>(pController->GetCharacter()) : nullptr;

	if (pCharacter)
	{
		pCharacter->UpdateAnimation();

		if (m_HUD)
		{
#ifdef _DEBUG
			if (m_HUD->GetVisibility() != ESlateVisibility::Visible)
			{
				UE_LOG(LogUI, Log, L"[MapleHUD] visible: controller=%p character=%p", (void*)pController, (void*)pCharacter);
			}
#endif

			m_HUD->SetVisibility(ESlateVisibility::Visible);
			m_HUD->UpdateHealth(pCharacter->GetHealth(), pCharacter->GetMaxHealth());
		}
	}

	else if (m_HUD)
	{
#ifdef _DEBUG
		if (m_HUD->GetVisibility() != ESlateVisibility::Hidden)
		{
			UE_LOG(LogUI, Warning, L"[MapleHUD] hidden: player unavailable; controller=%p controlledCharacter=%p", (void*)pController, pController ? (void*)pController->GetCharacter() : nullptr);
		}
#endif

		m_HUD->SetVisibility(ESlateVisibility::Hidden);
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
	const TSharedPtr<FAudioManager>& Audio = GetEngine().GetAudioManager();
	if (!Audio || !Audio->IsInitialized())
	{
		return;
	}

	Audio->SetBGMVolume(0.5f);
	Audio->SetSFXVolume(1.0f);

	TSharedPtr<USoundWave> BGM = FWzSoundLoader::LoadMapBGM(MAPLE_WZ_PATH, MAPLE_MAP_PATH);

	if (BGM)
	{
		// 볼륨 1.0, 0.5초 페이드인 후 반복 재생.
		Audio->PlayBGM(BGM, 1.0f, 0.5f);
	}
}

void UMapleGameInstance::InitHUD()
{
	FUIViewport& Viewport = GetEngine().GetUIViewport();
	if (!Viewport.IsInitialized())
	{
		return;
	}

	m_HUD = MakeShared<UMapleHUD>();

	bool bInitialized = false;
	if (MAPLE_USE_WZ_HUD)
	{
		FMapleHUDWzStyle Style;

		Style.m_BackgroundPath = MAPLE_HUD_BACKGROUND_NODE;
		Style.m_HealthFillPath = MAPLE_HUD_HEALTH_NODE;
		Style.m_ManaFillPath = MAPLE_HUD_MANA_NODE;
		Style.m_FramePath = MAPLE_HUD_FRAME_NODE;
		Style.m_NumberFontPath = MAPLE_HUD_NUMBER_FOLDER;
		Style.m_NumberSeparatorPath = MAPLE_HUD_NUMBER_SEPARATOR;

		bInitialized = m_HUD->InitializeFromWz(GetEngine().GetDevice(), MAPLE_WZ_PATH, Style);
		if (!bInitialized)
		{
			UE_LOG(LogUI, Warning, L"[MapleHUD] WZ skin unavailable; using solid-color HP/MP bars");
		}
	}

	if (!bInitialized)
	{
		bInitialized = m_HUD->Initialize();
	}

	if (!bInitialized || !Viewport.AddWidget(m_HUD))
	{
		UE_LOG(LogUI, Warning, L"[MapleHUD] initialization failed");
		m_HUD.Reset();
		return;
	}

	const APlayerController* pController = GetEngine().GetWorld().GetFirstPlayerController();
	const AMapleCharacter* pCharacter = pController ? Cast<AMapleCharacter>(pController->GetCharacter()) : nullptr;

	if (pCharacter)
	{
		m_HUD->UpdateHealth(pCharacter->GetHealth(), pCharacter->GetMaxHealth());
	}

	else
	{
		m_HUD->SetVisibility(ESlateVisibility::Hidden);
	}
}
