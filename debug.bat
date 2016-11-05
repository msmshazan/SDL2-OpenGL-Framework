@echo off
pushd build
call "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat" x64
devenv sdllayer.exe
popd