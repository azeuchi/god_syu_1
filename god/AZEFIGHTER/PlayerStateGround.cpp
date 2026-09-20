#include "PlayerStateGround.h"
#include "Player.h"

// 遷移先のステート
#include "NormalAttack.h"
#include "PlayerStateJump.h"
#include "PlayerStateCrouch.h"
#include "Hadouken.h"
#include "Shoryuken.h"

void PlayerStateGround::Update(Player* player, float tick)
{
	// --- 共通の遷移チェック (攻撃・ジャンプ・しゃがみ) ---

	const PlayerInputs& inputs = player->GetInputs();

	// 昇竜拳を波動拳より先に判定する。2回目の下を斜め下前で入れると、波動拳のコマンドとも重なるため
	if (player->CheckShoryukenCommand())
	{
		int strength = 0;
		if (inputs.HeavyPunch) strength = 2;
		else if (inputs.MediumPunch) strength = 1;

		player->SetState(std::make_unique<Shoryuken>(strength));
		return;
	}

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

	// 通常技。下を入れていればしゃがみ技になる
	for (int i = 0; i < ATTACK_BUTTON_COUNT; ++i)
	{
		const AttackButton button = (AttackButton)i;
		if (!IsButtonPressed(inputs, button)) continue;

		NormalAttack::Start(player, FindNormalMove(button, inputs.moveDown));
		return;
	}

	// ジャンプ
	if (inputs.jump && !player->GetIsJumping())
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