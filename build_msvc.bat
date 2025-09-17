@echo off
REM ==========================================================
REM  TR3_USB_CPP : MSVC simple build (USB ONLY)
REM  Requires: "Developer Command Prompt for VS" (cl in PATH)
REM
REM  Usage:
REM    build_msvc.bat               -> Debug (/MDd, PDBあり)
REM    build_msvc.bat release       -> Release (/O2 /MD, PDB最小)
REM    build_msvc.bat clean         -> build/ と生成物を削除
REM    build_msvc.bat rebuild       -> clean のちにビルド
REM ==========================================================

setlocal

set "ROOT=%~dp0"
set "SRC=%ROOT%src"
set "INC=%ROOT%include"
set "BUILD=%ROOT%build"
set "OUT_EXE=%BUILD%\tr3_usb.exe"
set "OUT_PDB=%BUILD%\tr3_usb.pdb"
set "CCPDB=%BUILD%\vc140.pdb"   REM コンパイラ用PDB(名前は任意)
set "OBJDIR=%BUILD%\"           REM /Fo で末尾に \ を付けるとディレクトリ指定

if /i "%1"=="clean" (
  echo [CLEAN] removing build folder and leftovers...
  if exist "%BUILD%" rmdir /s /q "%BUILD%"
  REM 念のためプロジェクト直下に残った中間生成物も削除
  del /q "%ROOT%*.obj" 2>nul
  del /q "%ROOT%*.pdb" 2>nul
  del /q "%ROOT%*.ilk" 2>nul
  del /q "%ROOT%*.idb" 2>nul
  del /q "%ROOT%*.iobj" 2>nul
  del /q "%ROOT%*.ipdb" 2>nul
  del /q "%ROOT%*.exp" 2>nul
  del /q "%ROOT%*.lib" 2>nul
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

REM /Fo  : オブジェクトの出力先ディレクトリ（末尾 \ で可）
REM /Fd  : Compiler PDBの出力先
REM /Zi  : デバッグ情報
REM /MDd : Debugランタイム, /MD : Releaseランタイム
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
