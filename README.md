# TR3_USB_CPP

タカヤ製 RFID リーダ／ライタ（TR3 シリーズ・HF 13.56MHz）を **Windows + MSVC** で制御する最小サンプルです。  
ビルドは **x64 Native Tools Command Prompt for VS 2022** から付属のバッチで行います。

---

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
├─ build_msvc.bat         ← ★ビルド用（このバッチだけ使います）
└─ README.md
```

---

## ビルド手順（バッチのみ）

1. **Visual Studio 2022**（C++/MSVC）をインストール済みであることを確認
2. **x64 Native Tools Command Prompt for VS 2022** を起動
3. 本フォルダへ移動して次のいずれかを実行

### Debug ビルド（既定）
```bat
build_msvc.bat
```

### Release ビルド
```bat
build_msvc.bat release
```

### クリーン（生成物を削除）
```bat
build_msvc.bat clean
```

> 生成物はすべて **`build/`** に出力され、ルート直下に “ゴミ” は残りません。

---

## 出力ファイル

- 実行ファイル: `build\tr3_usb.exe`  
- 中間ファイル: `build\*.obj`, `build\*.pdb`, `build\*.ilk` など（`clean` で削除可能）

---

## 実行方法（対話式）

プログラム起動後、**COM ポート**／**ボーレート**／**インベントリ試行回数**を対話入力します。  
ボーレートの既定は **19200 bps**（TR3 標準）です。

### 例（ビルド直後のログ）
```bat
C:\Users\tamaru\github\TR3_USB_CPP>build_msvc.bat
[BUILD] Compiling ...
main.cpp
serial_port.cpp
tr3_protocol.cpp
コードを生成中...
[SUCCESS] Output: C:\Users\tamaru\github\TR3_USB_CPP\build\tr3_usb.exe
Run: "C:\Users\tamaru\github\TR3_USB_CPP\build\tr3_usb.exe"   （※起動後に COM / ボーレート / インベントリ試行回数を対話入力）
```

### 例（実行時の典型ログ）
```
=== 利用可能なCOMポート ===
  [0] COM13
  [1] COM18
  [2] COM23
使用する番号（Enterで0）: 1
=== ボーレート（Enterで19200） ===
  [0] 19200 bps
  [1] 38400 bps
  [2] 57600 bps
  [3] 115200 bps
  [4] 9600 bps
番号を入力: 0
インベントリの試行回数（Enterで1）:
```

- 起動後にまず **ROMバージョン**を取得して疎通確認します。
- つづいて **リーダ／ライタ動作モード**を読み取り、  
  **「モードのみ」コマンドモード(0x00)へ変更**（アンチコリジョン／読取り動作／ブザー／送信データ／通信速度は**現状維持**）します。
- **Inventory2** を指定回数実行します。
  - タグが1件以上：**ピー(0x00)** を鳴らす  
  - タグ0件／エラー：**ピッピッピ(0x01)** を鳴らす  
  - ブザー制御は **CMD: 0x42 / Data: [応答要求(0x01), 音種]** を使用

---

## よくあるエラーと対処

- **「シリアルポートをオープンできません」**  
  - ポート番号の誤り／占有中／権限不足を確認  
  - デバイスマネージャで COM ポート番号を再確認
- **NACK（不明なエラー）**  
  - コマンド種別・データ長・SUM をログで確認  
  - ブザーは **0x42**, 動作モード書き込みは **0x4E**（詳細 0x00=RAM）であること
- **タイムアウト**  
  - ケーブル・電源・距離・タグの向き／枚数を確認  
  - ボーレートは 19200 bps が既定（変更した場合は両端一致が必要）

---

## 免責・注意

- 本プログラムはサンプルです。ご利用は自己責任でお願いします。
- 仕様の詳細は **通信プロトコル説明書** を参照してください（TR3 シリーズ向け）。
