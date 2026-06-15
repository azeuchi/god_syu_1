#pragma once
#include <DirectXMath.h>
#include <cstdlib>
#include <algorithm>

// カメラシェイク（Trauma方式）の共有調整値とロジック。
// 調整値は inline static なので全シーンで共有され、デバッグシーンで変更すると本編シーンにも反映される（同一起動中）。
class CameraShake
{
public:
	// --- 調整値（デバッグシーンから変更可能）---
	inline static float s_maxOffset  = 0.22f; // 揺れ幅の最大（ワールド単位）
	inline static float s_decay      = 2.2f;  // traumaの減衰速度（毎秒）
	inline static float s_kickRatio  = 0.6f;  // 初撃の指向性の強さ（0=ノイズのみ 1=方向のみ）
	inline static float s_hitStopRef = 0.18f; // この長さのヒットストップでtrauma=1相当にする

	// 既定値（デバッグシーンのリセット用）
	static void ResetParams()
	{
		s_maxOffset = 0.22f;
		s_decay = 2.2f;
		s_kickRatio = 0.6f;
		s_hitStopRef = 0.18f;
	}

	// ヒットストップ長から trauma 量を求める（0.0から1.0）
	static float TraumaFromHitStop(float hitStopTime)
	{
		return std::clamp(hitStopTime / s_hitStopRef, 0.0f, 1.0f);
	}

	// trauma に揺れを加える（0.0から1.0でクランプ）
	static void AddTrauma(float& trauma, float amount)
	{
		trauma = std::clamp(trauma + amount, 0.0f, 1.0f);
	}

	// 毎フレームの更新。traumaを減衰させ、今フレームの揺れオフセットを outOffset に書き込む。
	static void Tick(float& trauma, float kickDir, float tick, DirectX::XMFLOAT3& outOffset)
	{
		if (trauma <= 0.0f)
		{
			outOffset = { 0.0f, 0.0f, 0.0f };
			return;
		}

		// trauma^2 で立ち上がりを鋭く、終わり際をなめらかにする
		float shake = trauma * trauma;
		float amp = s_maxOffset * shake;

		// ノイズ成分（-1.0から1.0）
		float nx = (float)(rand() % 1000) / 500.0f - 1.0f;
		float ny = (float)(rand() % 1000) / 500.0f - 1.0f;

		// 横は初撃方向へ寄せ、縦はノイズ主体にする
		float ox = amp * (nx * (1.0f - s_kickRatio) + kickDir * s_kickRatio);
		float oy = amp * ny;
		outOffset = { ox, oy, 0.0f };

		// 実時間で減衰
		trauma -= s_decay * tick;
		if (trauma < 0.0f) trauma = 0.0f;
	}
};
