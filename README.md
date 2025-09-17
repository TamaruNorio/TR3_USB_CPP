# TR3_USB_CPP

TAKAYA TR3シリーズ（USB接続/仮想COM）を **Windows + C++** から制御するサンプル。
最小限の構成で、バージョン取得とシングルリードを確認できます。

## 動作環境
- Windows 10/11
- Visual Studio 2022 (MSVC)
- CMake 3.20+
- VSCode + CMake Tools 拡張（推奨）
- TR3（USB接続 / CDC 仮想COM 想定）

## 使い方（3ステップ）
1. 接続し、デバイスマネージャーで TR3 の **COMポート番号** を確認（例: COM5）
2. ビルド
   - `CMake: Configure` → `CMake: Build`（既定構成）
3. 実行
   - `build\Debug\tr3_usb.exe COM5`

## コード構成
- `include/serial_port.hpp, src/serial_port.cpp`
  - Win32 API で COM を開く/読む/書く
- `include/tr3_protocol.hpp, src/tr3_protocol.cpp`
  - TR3向けのコマンド送受（例: `VER`, `READ`）
- `src/main.cpp`
  - 動作確認用CLI

## 注意
- コマンド名/改行/ボーレート等は **実機仕様に合わせて調整** してください。
- CDC ではなく WinUSB/HID の場合、輸送層（`SerialPort`）を差し替えてください。
