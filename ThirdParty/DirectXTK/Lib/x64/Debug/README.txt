이 폴더에는 DirectXTK를 Debug|x64 구성으로 빌드했을 때 나오는
DirectXTK.lib 파일 하나만 넣습니다.

1. https://github.com/microsoft/DirectXTK 를 아무 곳에나 clone 하거나
   release zip을 받습니다.
2. 그 안의 DirectXTK_Desktop_2022.sln을 Visual Studio로 엽니다.
3. 구성을 Debug | x64로 맞추고 빌드합니다(Build Solution).
4. 빌드가 끝나면 출력 창(Output)에 DirectXTK.lib가 만들어진 정확한
   경로가 표시됩니다 — 보통 그 저장소 폴더 아래
   Bin\Desktop_2022\x64\Debug\ 계열의 경로입니다.
5. 그 DirectXTK.lib 파일을 이 폴더(ThirdParty/DirectXTK/Lib/x64/Debug/)
   안에 복사해 넣습니다. 최종적으로
   ThirdParty/DirectXTK/Lib/x64/Debug/DirectXTK.lib 파일이 있어야 합니다.

Release 구성용 DirectXTK.lib는 이 폴더가 아니라
ThirdParty/DirectXTK/Lib/x64/Release/ 폴더에 따로 넣어야 합니다(구성별로
빌드 옵션이 달라 Debug용과 Release용 .lib는 서로 다른 파일입니다).
