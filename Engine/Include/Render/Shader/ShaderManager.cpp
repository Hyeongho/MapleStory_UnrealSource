#include "ShaderManager.h"

bool CShaderManager::CompileVSFromFile(const wchar_t* path, const char* entry, const char* model, ComPtr<ID3DBlob>& outBlob)
{
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;

#if defined(_DEBUG)
    flags |= D3DCOMPILE_DEBUG;
#endif

    ComPtr<ID3DBlob> errors;
    HRESULT hr = D3DCompileFromFile(path, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entry, model, flags, 0, outBlob.GetAddressOf(), errors.GetAddressOf());

    if (FAILED(hr) && errors) 
    {
        OutputDebugStringA((char*)errors->GetBufferPointer());
    }

    return SUCCEEDED(hr);
}

bool CShaderManager::CompilePSFromFile(const wchar_t* path, const char* entry, const char* model, ComPtr<ID3DBlob>& outBlob)
{
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;

#if defined(_DEBUG)
    flags |= D3DCOMPILE_DEBUG;
#endif

    ComPtr<ID3DBlob> errors;
    HRESULT hr = D3DCompileFromFile(path, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entry, model, flags, 0, outBlob.GetAddressOf(), errors.GetAddressOf());

    if (FAILED(hr) && errors) 
    {
        OutputDebugStringA((char*)errors->GetBufferPointer());
    }

    return SUCCEEDED(hr);
}

bool CShaderManager::CreateVertexShader(const void* bc, SIZE_T sz, ID3D11VertexShader** outVS)
{
    return SUCCEEDED(m_Device->CreateVertexShader(bc, sz, nullptr, outVS));
}

bool CShaderManager::CreatePixelShader(const void* bc, SIZE_T sz, ID3D11PixelShader** outPS)
{
    return SUCCEEDED(m_Device->CreatePixelShader(bc, sz, nullptr, outPS));
}

bool CShaderManager::CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC* desc, UINT num, const void* bc, SIZE_T sz, ID3D11InputLayout** outIL)
{
    return SUCCEEDED(m_Device->CreateInputLayout(desc, num, bc, sz, outIL));
}