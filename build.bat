@echo off

SET "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community"
SET "SOLUTION=Velo_UI.sln"
SET "CONFIG=Release"
SET "PLATFORM=x86"

CALL "%VS_PATH%\VC\Auxiliary\Build\vcvarsall.bat" x86

msbuild.exe "%SOLUTION%" /m /p:Configuration=%CONFIG% /p:Platform=%PLATFORM%

pause