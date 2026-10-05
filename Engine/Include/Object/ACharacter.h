#pragma once
#include "AActor.h"
#include "Render/USpriteComponent.h"
#include "Render/WzTextureLoader.h"

class UBoxCollision;
class URigidbody;

class ACharacter :
    public AActor
{
    DECLARE_CLASS(ACharacter, AActor)
public:
    ACharacter();
    virtual ~ACharacter() override;

	// FWzTextureLoader::LoadAvatarTexture를 감싸서, 결과를 내부 USpriteComponent에
	// 바로 세팅한다. 실패(DLL/노드 없음)해도 조용히 무시 — 호출자는
	// GetSpriteComponent()->HasTexture()로 확인.
	void LoadAvatar(FDXDevice& Device, const char* WzPath, const char* LoadoutSpec, const char* ActionName, int32 FrameIndex, const char* EmotionName = "default", int32 EmotionFrameIndex = 0);

	void SetLocation(const FVector2D& Location);
	FVector2D GetLocation() const;

	// 발을 기준으로 충돌체를 붙인다. 물리를 쓰지 않는 캐릭터는 기존 직접 이동을 유지한다.
	void EnablePhysics(const FVector2D& CollisionHalfExtent);

	// 이번 프레임의 이동 방향을 모은 뒤 Tick에서 강체 속도에 반영한다.
	void AddMovementInput(const FVector2D& Direction);
	void SetMoveSpeed(float Speed);
	// 발판 또는 정적 지면에 닿았을 때만 점프한다. 속도는 게임에서 조절할 수 있다.
	void SetJumpSpeed(float Speed);
	void Jump();
	// 현재 단방향 발판 위에서만 아래로 통과한다.
	void DropThroughFoothold();
	virtual void Tick(float DeltaTime) override;

	void SetFacingRight(bool bFacingRight);

	// 맵의 Life 컨테이너 설정. INDEX_NONE은 발판에 속하지 않는 Sky 컨테이너다.
	void SetMapLayer(int32 LayerIndex, int32 FootholdOrder = 0);
	int32 GetMapLayer() const;

	USpriteComponent* GetSpriteComponent() const { return m_pSpriteComponent; }

private:
	USpriteComponent* m_pSpriteComponent = nullptr;
	UBoxCollision* m_pCollisionComponent = nullptr;
	URigidbody* m_pRigidbody = nullptr;
	int32 m_MapLayer = INDEX_NONE;
	FVector2D m_PendingMovementInput = FVector2D::Zero;
	float m_MoveSpeed = 200.0f;
	float m_JumpSpeed = 420.0f;
};

