#pragma once

// SE・BGM再生（XAudio2）。wavはそのまま、mp3などはMedia Foundationでデコードして再生する
// パスを渡して呼ぶだけで再生できる。読み込みは自動でキャッシュされ、
// ファイルが無い場合は何もしない（ログにだけ出す）
class Sound
{
public:
	static bool Init();
	static void Uninit();

	// 効果音を鳴らす。同じ音の重ね再生も可
	static void PlaySE(const char* path, float volume = 1.0f);

	// BGMをループ再生する。再生中の曲と同じパスなら音量変更だけ行う
	static void PlayBGM(const char* path, float volume = 1.0f);
	static void StopBGM();
	static void SetBGMVolume(float volume);

	// BGMにかけるフェード倍率。1.0で通常、0.0で無音。画面の暗転に合わせて使う
	static void SetBGMFade(float factor);
};