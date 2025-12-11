#include "PlayerStateGround.h"
#include "Player.h"

// 遷移先のステート
#include "LightPunch.h"
#include "MediumPunch.h"
#include "PlayerStateJump.h"

void PlayerStateGround::Update(Player* player, float tick)
{
	// --- 共通の遷移チェック (攻撃・ジャンプ) ---
	const PlayerInputs& inputs = player->GetInputs();

	// 攻撃1
	if (inputs.attack1)
	{
		player->SetCurrentAttackParams(&player->GetLightPunchParams());
		player->SetState(new LightPunch());
		return; // ステートが変わったので終了
	}
	// 攻撃2
	else if (inputs.attack2)
	{
		player->SetCurrentAttackParams(&player->GetMediumPunchParams());
		player->SetState(new MediumPunch());
		return;
	}
	// ジャンプ
	else if (inputs.jump && !player->GetIsJumping())
	{
		player->Jump();
		player->SetState(new PlayerStateJump());
		return;
	}

	// 共通チェックで遷移しなかった場合、子クラス固有の処理を行う
	UpdateBehavior(player, tick);
}