#include "EnginePCH.h"
#include "Character/AMapleCharacter.h"
#include "Ability/UAbilitySystemComponent.h"
#include "Ability/UGameplayEffect.h"
#include "Animation/UAnimStateMachine.h"
#include "Physics/URigidbody.h"

AMapleCharacter::AMapleCharacter()
{
	m_Attributes.InitAttribute(FName(L"Health"), m_MaxHealth, 0.0f, m_MaxHealth);
	AddComponent<UAbilitySystemComponent>()->SetAttributeSet(&m_Attributes);
	AddComponent<UAnimStateMachine>();
}

AMapleCharacter::~AMapleCharacter()
{
	ClearInvulnerability();
	// 멤버 속성이 먼저 소멸하므로 기반 액터가 컴포넌트를 정리하기 전에 분리한다.
	if (UAbilitySystemComponent* pAbilitySystem = GetAbilitySystemComponent())
	{
		pAbilitySystem->SetAttributeSet(nullptr);
	}
}

void AMapleCharacter::Tick(float DeltaTime)
{
	if (IsInvulnerable() && _finite(DeltaTime) && DeltaTime > 0.0f)
	{
		if (DeltaTime >= m_InvulnerabilityRemaining)
		{
			ClearInvulnerability();
			UE_LOG(LogCore, Log, L"[Damage] invulnerability ended: actor=%u", GetActorId());
		}
		else
		{
			m_InvulnerabilityRemaining -= DeltaTime;
			m_DamageFlashTime = fmod(m_DamageFlashTime + (double)DeltaTime, 2.0 * m_DamageBlinkInterval);
			UpdateDamageFlash();
		}
	}
	Super::Tick(DeltaTime);
}

void AMapleCharacter::EndPlay()
{
	ClearInvulnerability();
	Super::EndPlay();
}

float AMapleCharacter::TakeDamage(float DamageAmount, const AActor* pDamageCauser)
{
	if (!_finite(DamageAmount) || DamageAmount <= 0.0f || IsDead())
	{
		return 0.0f;
	}
	if (IsInvulnerable())
	{
		UE_LOG(LogCore, Log, L"[Damage] ignored: actor=%u hp=%.0f/%.0f remaining=%.2fs",
			GetActorId(), GetHealth(), m_MaxHealth, m_InvulnerabilityRemaining);
		return 0.0f;
	}
	UAbilitySystemComponent* pAbilitySystem = GetAbilitySystemComponent();
	if (!pAbilitySystem)
	{
		return 0.0f;
	}
	const float HealthBefore = GetHealth();
	UGameplayEffect Damage;
	Damage.AddModifier(FName(L"Health"), EGameplayModifierOperation::Add, -FMath::Min(DamageAmount, HealthBefore));
	pAbilitySystem->ApplyGameplayEffect(&Damage); // 즉시 효과는 활성 효과 목록에 보관되지 않는다.
	const float AppliedDamage = FMath::Max(0.0f, HealthBefore - GetHealth());
	if (AppliedDamage > 0.0f && !IsDead() && m_InvulnerabilityDuration > 0.0f)
	{
		m_TintBeforeDamage = GetFlipbookComponent()->GetTint();
		m_InvulnerabilityRemaining = m_InvulnerabilityDuration;
		m_DamageFlashTime = 0.0;
		UpdateDamageFlash();
	}
	UE_LOG(LogCore, Log, L"[Damage] applied: actor=%u causer=%u damage=%.0f hp=%.0f/%.0f invulnerable=%.2fs",
		GetActorId(), pDamageCauser ? pDamageCauser->GetActorId() : 0, AppliedDamage,
		GetHealth(), m_MaxHealth, m_InvulnerabilityRemaining);
	return AppliedDamage;
}

void AMapleCharacter::ResetHealth()
{
	ClearInvulnerability();
	FGameplayAttribute* pHealth = m_Attributes.GetAttribute(FName(L"Health"));
	pHealth->SetBaseValue(m_MaxHealth);
	pHealth->SetCurrentValue(m_MaxHealth);
	UE_LOG(LogCore, Log, L"[Damage] health reset: actor=%u hp=%.0f/%.0f", GetActorId(), GetHealth(), m_MaxHealth);
}

float AMapleCharacter::GetHealth() const
{
	return m_Attributes.GetCurrentValue(FName(L"Health"));
}

float AMapleCharacter::GetMaxHealth() const
{
	return m_MaxHealth;
}

bool AMapleCharacter::IsDead() const
{
	return GetHealth() <= 0.0f;
}

UAbilitySystemComponent* AMapleCharacter::GetAbilitySystemComponent() const
{
	return GetComponent<UAbilitySystemComponent>();
}

