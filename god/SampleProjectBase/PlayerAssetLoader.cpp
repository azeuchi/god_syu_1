#include "PlayerAssetLoader.h"
#include "Player.h"
#include "Model.h"

namespace
{
	struct AnimEntry { const char* file; const char* name; };

	// knight モデルが使う共通アニメーション（ファイルと登録名）
	const AnimEntry kCommonAnims[] = {
		{ "Assets/Model/knight/Walking.fbx",     "Walk" },
		{ "Assets/Model/knight/WalkBack.fbx",    "WalkBack" },
		{ "Assets/Model/knight/CrouchIdle.fbx",  "CrouchIdle" },
		{ "Assets/Model/knight/LightPunch.fbx",  "LightPunch" },
		{ "Assets/Model/knight/MediumPunch.fbx", "MediumPunch" },
		{ "Assets/Model/knight/HeavyPunch.fbx",  "HeavyPunch" },
		{ "Assets/Model/knight/MediumKick.fbx",  "MediumKick" },
		{ "Assets/Model/knight/HeavyKick.fbx",   "HeavyKick" },
		{ "Assets/Model/knight/Jump.fbx",        "Jump" },
		{ "Assets/Model/knight/Damage.fbx",      "Damage" },
		{ "Assets/Model/knight/Down.fbx",        "Down" },
		{ "Assets/Model/knight/WakeUp.fbx",      "WakeUp" },
		{ "Assets/Model/knight/Hadouken.fbx",    "Hadouken" },
		{ "Assets/Model/knight/Death.fbx",       "Death" },
	};
}

void PlayerAssetLoader::LoadCommonAnimations(Player* player)
{
	if (!player) return;
	Model* m = player->GetModel();
	if (!m) return;

	for (const auto& a : kCommonAnims)
	{
		m->LoadAnimation(a.file, a.name, true);
	}
}