@echo off
set CompilerFlags= /Z7 /FC /nologo /Od -fp:fast -Gm- -GR- -EHa- /Oi -WX -W4 -wd4189 -wd4505 -wd4100 -wd4201
set LinkerFlags=-subsystem:console 

mkdir build > NUL 2> NUL
pushd build
call "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat" x86
del *.pdb > NUL 2> NUL
cl %CompilerFlags% /MTd ..\code\gamecode.cpp -Fmgamecode.map -LD /link -incremental:no -opt:ref -PDB:handmade_%RANDOM%.pdb -EXPORT:GameUpdateAndRender -out:GameCode.dll
cl %CompilerFlags% /MD ..\code\sdllayer.cpp /I..\deps\include  /link -incremental:no ..\deps\lib\x86\SDL2.lib ..\deps\lib\x86\SDL2main.lib opengl32.lib  %LinkerFlags%
popd
