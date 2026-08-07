이 폴더에는 DirectXTK 헤더 파일을 통째로 복사해 넣습니다.

1. https://github.com/microsoft/DirectXTK 를 아무 곳에나 clone 하거나
   release zip을 받습니다 (이 저장소 안에 넣을 필요는 없습니다 — 빌드
   재료일 뿐입니다).
2. 그 안의 `Inc` 폴더 전체(SpriteBatch.h, CommonStates.h, DirectXHelpers.h 등
   모든 .h 파일)를 이 폴더(`ThirdParty/DirectXTK/Inc/`) 안에 그대로
   복사합니다. 즉 복사가 끝나면 이 경로에
   `ThirdParty/DirectXTK/Inc/SpriteBatch.h` 처럼 파일이 바로 보여야
   합니다(하위 폴더를 한 겹 더 만들지 않습니다).

빌드 없이 파일 복사만으로 끝나는 단계입니다.
