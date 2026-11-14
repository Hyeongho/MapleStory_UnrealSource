#pragma once

#include "EnginePCH.h"

class FDXDevice;

// WIC가 지원하는 일반 이미지 파일을 정적 2D 텍스처로 읽는다.
class FTextureLoader
{
public:
	// PNG, JPEG, BMP, GIF, TIFF 등 지원 형식은 Windows에 설치된 WIC 코덱을 따른다.
	// 한글 경로는 wchar_t로 전달한다. 상대 경로는 프로세스의 현재 작업 폴더 기준이다.
	// 다중 프레임 이미지는 첫 프레임만 읽으며 반환 크기는 생성된 GPU 텍스처의 실제 크기다.
	// 호출 스레드는 COM 초기화가 필요하다. 기본 게임 스레드는 UEngine::Init에서 준비한다.
	// 성공: 호출자가 Release할 참조 1개를 반환한다. 실패: nullptr, 출력 크기는 0이다.
	static ID3D11ShaderResourceView* LoadTextureFromFile(FDXDevice& Device, const wchar_t* FilePath,
		int32* OutWidth = nullptr, int32* OutHeight = nullptr);
};
