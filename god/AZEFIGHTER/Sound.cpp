#include "Sound.h"
#include "DebugLog.h"
#include <windows.h>
#include <xaudio2.h>
#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <cctype>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

namespace
{
	IXAudio2* g_pXAudio = nullptr;
	IXAudio2MasteringVoice* g_pMaster = nullptr;
	bool g_comInitialized = false;

	// 読み込んだWAVのキャッシュ。失敗もキャッシュして毎フレームのディスクアクセスを防ぐ
	struct WavData
	{
		std::vector<BYTE> format; // WAVEFORMATEX一式
		std::vector<BYTE> data;   // 波形本体
		bool valid = false;
	};
	std::map<std::string, WavData> g_wavCache;

	// 再生中のSEボイス。再生が終わったものは次のPlaySE時に回収する
	std::vector<IXAudio2SourceVoice*> g_seVoices;

	IXAudio2SourceVoice* g_pBgmVoice = nullptr;
	std::string g_bgmPath;
	float g_bgmBaseVolume = 1.0f; // PlayBGMで指定された元の音量
	float g_bgmFade = 1.0f;       // 暗転に合わせてかける倍率

	// 拡張子がwavかどうか
	bool IsWavPath(const std::string& path)
	{
		if (path.size() < 4) return false;
		std::string ext = path.substr(path.size() - 4);
		for (auto& c : ext) c = (char)tolower((unsigned char)c);
		return ext == ".wav";
	}

	// mp3などをMedia Foundationでデコードし、PCMとして取り出す
	bool DecodeByMediaFoundation(const std::string& path, WavData& out)
	{
		int wlen = MultiByteToWideChar(CP_ACP, 0, path.c_str(), -1, nullptr, 0);
		if (wlen <= 0) return false;
		std::wstring wpath((size_t)wlen, L'\0');
		MultiByteToWideChar(CP_ACP, 0, path.c_str(), -1, &wpath[0], wlen);

		IMFSourceReader* reader = nullptr;
		if (FAILED(MFCreateSourceReaderFromURL(wpath.c_str(), nullptr, &reader))) return false;

		IMFMediaType* pcmType = nullptr;
		IMFMediaType* actualType = nullptr;
		WAVEFORMATEX* wfx = nullptr;
		bool ok = false;

		// 出力をPCM指定にしてデコーダを挟ませる
		if (SUCCEEDED(MFCreateMediaType(&pcmType)) &&
			SUCCEEDED(pcmType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio)) &&
			SUCCEEDED(pcmType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM)) &&
			SUCCEEDED(reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, pcmType)) &&
			SUCCEEDED(reader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, &actualType)))
		{
			UINT32 fmtSize = 0;
			if (SUCCEEDED(MFCreateWaveFormatExFromMFMediaType(actualType, &wfx, &fmtSize)))
			{
				out.format.assign((BYTE*)wfx, (BYTE*)wfx + fmtSize);
				ok = true;

				// 最後まで読み進めて波形をつなげる
				for (;;)
				{
					DWORD flags = 0;
					IMFSample* sample = nullptr;
					if (FAILED(reader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &flags, nullptr, &sample)))
					{
						ok = false;
						if (sample) sample->Release();
						break;
					}
					if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
					{
						if (sample) sample->Release();
						break;
					}
					if (!sample) continue;

					IMFMediaBuffer* buffer = nullptr;
					if (SUCCEEDED(sample->ConvertToContiguousBuffer(&buffer)))
					{
						BYTE* p = nullptr;
						DWORD len = 0;
						if (SUCCEEDED(buffer->Lock(&p, nullptr, &len)))
						{
							out.data.insert(out.data.end(), p, p + len);
							buffer->Unlock();
						}
						buffer->Release();
					}
					sample->Release();
				}
			}
		}

		if (wfx) CoTaskMemFree(wfx);
		if (actualType) actualType->Release();
		if (pcmType) pcmType->Release();
		reader->Release();

		return ok && !out.format.empty() && !out.data.empty();
	}

	// WAVファイル（RIFF）を読み込む
	const WavData* LoadWav(const std::string& path)
	{
		auto it = g_wavCache.find(path);
		if (it != g_wavCache.end()) return &it->second;

		WavData& w = g_wavCache[path];

		// wav以外（mp3など）はMedia Foundation側に任せる
		if (!IsWavPath(path))
		{
			w.valid = DecodeByMediaFoundation(path, w);
			if (!w.valid) DebugLog::log(DebugLog::INFO_LOG, "Sound: decode failed %s", path.c_str());
			return &w;
		}

		std::ifstream file(path, std::ios::binary);
		if (!file)
		{
			DebugLog::log(DebugLog::INFO_LOG, "Sound: file not found %s", path.c_str());
			return &w;
		}

		char riff[12] = {};
		file.read(riff, 12);
		if (file.gcount() != 12 || memcmp(riff, "RIFF", 4) != 0 || memcmp(riff + 8, "WAVE", 4) != 0)
		{
			DebugLog::log(DebugLog::INFO_LOG, "Sound: not a wav %s", path.c_str());
			return &w;
		}

		// チャンクを走査して fmt と data を取り出す
		while (file)
		{
			char id[4] = {};
			unsigned int size = 0;
			file.read(id, 4);
			file.read((char*)&size, 4);
			if (file.gcount() != 4) break;

			if (memcmp(id, "fmt ", 4) == 0)
			{
				unsigned int keep = (size < sizeof(WAVEFORMATEX)) ? (unsigned int)sizeof(WAVEFORMATEX) : size;
				w.format.assign(keep, 0);
				file.read((char*)w.format.data(), size);
			}
			else if (memcmp(id, "data", 4) == 0)
			{
				w.data.resize(size);
				file.read((char*)w.data.data(), size);
			}
			else
			{
				file.seekg(size, std::ios::cur);
			}
			// チャンクは2バイト境界に揃う
			if (size & 1) file.seekg(1, std::ios::cur);
		}

		w.valid = !w.format.empty() && !w.data.empty();
		if (!w.valid) DebugLog::log(DebugLog::INFO_LOG, "Sound: invalid wav %s", path.c_str());
		return &w;
	}

	// 再生し終わったSEボイスを破棄する
	void SweepFinishedVoices()
	{
		for (size_t i = 0; i < g_seVoices.size(); )
		{
			XAUDIO2_VOICE_STATE state;
			g_seVoices[i]->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
			if (state.BuffersQueued == 0)
			{
				g_seVoices[i]->DestroyVoice();
				g_seVoices.erase(g_seVoices.begin() + i);
			}
			else
			{
				++i;
			}
		}
	}

	// ボイスを1つ作って再生を開始する
	IXAudio2SourceVoice* StartVoice(const WavData* w, float volume, bool loop)
	{
		IXAudio2SourceVoice* v = nullptr;
		if (FAILED(g_pXAudio->CreateSourceVoice(&v, (const WAVEFORMATEX*)w->format.data()))) return nullptr;

		XAUDIO2_BUFFER buf = {};
		buf.AudioBytes = (UINT32)w->data.size();
		buf.pAudioData = w->data.data();
		buf.Flags = XAUDIO2_END_OF_STREAM;
		if (loop) buf.LoopCount = XAUDIO2_LOOP_INFINITE;

		if (FAILED(v->SubmitSourceBuffer(&buf)))
		{
			v->DestroyVoice();
			return nullptr;
		}
		v->SetVolume(volume);
		v->Start();
		return v;
	}
}

