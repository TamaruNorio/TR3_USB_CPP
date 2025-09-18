@echo off
REM ==========================================================
REM  TR3_USB_CPP : MSVC simple build (USB ONLY)
REM  Usage:
REM    build_msvc.bat               -> Debug (/MDd)
REM    build_msvc.bat release       -> Release (/O2 /MD)
REM    build_msvc.bat clean         -> remove build & leftovers
REM    build_msvc.bat rebuild       -> clean + build
REM ==========================================================

setlocal EnableExtensions
chcp 65001 >nul

set "ROOT=%~dp0"
set "SRC=%ROOT%src"
set "INC=%ROOT%include"
set "BUILD=%ROOT%build"
set "OUT_EXE=%BUILD%\tr3_usb.exe"
set "OUT_PDB=%BUILD%\tr3_usb.pdb"
set "OBJDIR=%BUILD%\"
set "CCPDB=%BUILD%\vc140.pdb"

if /i "%1"=="clean" (
  echo [CLEAN] removing build folder and leftovers...
  if exist "%BUILD%" rmdir /s /q "%BUILD%"
  del /q "%ROOT%*.obj"  "%ROOT%*.pdb"  "%ROOT%*.ilk"  "%ROOT%*.idb"  "%ROOT%*.iobj"  "%ROOT%*.ipdb"  "%ROOT%*.exp"  "%ROOT%*.lib"  2>nul
  echo [DONE] Clean complete.
  exit /b 0
)

if /i "%1"=="rebuild" (
  call "%~f0" clean
  call "%~f0" %2
  exit /b %errorlevel%
)

if not exist "%BUILD%" mkdir "%BUILD%"

echo [BUILD] Compiling ...

set "CFLAGS=/nologo /EHsc /std:c++17 /W4 /utf-8 /Zi /Fo:%OBJDIR% /Fd:%CCPDB%"
if /i "%1"=="release" (
  set "CFLAGS=%CFLAGS% /O2 /MD"
  set "LFLAGS=/INCREMENTAL:NO /PDB:%OUT_PDB%"
) else (
  set "CFLAGS=%CFLAGS% /MDd"
  set "LFLAGS=/INCREMENTAL /DEBUG /PDB:%OUT_PDB%"
)

cl %CFLAGS% ^
  /I "%INC%" ^
  "%SRC%\main.cpp" ^
  "%SRC%\serial_port.cpp" ^
  "%SRC%\tr3_protocol.cpp" ^
  /link %LFLAGS% /OUT:%OUT_EXE%

if errorlevel 1 (
  echo [ERROR] Build failed.
  exit /b 1
)

echo [SUCCESS] Output: %OUT_EXE%
echo Run: "%OUT_EXE%"   （※起動後に COM / ボーレート / インベントリ試行回数を対話入力）
exit /b 0
