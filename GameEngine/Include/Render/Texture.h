//#pragma once
//
//#include <d3d11.h>
//#include "../SmartPointer/TSharedPtr.h"
//#include "../Math/FVector2.h" 
//#include "../Container/FName.h"
//
//class CTexture
//{
//public:
//    CTexture();
//    ~CTexture();
//
//    bool LoadFromFile(const wchar_t* InPath);
//    void Bind(unsigned int Slot = 0);
//
//    FVector2 GetSize() const 
//    { 
//        return m_Size; 
//    }
//
//    const FName& GetName() const 
//    { 
//        return m_Name; 
//    }
//
//private:
//    ID3D11ShaderResourceView* m_ShaderResourceView;
//    ID3D11Texture2D* m_Texture;
//    FVector2 m_Size;
//    FName m_Name;
//};
//
