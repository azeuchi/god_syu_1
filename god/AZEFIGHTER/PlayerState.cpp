#include "PlayerState.h"
#include "Player.h"

#include "NormalAttack.h"
#include "Hadouken.h"
#include "Shoryuken.h"

// 弱から弱へのキャンセルは、最初の1発を含めてこの数まで。押し返しに頼らず弱の連打を止める
static constexpr int MAX_LIGHT_CHAIN = 3;

bool PlayerState::TryStartSpecial(Player* player)
{
	const PlayerInputs& inputs = player->GetInputs();
	int strength = 0;
	if (inputs.HeavyPunch) strength = 2;
	else if (inputs.MediumPunch) strength = 1;

	// 昇竜拳を波動拳より先に判定する。2回目の下を斜め下前で入れると、波動拳のコマンドとも重なるため
	if (player->CheckShoryukenCommand())
	{
		player->SetState(std::make_unique<Shoryuken>(strength));
		return true;
	}
	if (player->CanFireProjectile() && player->CheckHadoukenCommand())
	{
		player->SetState(std::make_unique<Hadouken>(strength));
		return true;
	}
	return false;
}

bool PlayerState::CheckCancel(Player* player, float stateTimer, const AttackParams& params)
{
	// キャンセルが無効なら何もしない
	if (!params.cancelEnabled)
	{
		return false;
	}

	// キャンセル受付期間外なら何もしない
	if (stateTimer < params.cancelStart || stateTimer > params.cancelEnd)
	{
		return false;
	}

	
	if (!player->HasHit()) return false;

	// 必殺技へのキャンセル。通常技の残りの動きを省いてすぐ出せるので、フレームを細かく合わせなくてもつながる
	if (params.cancelToSpecial && TryStartSpecial(player))
	{
		player->SetLightChain(0);
		player->MarkCancel();
		return true;
	}

	const PlayerInputs& inputs = player->GetInputs();

	// ボタンの優先順は地上で技を出すときと同じ。下を入れていればしゃがみ技へ繋ぐ
	for (int i = 0; i < ATTACK_BUTTON_COUNT; ++i)
	{
		const AttackButton button = (AttackButton)i;
		if (!IsButtonPressed(inputs, button) || !CanCancelInto(params, button)) continue;

		const bool light = IsLightButton(button);
		if (light && player->GetLightChain() >= MAX_LIGHT_CHAIN) continue;
		player->SetLightChain(light ? player->GetLightChain() + 1 : 0);

		NormalAttack::Start(player, FindNormalMove(button, inputs.moveDown));
		player->MarkCancel();
		return true;
	}

	return false;
}