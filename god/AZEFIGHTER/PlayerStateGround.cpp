#include "PlayerStateGround.h"
#include "Player.h"

// 遷移先のステート
#include "LightPunch.h"
#include "MediumPunch.h"
#include "HeavyPunch.h"
#include "MediumKick.h"
#include "HeavyKick.h" 
#include "PlayerStateJump.h"
#include "PlayerStateCrouch.h"
#include "Hadouken.h"

void PlayerStateGround::Update(Player* player, float tick)
{
	// --- 共通の遷移チェック (攻撃・ジャンプ・しゃがみ) ---

	const PlayerInputs& inputs = player->GetInputs();

	// 波動拳コマンドチェック
	if (player->CanFireProjectile() && player->CheckHadoukenCommand())
	{
		int strength = 0;
		if (inputs.HeavyPunch) strength = 2;
		else if (inputs.MediumPunch) strength = 1;
		else strength = 0;

		player->SetState(std::make_unique<Hadouken>(strength));
		return;
	}

	// 攻撃1 (Light Punch)
	if (inputs.LightPunch)
	{
		player->SetCurrentAttackParams(&player->GetLightPunchParams());
		player->SetState(std::make_unique<LightPunch>());
		return;
	}
	// 攻撃2 (Medium Punch)
	else if (inputs.MediumPunch)
	{
		player->SetCurrentAttackParams(&player->GetMediumPunchParams());
		player->SetState(std::make_unique<MediumPunch>());
		return;
	}
	// 攻撃3 (Heavy Punch)
	else if (inputs.HeavyPunch)
	{
		player->SetCurrentAttackParams(&player->GetHeavyPunchParams());
		player->SetState(std::make_unique<HeavyPunch>());
		return;
	}
	// 攻撃4 (Medium Kick) Kキー
	else if (inputs.MediumKick)
	{
		player->SetCurrentAttackParams(&player->GetMediumKickParams());
		player->SetState(std::make_unique<MediumKick>());
		return;
	}
	// 攻撃5 (Heavy Kick) Lキー
	else if (inputs.HeavyKick)
	{
		player->SetCurrentAttackParams(&player->GetHeavyKickParams());
		player->SetState(std::make_unique<HeavyKick>());
		return;
	}
	// ジャンプ
	else if (inputs.jump && !player->GetIsJumping())
	{
		player->Jump();
		player->SetState(std::make_unique<PlayerStateJump>());
		return;
	}
	// しゃがみ
	// 「下入力があり」かつ「現在自分がしゃがみステートではない」場合のみ遷移
	else if (inputs.moveDown && !IsCrouch())
	{
		player->SetState(std::make_unique<PlayerStateCrouch>());
		return;
	}

	// 共通チェックで遷移しなかった場合、子クラス固有の処理を行う
	UpdateBehavior(player, tick);
}