void AMapleCharacter::SetMaxHealth(float MaxHealth)
{
	if (!_finite(MaxHealth) || MaxHealth <= 0.0f)
	{
		return;
	}
	const float Health = FMath::Min(GetHealth(), MaxHealth);
	m_MaxHealth = MaxHealth;
	m_Attributes.InitAttribute(FName(L"Health"), Health, 0.0f, m_MaxHealth);
}

void AMapleCharacter::SetInvulnerabilityDuration(float Duration)
{
	if (_finite(Duration) && Duration >= 0.0f)
	{
		m_InvulnerabilityDuration = Duration;
	}
}

void AMapleCharacter::SetDamageBlinkInterval(float Interval)
{
	if (_finite(Interval) && Interval > 0.0f)
	{
		m_DamageBlinkInterval = Interval;
		m_DamageFlashTime = 0.0;
		UpdateDamageFlash();
	}
}

bool AMapleCharacter::IsInvulnerable() const
{
	return m_InvulnerabilityRemaining > 0.0f;
}

float AMapleCharacter::GetInvulnerabilityTimeRemaining() const
{
	return (float)m_InvulnerabilityRemaining;
}

void AMapleCharacter::ClearInvulnerability()
{
	if (IsInvulnerable())
	{
		GetFlipbookComponent()->SetTint(m_TintBeforeDamage);
	}
	m_InvulnerabilityRemaining = 0.0f;
	m_DamageFlashTime = 0.0;
}

void AMapleCharacter::UpdateDamageFlash()
{
	if (IsInvulnerable())
	{
		const bool bGrayTint = m_DamageFlashTime < m_DamageBlinkInterval;
		// 투명도는 유지하고 회색 틴트로 RGB 밝기만 낮춘다.
		const FLinearColor DamageTint = m_TintBeforeDamage * FLinearColor(0.65f, 0.65f, 0.65f, 1.0f);
		GetFlipbookComponent()->SetTint(bGrayTint ? DamageTint : m_TintBeforeDamage);
	}
}

bool AMapleCharacter::InitializeAvatar(FDXDevice& Device, const char* WzPath, const char* LoadoutSpec)
{
	LoadAvatar(Device, WzPath, LoadoutSpec, "stand1", 0);
	const bool bHasMove = RegisterAvatarState(Device, WzPath, LoadoutSpec, FName(L"Move"), "walk1");
	const bool bHasIdle = RegisterAvatarState(Device, WzPath, LoadoutSpec, FName(L"Idle"), "stand1");
	RegisterAvatarState(Device, WzPath, LoadoutSpec, FName(L"Jump"), "jump");
	RegisterAvatarState(Device, WzPath, LoadoutSpec, FName(L"Ladder"), "ladder");
	RegisterAvatarState(Device, WzPath, LoadoutSpec, FName(L"Rope"), "rope");
	if (bHasIdle || bHasMove)
	{
		GetComponent<UAnimStateMachine>()->SetState(FName(bHasIdle ? L"Idle" : L"Move"));
	}
	return GetFlipbookComponent()->HasTexture();
}

bool AMapleCharacter::RegisterAvatarState(FDXDevice& Device, const char* WzPath, const char* LoadoutSpec,
	FName StateName, const char* ActionName)
{
	TArray<FFlipbookFrame> Frames;
	for (int32 i = 0; ; i++)
	{
		FAvatarTexture Frame = FWzTextureLoader::LoadAvatarTexture(Device, WzPath, LoadoutSpec, ActionName, i);
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
	TSharedPtr<UFlipbook> Flipbook = UFlipbook::Create(Frames);
	UAnimStateMachine* pStateMachine = GetComponent<UAnimStateMachine>();
	const bool bRegistered = Flipbook && pStateMachine && pStateMachine->RegisterState(StateName, Flipbook, true);
	for (int32 i = 0; i < Frames.Num(); i++)
	{
		Frames[i].m_pTexture->Release();
	}
	return bRegistered;
}

void AMapleCharacter::UpdateAnimation()
{
	UAnimStateMachine* pStateMachine = GetComponent<UAnimStateMachine>();
	UFlipbookComponent* pFlipbook = GetFlipbookComponent();
	const URigidbody* pBody = GetComponent<URigidbody>();
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
	else if (FMath::Abs(GetLastMovementInputVector().m_X) > FMath::KINDA_SMALL_NUMBER)
	{
		StateName = FName(L"Move");
	}
	if (!pStateMachine->HasState(StateName))
	{
		StateName = pStateMachine->HasState(FName(L"Idle")) ? FName(L"Idle") : FName(L"Move");
	}
	if (pStateMachine->HasState(StateName))
	{
		pStateMachine->SetState(StateName);
		if (pBody->IsClimbing() && FMath::Abs(pBody->GetVelocity().m_Y) <= FMath::KINDA_SMALL_NUMBER)
		{
			pFlipbook->Stop();
		}
		else
		{
			pFlipbook->Play();
		}
	}
}
