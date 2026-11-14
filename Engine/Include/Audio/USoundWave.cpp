#include "EnginePCH.h"
#include "Audio/USoundWave.h"

USoundWave::USoundWave() = default;

USoundWave::~USoundWave()
{
	FMemory::Free(m_pPCMData);
}

TSharedPtr<USoundWave> USoundWave::LoadFromFile(const wchar_t* FilePath)
{
	if (!FilePath || !*FilePath)
	{
		return TSharedPtr<USoundWave>();
	}

	TSharedPtr<USoundWave> Sound = MakeShared<USoundWave>();

	if (!Sound->DecodeFile(FilePath))
	{
		return TSharedPtr<USoundWave>();
	}

	UE_LOG(LogAudio, Log, L"[SoundWave] loaded: path=%ls bytes=%u duration=%.2fs", FilePath, Sound->GetPCMSize(), Sound->GetDuration());

	return Sound;
}

TSharedPtr<USoundWave> USoundWave::LoadFromMemory(const uint8* pData, uint32 Size)
{
	if (!pData || Size == 0 || Size > 256u * 1024u * 1024u)
	{
		return TSharedPtr<USoundWave>();
	}

	TSharedPtr<USoundWave> Sound = MakeShared<USoundWave>();

	if (!Sound->DecodeMemory(pData, Size))
	{
		return TSharedPtr<USoundWave>();
	}

	return Sound;
}

WAVEFORMATEX USoundWave::GetWaveFormat()
{
	WAVEFORMATEX Format = {};
	Format.wFormatTag = WAVE_FORMAT_PCM;
	Format.nChannels = 2;
	Format.nSamplesPerSec = 48000;
	Format.wBitsPerSample = 16;
	Format.nBlockAlign = 4;
	Format.nAvgBytesPerSec = 192000;
	return Format;
}

const uint8* USoundWave::GetPCMData() const
{
	return m_pPCMData;
}

uint32 USoundWave::GetPCMSize() const
{
	return m_PCMSize;
}

float USoundWave::GetDuration() const
{
	return (float)m_PCMSize / (float)GetWaveFormat().nAvgBytesPerSec;
}

bool USoundWave::DecodeFile(const wchar_t* FilePath)
{
	Microsoft::WRL::ComPtr<IMFSourceReader> Reader;

	const HRESULT Result = MFCreateSourceReaderFromURL(FilePath, nullptr, Reader.GetAddressOf());

	if (FAILED(Result))
	{
		UE_LOG(LogAudio, Warning, L"[SoundWave] open failed: path=%ls hr=0x%08X", FilePath, (uint32)Result);
		return false;
	}

	return DecodeReader(Reader.Get());
}

bool USoundWave::DecodeMemory(const uint8* pData, uint32 Size)
{
	// 스트림이 입력 바이트를 복사해 소유하므로 DLL 반환 버퍼의 수명과 분리된다.
	Microsoft::WRL::ComPtr<IStream> Stream;
	Stream.Attach(SHCreateMemStream(pData, Size));

	Microsoft::WRL::ComPtr<IMFByteStream> ByteStream;
	Microsoft::WRL::ComPtr<IMFSourceResolver> Resolver;
	Microsoft::WRL::ComPtr<IUnknown> SourceObject;
	Microsoft::WRL::ComPtr<IMFMediaSource> Source;
	Microsoft::WRL::ComPtr<IMFSourceReader> Reader;

	HRESULT Result = Stream ? MFCreateMFByteStreamOnStream(Stream.Get(), ByteStream.GetAddressOf()) : E_OUTOFMEMORY;
	if (SUCCEEDED(Result))
	{
		Result = MFCreateSourceResolver(Resolver.GetAddressOf());
	}

	if (SUCCEEDED(Result))
	{
		// 확장자는 처리기 선택용 힌트다. 데이터는 메모리 스트림에서만 읽으며 파일을 열지 않는다.
		const bool bWave = Size >= 12 && memcmp(pData, "RIFF", 4) == 0 && memcmp(pData + 8, "WAVE", 4) == 0;

		MF_OBJECT_TYPE ObjectType = MF_OBJECT_INVALID;

		Result = Resolver->CreateObjectFromByteStream(ByteStream.Get(), bWave ? L"wz.wav" : L"wz.mp3", MF_RESOLUTION_MEDIASOURCE, nullptr, &ObjectType, SourceObject.GetAddressOf());
	}

	if (SUCCEEDED(Result))
	{
		Result = SourceObject.As(&Source);
	}

	if (SUCCEEDED(Result))
	{
		Result = MFCreateSourceReaderFromMediaSource(Source.Get(), nullptr, Reader.GetAddressOf());
	}

	bool bDecoded = false;

	if (FAILED(Result))
	{
		UE_LOG(LogAudio, Warning, L"[SoundWave] memory stream failed: hr=0x%08X bytes=%u", (uint32)Result, Size);
	}

	else
	{
		bDecoded = DecodeReader(Reader.Get());
	}

	// 메모리 입력에서 생성한 소스의 백그라운드 작업도 함수 반환 전에 종료한다.
	Reader.Reset();

	if (Source)
	{
		Source->Shutdown();
	}

	return bDecoded;
}

