cd..
cd..
xcopy .\Engine\Bin\*.* .\Game\Bin\ /s /d /y /exclude:exclude.txt
xcopy .\Engine\Bin\*.* .\GameEngine\Bin\ /s /d /y /exclude:exclude.txt
xcopy .\Engine\Bin\*.* .\Test\Bin\ /s /d /y /exclude:exclude.txt

xcopy .\Engine\Include\*.h .\GameEngine\Include\ /s /d /y
xcopy .\Engine\Include\*.inl .\GameEngine\Include\ /s /d /y
xcopy .\Engine\Include\*.hpp .\GameEngine\Include\ /s /d /y
