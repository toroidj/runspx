@echo off
set RETAIL=1
rem *** set value ***
set arcname=runspx11.zip
set readme=runspx.txt
set srcname=runspxsrc.lzh

rem *** main ***
rem WINCLOSE xxx
del /q *.zip 2> NUL
del /q *.lzh 2> NUL
del /q *.map 2> NUL
del /q *.obj 2> NUL
del /q *.res 2> NUL
call m64 -DRELEASE

del /q *.obj 2> NUL
del /q *.res 2> NUL
call mvc -DRELEASE

del /q *.obj 2> NUL
del /q *.res 2> NUL
call marm64 -DRELEASE

for %%i in (*.exe) do tfilesign sp %%i %%i
for %%i in (*.exe) do CT %readme% %%i

rem *** Source Archive ***
if %RETAIL%==0 goto :skipsource

for %%i in (*) do CT %readme% %%i
ppb /c %%u/UNLHA32.DLL,a %~dp0\%srcname% -z -n -r2x MAKE*. GNUmake*. *.BAT *.C *.CPP *.H *.HPJ *.pl *.RC *.RH *.ICO *.DEF *.sln *.vcproj
CT %readme% %srcname%
:skipsource

ppb /c %%u/7-ZIP32.DLL,a -tzip -hide -mx=7 %arcname% %readme% *.exe %srcname%
tfilesign s %arcname% %arcname%
CT %readme% %arcname%
