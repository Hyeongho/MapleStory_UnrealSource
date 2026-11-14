#include "EnginePCH.h"
#include "Audio/WzSoundLoader.h"

FWzSoundLoader::FDllState& FWzSoundLoader::GetDllState()
{
	static FDllState State;

	if (State.m_bTriedLoad)
	{
		return State;
	}

	State.m_bTriedLoad = true;

	// 기존 이미지·맵 로더와 같은 DLL을 사용한다. 성공한 모듈은 프로세스 수명 동안 유지한다.
	State.m_hModule = LoadLibraryW(L"WzNativeLib.dll");
	if (!State.m_hModule)
	{
		UE_LOG(LogAudio, Warning, L"[WzSound] WzNativeLib.dll unavailable: error=%u", (uint32)GetLastError());
		return State;
	}

	State.m_pReadSound = reinterpret_cast<FReadSound>(GetProcAddress(State.m_hModule, "wz_read_sound"));
	State.m_pReadMapBGM = reinterpret_cast<FReadMapBGM>(GetProcAddress(State.m_hModule, "wz_map_read_bgm"));
	State.m_pFree = reinterpret_cast<FFree>(GetProcAddress(State.m_hModule, "wz_free"));

	if (!State.m_pReadSound || !State.m_pFree)
	{
		UE_LOG(LogAudio, Warning, L"[WzSound] sound API unavailable; publish and deploy updated WzNativeLib.dll");
	}

	return State;
}

TSharedPtr<USoundWave> FWzSoundLoader::LoadSound(const char* WzPath, const char* NodePath)
{
	if (!WzPath || !*WzPath || !NodePath || !*NodePath)
	{
		return TSharedPtr<USoundWave>();
	}

	FDllState& State = GetDllState();

	if (!State.m_pReadSound || !State.m_pFree)
	{
		return TSharedPtr<USoundWave>();
	}

	int32 Size = 0;
	const uint8* pBytes = State.m_pReadSound(WzPath, NodePath, &Size);
	TSharedPtr<USoundWave> Sound;

	if (pBytes && Size > 0)
	{
		// 입력 바이트를 메모리에서 디코딩해 PCM 에셋으로 만든다. 파일 저장은 하지 않는다.
		Sound = USoundWave::LoadFromMemory(pBytes, (uint32)Size);
	}

	// 잘못된 크기가 반환되거나 디코딩이 실패한 경우에도 DLL 버퍼는 반드시 해제한다.
	if (pBytes)
	{
		State.m_pFree(pBytes);
	}

	if (Sound)
	{
		UE_LOG(LogAudio, Log, L"[WzSound] loaded: node=%hs bytes=%u duration=%.2fs",
			NodePath, Sound->GetPCMSize(), Sound->GetDuration());
	}

	else
	{
		UE_LOG(LogAudio, Warning, L"[WzSound] load failed: node=%hs encodedBytes=%d", NodePath, Size);
	}

	return Sound;
}

TSharedPtr<USoundWave> FWzSoundLoader::LoadMapBGM(const char* WzPath, const char* MapPath)
{
	if (!WzPath || !*WzPath || !MapPath || !*MapPath)
	{
		return TSharedPtr<USoundWave>();
	}

	FDllState& State = GetDllState();

	if (!State.m_pReadSound || !State.m_pFree)
	{
		return TSharedPtr<USoundWave>();
	}

	if (!State.m_pReadMapBGM)
	{
		if (!State.m_bWarnedMapBGM)
		{
			UE_LOG(LogAudio, Warning, L"[WzSound] wz_map_read_bgm unavailable; deploy updated WzNativeLib.dll");
			State.m_bWarnedMapBGM = true;
		}

		return TSharedPtr<USoundWave>();
	}

	const char* pNodePath = State.m_pReadMapBGM(WzPath, MapPath);

	if (!pNodePath)
	{
		// BGM이 없는 맵은 정상적으로 무음이다.
		return TSharedPtr<USoundWave>();
	}

	TSharedPtr<USoundWave> Sound = LoadSound(WzPath, pNodePath);
	State.m_pFree(pNodePath);

	return Sound;
}