bool Sound::Init()
{
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	g_comInitialized = SUCCEEDED(hr);

	// mp3などのデコードにMedia Foundationを使う
	if (FAILED(MFStartup(MF_VERSION)))
	{
		DebugLog::log(DebugLog::INFO_LOG, "Sound: MFStartup failed");
	}

	if (FAILED(XAudio2Create(&g_pXAudio)))
	{
		DebugLog::log(DebugLog::INFO_LOG, "Sound: XAudio2Create failed");
		return false;
	}
	if (FAILED(g_pXAudio->CreateMasteringVoice(&g_pMaster)))
	{
		g_pXAudio->Release();
		g_pXAudio = nullptr;
		DebugLog::log(DebugLog::INFO_LOG, "Sound: CreateMasteringVoice failed");
		return false;
	}
	return true;
}

void Sound::Uninit()
{
	for (auto v : g_seVoices) v->DestroyVoice();
	g_seVoices.clear();
	if (g_pBgmVoice) { g_pBgmVoice->DestroyVoice(); g_pBgmVoice = nullptr; }
	if (g_pMaster) { g_pMaster->DestroyVoice(); g_pMaster = nullptr; }
	if (g_pXAudio) { g_pXAudio->Release(); g_pXAudio = nullptr; }
	g_wavCache.clear();
	MFShutdown();
	if (g_comInitialized) CoUninitialize();
}

void Sound::PlaySE(const char* path, float volume)
{
	if (!g_pXAudio || !path) return;
	const WavData* w = LoadWav(path);
	if (!w->valid) return;

	SweepFinishedVoices();
	IXAudio2SourceVoice* v = StartVoice(w, volume, false);
	if (v) g_seVoices.push_back(v);
}

void Sound::PlayBGM(const char* path, float volume)
{
	if (!g_pXAudio || !path) return;

	g_bgmBaseVolume = volume;

	// 同じ曲なら音量だけ合わせて続行
	if (g_pBgmVoice && g_bgmPath == path)
	{
		g_pBgmVoice->SetVolume(g_bgmBaseVolume * g_bgmFade);
		return;
	}

	const WavData* w = LoadWav(path);
	if (!w->valid) return;

	StopBGM();
	g_pBgmVoice = StartVoice(w, g_bgmBaseVolume * g_bgmFade, true);
	if (g_pBgmVoice) g_bgmPath = path;
}

void Sound::StopBGM()
{
	if (!g_pBgmVoice) return;
	g_pBgmVoice->DestroyVoice();
	g_pBgmVoice = nullptr;
	g_bgmPath.clear();
}

void Sound::SetBGMVolume(float volume)
{
	g_bgmBaseVolume = volume;
	if (g_pBgmVoice) g_pBgmVoice->SetVolume(g_bgmBaseVolume * g_bgmFade);
}

void Sound::SetBGMFade(float factor)
{
	if (factor < 0.0f) factor = 0.0f;
	if (factor > 1.0f) factor = 1.0f;
	g_bgmFade = factor;
	if (g_pBgmVoice) g_pBgmVoice->SetVolume(g_bgmBaseVolume * g_bgmFade);
}