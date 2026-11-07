#pragma once

#include "Object/ACharacter.h"
#include "Ability/UAttributeSet.h"

class UAbilitySystemComponent;

class AMapleCharacter :
    public ACharacter
{
    DECLARE_CLASS(AMapleCharacter, ACharacter)

public:
    AMapleCharacter();
    virtual ~AMapleCharacter();

    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay() override;

	// 아바타 준비와 물리 갱신 이후의 애니메이션 선택
	bool InitializeAvatar(FDXDevice& Device, const char* WzPath, const char* LoadoutSpec);
	void UpdateAnimation();

	// 피해 적용. 실제 감소한 체력을 반환하며 무적·사망 상태에서는 0을 반환한다.
	virtual float TakeDamage(float DamageAmount, const AActor* pDamageCauser = nullptr);
	void ResetHealth();
	float GetHealth() const;
	float GetMaxHealth() const;
	bool IsDead() const;

	UAbilitySystemComponent* GetAbilitySystemComponent() const;

	// 최대 체력을 줄이면 현재 체력도 상한 안으로 보정한다.
	void SetMaxHealth(float MaxHealth);
	// 다음 피격부터 적용한다. 0이면 피격 후 무적 시간을 사용하지 않는다.
	void SetInvulnerabilityDuration(float Duration);
	void SetDamageBlinkInterval(float Interval);
	bool IsInvulnerable() const;
	float GetInvulnerabilityTimeRemaining() const;

protected:
	// 게임 캐릭터를 확장할 때 아바타 액션을 추가로 등록할 수 있다.
	bool RegisterAvatarState(FDXDevice& Device, const char* WzPath, const char* LoadoutSpec, FName StateName, const char* ActionName);

private:
	void ClearInvulnerability();
	void UpdateDamageFlash();

	// 속성은 이 캐릭터가 소유하며 Ability System은 비소유 참조로 연결한다.
	UAttributeSet m_Attributes;
	float m_MaxHealth = 100.0f;

	float m_InvulnerabilityDuration = 1.0f;
	double m_InvulnerabilityRemaining = 0.0;

	float m_DamageBlinkInterval = 0.1f;
	double m_DamageFlashTime = 0.0;

	FLinearColor m_TintBeforeDamage = FLinearColor::White;
};

