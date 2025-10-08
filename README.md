## TAKAYA RFID リーダライタ サンプルプログラム ドキュメント

> **ドキュメントの全体像や他のサンプルプログラムについては、[こちらのランディングページ](https://TamaruNorio.github.io/TAKAYA-RFID-Sample-Docs/cpp/index.md)をご覧ください。**

# TR3_USB_CPP

タカヤ製 RFID リーダ／ライタ（TR3 シリーズ・HF 13.56MHz）を **Windows + MSVC** で制御する最小サンプルです。ビルドは **x64 Native Tools Command Prompt for VS 2022** から付属のバッチで行います。

## 概要

このサンプルプログラムは、TR3シリーズ（HF 13.56MHz）をUSBシリアル接続で制御するためのC++サンプルです。ROMバージョン取得、動作モード制御、Inventory2、ブザー制御といった基本的なRFID操作を実装しています。

## 動作環境

-   OS: Windows 10 / 11 (64bit)
-   開発環境: Visual Studio 2022（C++/MSVC）
-   ハードウェア: TR3 シリーズ（HF 13.56MHz / USBモデル）

## セットアップと実行方法

1.  **開発環境の準備**: Visual Studio 2022（C++/MSVC）がインストール済みであることを確認してください。
2.  **リポジトリのクローン**:
    ```bash
    git clone https://github.com/TamaruNorio/TR3_USB_CPP.git
    cd TR3_USB_CPP
    ```
3.  **ビルド**: **x64 Native Tools Command Prompt for VS 2022** を起動し、本フォルダへ移動して次のいずれかを実行します。
    ```bat
    build_msvc.bat          # Debug ビルド（既定）
    build_msvc.bat release  # Release ビルド
    build_msvc.bat clean    # 生成物の削除
    ```
    生成物はすべて `build/` に出力され、実行ファイルは `build\tr3_usb.exe` です。
4.  **実行**: プログラム起動後、**COM ポート**／**ボーレート**／**インベントリ試行回数**を対話入力します。ボーレートの既定は **19200 bps**（TR3 標準）です。

### 実行フロー

-   起動後にまず **ROMバージョン**を取得して疎通確認します。
-   つづいて **リーダ／ライタ動作モード**を読み取り、**「モードのみ」コマンドモード(0x00)へ変更**（アンチコリジョン／読取り動作／ブザー／送信データ／通信速度は**現状維持**）します。
-   **Inventory2** を指定回数実行します。
    -   タグが1件以上：**ピー(0x00)** を鳴らす
    -   タグ0件／エラー：**ピッピッピ(0x01)** を鳴らす
    -   ブザー制御は **CMD: 0x42 / Data: [応答要求(0x01), 音種]** を使用

## プロジェクト構成

```
TR3_USB_CPP/
├─ build/                 ← 生成物（.exe / .obj / .pdb など）を集約
├─ include/
│   ├─ serial_port.hpp
│   └─ tr3_protocol.hpp
├─ src/
│   ├─ main.cpp           ← 実行エントリ（対話UI）
│   ├─ serial_port.cpp    ← シリアル I/O（Win32 API）
│   └─ tr3_protocol.cpp   ← TR3 プロトコル（ROM版取得・動作モード・Inventory2・ブザー等）
├─ build_msvc.bat         ← ビルド用バッチ
└─ README.md
```

## ライセンス

本プログラムはサンプルです。ご利用は自己責任でお願いします。仕様の詳細は **通信プロトコル説明書** を参照してください（TR3 シリーズ向け）。

