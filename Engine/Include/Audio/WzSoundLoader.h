#pragma once

#include "Audio/USoundWave.h"

// WZ에서 추출한 사운드를 엔진의 공유 PCM 에셋으로 변환한다. 게임 스레드 전용이다.
class FWzSoundLoader
{
public:
	// 예: Base.wz 경로와 Sound\Bgm00.img\GoPicnic 노드 경로.
	// Sound.wz를 직접 열거나 Bgm00/GoPicnic처럼 상대 사운드 경로를 사용해도 된다.
	static TSharedPtr<USoundWave> LoadSound(const char* WzPath, const char* NodePath);

	// 요청한 맵의 info/bgm을 읽어 반복 BGM용 에셋을 준비한다. 자동 재생하지 않는다.
	static TSharedPtr<USoundWave> LoadMapBGM(const char* WzPath, const char* MapPath);

private:
	using FReadSound = const uint8* (*)(const char* WzPath, const char* NodePath, int32* OutSize);
	using FReadMapBGM = const char* (*)(const char* WzPath, const char* MapPath);
	using FFree = void (*)(const void* pBuffer);

	struct FDllState
	{
		HMODULE m_hModule = nullptr;
		FReadSound m_pReadSound = nullptr;
		FReadMapBGM m_pReadMapBGM = nullptr;
		FFree m_pFree = nullptr;
		bool m_bTriedLoad = false;
		bool m_bWarnedMapBGM = false;
	};

	static FDllState& GetDllState();
};