bool USoundWave::DecodeReader(IMFSourceReader* pReader)
{
	// Source Reader가 Windows 코덱으로 디코딩과 리샘플링을 수행한다.
	// 보이스 풀을 공유할 수 있도록 모든 파일을 48kHz / 스테레오 / 16bit PCM으로 맞춘다.
	Microsoft::WRL::ComPtr<IMFMediaType> MediaType;
	HRESULT Result = pReader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS, FALSE);

	if (SUCCEEDED(Result))
	{
		Result = pReader->SetStreamSelection(MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);
	}

	if (SUCCEEDED(Result))
	{
		Result = MFCreateMediaType(MediaType.GetAddressOf());
	}

	if (SUCCEEDED(Result))
	{
		Result = MediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
	}

	if (SUCCEEDED(Result))
	{
		Result = MediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
	}

	if (SUCCEEDED(Result))
	{
		Result = MediaType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, 2);
	}

	if (SUCCEEDED(Result))
	{
		Result = MediaType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000);
	}

	if (SUCCEEDED(Result))
	{
		Result = MediaType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
	}

	if (SUCCEEDED(Result))
	{
		Result = MediaType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4);
	}

	if (SUCCEEDED(Result))
	{
		Result = MediaType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 192000);
	}

	if (SUCCEEDED(Result))
	{
		Result = pReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, MediaType.Get());
	}

	if (SUCCEEDED(Result) && !IsExpectedFormat(pReader))
	{
		Result = MF_E_INVALIDMEDIATYPE;
	}

	while (SUCCEEDED(Result))
	{
		DWORD Flags = 0;
		Microsoft::WRL::ComPtr<IMFSample> Sample;
		Result = pReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &Flags, nullptr, Sample.GetAddressOf());
		if (FAILED(Result))
		{
			break;
		}

		if (Flags & MF_SOURCE_READERF_ERROR)
		{
			Result = E_FAIL;
			break;
		}

		if ((Flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED) && !IsExpectedFormat(pReader))
		{
			// 메타데이터만 바뀐 경우는 허용하되, 서로 다른 PCM 포맷은 이어 붙이지 않는다.
			Result = MF_E_INVALIDMEDIATYPE;
			break;
		}

		if (Sample)
		{
			Microsoft::WRL::ComPtr<IMFMediaBuffer> Buffer;
			Result = Sample->ConvertToContiguousBuffer(Buffer.GetAddressOf());

			if (FAILED(Result))
			{
				break;
			}

			BYTE* pBytes = nullptr;
			DWORD Size = 0;
			Result = Buffer->Lock(&pBytes, nullptr, &Size);

			if (FAILED(Result))
			{
				break;
			}

			const bool bAppended = AppendPCM(pBytes, Size);
			const HRESULT UnlockResult = Buffer->Unlock();

			Result = bAppended ? UnlockResult : E_OUTOFMEMORY;

			if (FAILED(Result))
			{
				break;
			}
		}

		// 마지막 샘플과 종료 플래그가 함께 올 수도 있으므로 데이터 처리 뒤 검사한다.
		if (Flags & MF_SOURCE_READERF_ENDOFSTREAM)
		{
			break;
		}
	}

	if (FAILED(Result) || m_PCMSize == 0 || m_PCMSize % GetWaveFormat().nBlockAlign != 0)
	{
		UE_LOG(LogAudio, Warning, L"[SoundWave] decode failed: hr=0x%08X bytes=%u", (uint32)(FAILED(Result) ? Result : E_FAIL), m_PCMSize);
		return false;
	}

	// 디코딩 중 여유 공간만 정리한다. 재생 중에는 버퍼를 변경하지 않는다.
	if (void* pExactData = FMemory::Realloc(m_pPCMData, m_PCMSize))
	{
		m_pPCMData = static_cast<uint8*>(pExactData);
		m_PCMCapacity = m_PCMSize;
	}

	return true;
}

bool USoundWave::IsExpectedFormat(IMFSourceReader* pReader) const
{
	Microsoft::WRL::ComPtr<IMFMediaType> CurrentType;
	if (!pReader || FAILED(pReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, CurrentType.GetAddressOf())))
	{
		return false;
	}
	GUID Subtype = {};
	UINT32 Channels = 0;
	UINT32 SampleRate = 0;
	UINT32 BitsPerSample = 0;
	UINT32 BlockAlign = 0;
	return SUCCEEDED(CurrentType->GetGUID(MF_MT_SUBTYPE, &Subtype)) && IsEqualGUID(Subtype, MFAudioFormat_PCM)
		&& SUCCEEDED(CurrentType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &Channels)) && Channels == 2
		&& SUCCEEDED(CurrentType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &SampleRate)) && SampleRate == 48000
		&& SUCCEEDED(CurrentType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &BitsPerSample)) && BitsPerSample == 16
		&& SUCCEEDED(CurrentType->GetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, &BlockAlign)) && BlockAlign == 4;
}

bool USoundWave::AppendPCM(const uint8* pData, uint32 Size)
{
	// 잘못된 파일 또는 긴 파일 하나가 메모리를 무제한 소비하지 않도록 제한한다.
	static constexpr uint32 MaxPCMSize = 256u * 1024u * 1024u;
	if (Size > MaxPCMSize - m_PCMSize || (Size > 0 && !pData))
	{
		return false;
	}

	const uint32 Required = m_PCMSize + Size;

	if (Required > m_PCMCapacity)
	{
		const uint32 NewCapacity = FMath::Max(Required, FMath::Max(65536u, m_PCMCapacity * 2));

		void* pNewData = FMemory::Realloc(m_pPCMData, NewCapacity);

		if (!pNewData) 
		{
			return false;
		}

		m_pPCMData = static_cast<uint8*>(pNewData);
		m_PCMCapacity = NewCapacity;
	}

	if (Size > 0) 
	{
		FMemory::Memcpy(m_pPCMData + m_PCMSize, pData, Size);
	}

	m_PCMSize = Required;

	return true;
}