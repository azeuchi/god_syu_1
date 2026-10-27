#include "NormalAttack.h"
#include "PlayerStateIdle.h"
#include "PlayerStateCrouch.h"
#include "Player.h"

NormalAttack::NormalAttack(NormalMove move)
	: m_move(move)
{
}

void NormalAttack::Start(Player* player, NormalMove move)
{
	player->SetCurrentAttackParams(&player->GetNormalParams(move));
	player->SetState(std::make_unique<NormalAttack>(move));
}

bool NormalAttack::IsCrouch() const
{
	return GetNormalMoveInfo(m_move).crouch;
}

void NormalAttack::OnEnter(Player* player)
{
	const NormalMoveInfo& info = GetNormalMoveInfo(m_move);
	player->PlayAnimation(info.animName, true);

	// アニメーション全体が、緩急をかけた状態でパラメータの全体時間にちょうど収まる再生速度にする
	const AttackParams& params = player->GetNormalParams(m_move);
	const int originalFrames = player->GetModel()->GetAnimationTotalFrame(info.animName);
	player->SetAnimationSpeed(AttackAnimBaseSpeed(params, originalFrames, Player::s_variableSpeed));

	m_stateTimer = 0.0f;
	player->SetActiveHitbox(false);
	DirectX::XMFLOAT3 vel = player->GetVelocity();
	vel.x = 0.0f;
	vel.z = 0.0f;
	player->SetVelocity(vel);
}

void NormalAttack::Update(Player* player, float tick)
{
	m_stateTimer += tick;
	const AttackParams& params = player->GetNormalParams(m_move);

	player->SetActiveHitbox(m_stateTimer >= params.hitboxStart && m_stateTimer < params.hitboxEnd);

	// 踏み込みのある技は、発生するまで前へ進み、発生した瞬間に止まる
	if (params.forwardSpeed > 0.0f && m_stateTimer - tick < params.hitboxStart)
	{
		const float dir = (player->GetRotation().y < 0.0f) ? 1.0f : -1.0f;
		DirectX::XMFLOAT3 vel = player->GetVelocity();
		vel.x = (m_stateTimer < params.hitboxStart) ? dir * params.forwardSpeed : 0.0f;
		player->SetVelocity(vel);
	}

	if (m_stateTimer >= params.totalDuration)
	{
		// しゃがみ技のあとも下を入れ続けていれば、立ち上がらずにしゃがみへ戻す
		if (IsCrouch() && player->GetInputs().moveDown)
		{
			player->SetState(std::make_unique<PlayerStateCrouch>());
		}
		else
		{
			player->SetState(std::make_unique<PlayerStateIdle>());
		}
		return;
	}

	CheckCancel(player, m_stateTimer, params);
}
