이 폴더에는 DirectXTK.lib 파일 두 개(Debug용, Release용)를 하위 폴더
구분 없이 이름으로만 구분해서 나란히 넣습니다(`Engine.lib`/
`Engine_Debug.lib`를 구분하는 방식과 동일).

1. https://github.com/microsoft/DirectXTK 를 아무 곳에나 clone 하거나
   release zip을 받습니다(이 저장소 안에 넣을 필요 없음).
2. 그 안의 DirectXTK_Desktop_2022.sln을 Visual Studio로 엽니다.
3. 구성을 Debug | x64로 맞추고 빌드합니다. 결과물로 나온
   `DirectXTK.lib` 파일의 이름을 **`DirectXTK_Debug.lib`로 바꿔서**
   이 폴더(`ThirdParty/DirectXTK/Lib/`) 안에 복사합니다.
4. 구성을 Release | x64로 바꿔서 다시 빌드합니다. 결과물로 나온
   `DirectXTK.lib`는 **이름을 바꾸지 않고 그대로** 이 폴더 안에
   복사합니다.
5. 최종적으로 이 폴더 안에 다음 두 파일이 함께 있어야 합니다:
   - `ThirdParty/DirectXTK/Lib/DirectXTK_Debug.lib` (Debug용)
   - `ThirdParty/DirectXTK/Lib/DirectXTK.lib` (Release용)

Debug/Release 빌드는 CRT 링크 방식 등이 서로 달라 호환되지 않으므로,
두 파일을 절대 서로 바꿔 넣지 않도록 주의합니다.
