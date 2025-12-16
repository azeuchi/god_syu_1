#include "PlayerStateGround.h"
#include "Player.h"

// 遷移先のステート
#include "LightPunch.h"
#include "MediumPunch.h"
#include "HeavyKick.h" 
#include "PlayerStateJump.h"
#include "PlayerStateCrouch.h"

void PlayerStateGround::Update(Player* player, float tick)
{
	// --- 共通の遷移チェック (攻撃・ジャンプ・しゃがみ) ---

	const PlayerInputs& inputs = player->GetInputs();

	// 攻撃1 (Light Punch)
	if (inputs.LightPunchi)
	{
		player->SetCurrentAttackParams(&player->GetLightPunchParams());
		player->SetState(new LightPunch());
		return;
	}
	// 攻撃2 (Medium Punch)
	else if (inputs.MediumPunch)
	{
		player->SetCurrentAttackParams(&player->GetMediumPunchParams());
		player->SetState(new MediumPunch());
		return;
	}
	// 攻撃3 (Heavy Kick)
	else if (inputs.HeavyKick)
	{
		player->SetCurrentAttackParams(&player->GetHeavyKickParams());
		player->SetState(new HeavyKick());
		return;
	}
	// ジャンプ
	else if (inputs.jump && !player->GetIsJumping())
	{
		player->Jump();
		player->SetState(new PlayerStateJump());
		return;
	}
	// しゃがみ
	// 「下入力があり」かつ「現在自分がしゃがみステートではない」場合のみ遷移
	else if (inputs.moveDown && !IsCrouch())
	{
		player->SetState(new PlayerStateCrouch());
		return;
	}

	// 共通チェックで遷移しなかった場合、子クラス固有の処理を行う
	UpdateBehavior(player, tick);
}