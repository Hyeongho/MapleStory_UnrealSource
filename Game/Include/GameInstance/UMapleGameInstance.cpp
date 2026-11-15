#include "EnginePCH.h"
#include "GameInstance/UMapleGameInstance.h"
#include "Engine.h"
#include "Controller/AMaplePlayerController.h"
#include "Character/AMapleCharacter.h"
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
static const char* MAPLE_MAP_PATH = R"(Map\Map\Map2\240020210.img)";
// 실제 클라이언트에서 확인할 ladderRope 배치 번호. INDEX_NONE이면 기본 시작점을 쓴다.
static constexpr int32 MAPLE_START_LADDER_ROPE_INDEX = 7;
// 사다리·로프 점프 직후 다시 매달릴 수 있을 때까지의 시간(초).
static constexpr float MAPLE_CLIMB_REENTRY_DELAY = 0.2f;
// 지면에서 떨어진 직후 첫 점프를 허용하는 시간(초).
static constexpr float MAPLE_COYOTE_TIME = 0.1f;
static const char* MAPLE_LOADOUT_SPEC = "2015,12015,53003,65007,,,1054087,,1073816,,,,1703431,,,,,,,,,,,,,,,,";

// WZ 사운드 선택은 Game에서 한다. BGM 노드를 지정하지 않으면 현재 맵의 info/bgm을 사용한다.
static constexpr bool MAPLE_USE_MAP_BGM = true;
static const char* MAPLE_BGM_WZ_NODE = nullptr;
// Weapon.img XML에 있는 공격음. 사용할 무기 종류는 Game에서 선택한다.
static const char* MAPLE_ATTACK_SOUND_WZ_NODE = R"(Sound\Weapon.img\swordL\Attack)";
// 일반 파일을 지정하면 해당 사운드는 WZ 대신 이 파일을 사용한다.
static const wchar_t* MAPLE_BGM_FILE = nullptr;
static const wchar_t* MAPLE_ATTACK_SOUND_FILE = nullptr;

// 제공된 StatusBar3 XML의 기본 스킨. false면 단색 HP·MP바를 사용한다.
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
	// 이 맵에서 표시하지 않을 obj 리소스를 Game에서 선택한다.
	// Emplace("guide", "tutorial")은 그 아래의 모든 오브젝트를 제외한다.
	// Emplace("guide", "tutorial", "key")는 key 아래의 모든 오브젝트를 제외한다.
	// 동일한 리소스의 특정 배치만 제외하려면 뒤에 레이어와 슬롯 번호를 추가한다.
	MapOptions.m_ExcludedObjects.Emplace("guide", "tutorial", "key", "4");
	FMapLoader::LoadMap(Engine.GetDevice(), Engine.GetWorld(), Engine.GetCamera(), MAPLE_WZ_PATH, MAPLE_MAP_PATH, MapOptions);
	InitPlayer();
	InitAudio();
	InitHUD();
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
		// 거리 감쇠의 기준은 카메라 경계 보정과 무관하게 플레이어 위치로 정한다.
		if (const TSharedPtr<FAudioManager>& Audio = GetEngine().GetAudioManager())
		{
			Audio->SetListenerLocation(pCharacter->GetLocation());
		}
	}
	else if (m_HUD)
	{
#ifdef _DEBUG
		if (m_HUD->GetVisibility() != ESlateVisibility::Hidden)
		{
			UE_LOG(LogUI, Warning, L"[MapleHUD] hidden: player unavailable; controller=%p controlledCharacter=%p",
				(void*)pController, pController ? (void*)pController->GetCharacter() : nullptr);
		}
#endif
		m_HUD->SetVisibility(ESlateVisibility::Hidden);
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

APlayerController* UMapleGameInstance::CreatePlayerController(UWorld& World)
{
	return World.SpawnActor<AMaplePlayerController>();
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
	const APlayerController* pController = GetEngine().GetWorld().GetFirstPlayerController();
	AMapleCharacter* pCharacter = pController ? Cast<AMapleCharacter>(pController->GetCharacter()) : nullptr;
	if (pCharacter)
	{
		Audio->SetListenerLocation(pCharacter->GetLocation());
	}
	TSharedPtr<USoundWave> BGM;
	if (MAPLE_BGM_FILE)
	{
		BGM = USoundWave::LoadFromFile(MAPLE_BGM_FILE);
	}
	else if (MAPLE_BGM_WZ_NODE)
	{
		BGM = FWzSoundLoader::LoadSound(MAPLE_WZ_PATH, MAPLE_BGM_WZ_NODE);
	}
	else if (MAPLE_USE_MAP_BGM)
	{
		BGM = FWzSoundLoader::LoadMapBGM(MAPLE_WZ_PATH, MAPLE_MAP_PATH);
	}
	if (BGM)
	{
		Audio->PlayBGM(BGM, 1.0f, 0.5f);
	}
	if (pCharacter)
	{
		if (MAPLE_ATTACK_SOUND_FILE)
		{
			pCharacter->SetAttackSound(USoundWave::LoadFromFile(MAPLE_ATTACK_SOUND_FILE));
		}
		else if (MAPLE_ATTACK_SOUND_WZ_NODE)
		{
			pCharacter->SetAttackSound(FWzSoundLoader::LoadSound(MAPLE_WZ_PATH, MAPLE_ATTACK_SOUND_WZ_NODE));
		}
	}
}

void UMapleGameInstance::InitPlayer()
{
	UEngine& Engine = GetEngine();
	AMapleCharacter* pPlayerCharacter = Engine.GetWorld().SpawnActor<AMapleCharacter>();
	pPlayerCharacter->InitializeAvatar(Engine.GetDevice(), MAPLE_WZ_PATH, MAPLE_LOADOUT_SPEC);
	pPlayerCharacter->SetLocation(FVector2D(-300.0f, 100.0f));

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
