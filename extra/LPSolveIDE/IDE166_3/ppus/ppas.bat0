@echo off
c:\lazarus\fpc\2.2.2\bin\i386-win32\windres.exe --include c:\lazarus\fpc\2.2.2\bin\i386-win32\ -O res -o C:\LAAzProj\IDE16\ppus\LPSolveIDE.res LPSolveIDE.rc --preprocessor=c:\lazarus\fpc\2.2.2\bin\i386-win32\cpp.exe
if errorlevel 1 goto linkend
SET THEFILE=C:\LAAzProj\IDE16\ppus\LPSolveIDE.exe
echo Linking %THEFILE%
c:\lazarus\fpc\2.2.2\bin\i386-win32\ld.exe -b pe-i386 -m i386pe  --gc-sections    --entry=_mainCRTStartup    -o C:\LAAzProj\IDE16\ppus\LPSolveIDE.exe C:\LAAzProj\IDE16\ppus\link.res
if errorlevel 1 goto linkend
c:\lazarus\fpc\2.2.2\bin\i386-win32\postw32.exe --subsystem console --input C:\LAAzProj\IDE16\ppus\LPSolveIDE.exe --stack 262144
if errorlevel 1 goto linkend
goto end
:asmend
echo An error occured while assembling %THEFILE%
goto end
:linkend
echo An error occured while linking %THEFILE%
:end
