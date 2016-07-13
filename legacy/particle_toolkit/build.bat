@echo off
IF NOT EXIST build mkdir build
pushd build
cl -nologo /EHsc /FC /Zi ..\code\particle_toolkit.cpp user32.lib kernel32.lib gdi32.lib winmm.lib
popd