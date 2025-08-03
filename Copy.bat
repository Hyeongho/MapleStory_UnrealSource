cd..
cd..
xcopy .\Engine\Include\*.h .\GameEngine\Include\ /d /s /y
xcopy .\Engine\Include\*.inl .\GameEngine\Include\ /d /s /y
xcopy .\Engine\Include\*.hpp .\GameEngine\Include\ /d /s /y
xcopy .\Engine\Bin\*.* .\GameEngine\Bin\ /d /s /y /exclude:Exclude.txt
xcopy .\Engine\Bin\*.* .\Editor\Bin\ /d /s /y /exclude:Exclude.txt
xcopy .\Engine\Bin\*.* .\Client\Bin\ /d /s /y /exclude:Exclude.txt