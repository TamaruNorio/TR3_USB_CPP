#pragma once
// TR3（HF 13.56MHz）通信プロトコル上位層
#include <string>
#include <vector>
#include <cstdint>
#include "serial_port.hpp"

namespace tr3 {

// ───────────────────────────────────
// 動作モード（読み取り結果）
// ───────────────────────────────────
struct ReaderModeRaw {
    // 0x4F 読み取りACKのデータ部（9バイト想定）を丸ごと保持
    std::vector<uint8_t> bytes;  // [0]=モード, [1]=予約, [2]=フラグ(bit2..bit5), [3]=速度(bit6..7), [4..8]=予約
};
struct ReaderModePretty {
    std::string mode;           // モード名
    std::string anticollision;  // アンチコリジョン有無
    std::string read_behavior;  // 読み取り動作（1回／連続）
    std::string buzzer;         // ブザー
    std::string tx_data;        // 送信データ
    std::string baud;           // ボーレート
};

// ───────────────────────────────────
// インベントリ結果
// ───────────────────────────────────
struct InventoryItem {
    std::vector<uint8_t> uid;  // MSB→LSB順へ並べ替え済み
    uint8_t dsfid = 0x00;
};
struct InventoryResult {
    std::vector<InventoryItem> items; // 取得UID配列
    int expected_count = 0;           // ACKで通知されたUID数
    std::string error_message;        // NACKなど（空=正常）
};

// ───────────────────────────────────
// 共通ユーティリティ
// stop_on_ack=true: ACK/NACK受信で戻る（従来動作）
// stop_on_ack=false: タイムアウトまで全フレーム収集（Inventory2等）
// ───────────────────────────────────
std::vector<uint8_t> communicate(SerialPort& sp,
                                 const std::vector<uint8_t>& command,
                                 uint32_t timeout_ms,
                                 bool stop_on_ack = true);

bool verify_frame(const std::vector<uint8_t>& frame);

// ───────────────────────────────────
// ROMバージョン
// ───────────────────────────────────
std::string read_rom_version(SerialPort& sp, uint32_t timeout_ms = 600);

// ───────────────────────────────────
// 動作モード 読み取り / 書き込み
// ───────────────────────────────────
bool read_reader_mode(SerialPort& sp, ReaderModeRaw& raw, ReaderModePretty& pretty, uint32_t timeout_ms = 600);

// ★要件対応：モードのみ「コマンドモード(0x00)」に変更し、他設定は維持して書き込む
bool write_reader_mode_to_command(SerialPort& sp,
                                  const ReaderModeRaw& current,
                                  uint32_t timeout_ms = 600);

// ───────────────────────────────────
// ブザー制御（BUZ: 0x57）
// response_type: 0x00=応答なし, 0x01=応答あり（本プログラムでは 0x01 を使用）
// sound_type   : 0x00=ピー, 0x01=ピッピッピ（他バリエーションは機種仕様に準拠）
// 戻り値       : 送信・ACK受信に成功したら true
// ───────────────────────────────────
bool buzzer(SerialPort& sp, uint8_t response_type, uint8_t sound_type, uint32_t timeout_ms = 600);

// ───────────────────────────────────
// Inventory2（アンチコリジョン設定により順序が変わってもOK）
// ───────────────────────────────────
InventoryResult run_inventory2(SerialPort& sp, uint32_t timeout_ms = 1500);

// ───────────────────────────────────
// NACK
// ───────────────────────────────────
std::string parse_nack_message(const std::vector<uint8_t>& nack_frame);

} // namespace tr3

