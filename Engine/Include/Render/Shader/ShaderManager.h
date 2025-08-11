#pragma once

#include "../../EngineInfo.h"

class CShaderManager
{
public:
    bool Init(ID3D11Device* device) 
    { 
        m_Device = device; return true; 
    }

    void Shutdown() {}

    bool CompileVSFromFile(const wchar_t* path, const char* entry, const char* model, ComPtr<ID3DBlob>& outBlob);
    bool CompilePSFromFile(const wchar_t* path, const char* entry, const char* model, ComPtr<ID3DBlob>& outBlob);

    bool CreateVertexShader(const void* bytecode, SIZE_T size, ID3D11VertexShader** outVS);
    bool CreatePixelShader(const void* bytecode, SIZE_T size, ID3D11PixelShader** outPS);
    bool CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC* desc, UINT num, const void* bytecode, SIZE_T size, ID3D11InputLayout** outIL);

private:
    ID3D11Device* m_Device = nullptr;
};

