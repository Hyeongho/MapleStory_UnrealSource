#pragma once

#include "EnginePCH.h"
#include "Object/UObject.h"
#include "Core/SmartPointer/TSharedPtr.h"

// 여러 보이스가 공유하는 불변 PCM 에셋. 현재 엔진은 GC 대신 TSharedPtr로 수명을 관리한다.
class USoundWave : public UObject
{
	DECLARE_CLASS(USoundWave, UObject)

public:
	// 생성 / 소멸
	USoundWave();
	virtual ~USoundWave() override;
	USoundWave(const USoundWave&) = delete;
	USoundWave& operator=(const USoundWave&) = delete;

	// 엔진 오디오 초기화 후 호출한다. 실패하면 빈 포인터를 반환한다.
	// 파일을 한 번 디코딩하며, 재생과 루프에서는 파일을 다시 읽지 않는다.
	static TSharedPtr<USoundWave> LoadFromFile(const wchar_t* FilePath);
	// MP3 또는 WAV 파일 바이트를 메모리에서 디코딩한다. 입력은 호출자가 소유한다.
	// 반환된 뒤 입력 버퍼를 해제할 수 있으며 임시 파일을 생성하지 않는다.
	static TSharedPtr<USoundWave> LoadFromMemory(const uint8* pData, uint32 Size);
	static WAVEFORMATEX GetWaveFormat();

	// 이 에셋이 살아 있는 동안만 유효한 읽기 전용 데이터.
	const uint8* GetPCMData() const;
	uint32 GetPCMSize() const;
	float GetDuration() const;

private:
	bool DecodeFile(const wchar_t* FilePath);
	bool DecodeMemory(const uint8* pData, uint32 Size);
	bool DecodeReader(IMFSourceReader* pReader);
	bool IsExpectedFormat(IMFSourceReader* pReader) const;
	bool AppendPCM(const uint8* pData, uint32 Size);

	uint8* m_pPCMData = nullptr;
	uint32 m_PCMSize = 0;
	uint32 m_PCMCapacity = 0;
};
