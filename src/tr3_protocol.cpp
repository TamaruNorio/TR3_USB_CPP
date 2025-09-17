// TR3 HF: フレーム生成／厳密受信／モード・Inventory2 実装
#include <cstdio>
#include <cstdint>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <cctype>

#include "../include/tr3_protocol.hpp"
#include "../include/serial_port.hpp"

using namespace std::chrono;

//===============================
// 定数
//===============================
static constexpr uint8_t STX = 0x02;
static constexpr uint8_t ETX = 0x03;
static constexpr uint8_t CR  = 0x0D;

static constexpr uint8_t ADDR_DEFAULT = 0x00;

static constexpr uint8_t CMD_ACK      = 0x30;
static constexpr uint8_t CMD_NACK     = 0x31;

// HF: ROM / ReaderMode / Inventory2
static constexpr uint8_t CMD_ROM_REQ  = 0x4F; // len=1, data=0x90
static constexpr uint8_t DETAIL_ROM   = 0x90;

static constexpr uint8_t CMD_MODE_RD  = 0x4F; // len=1, data=0x00
static constexpr uint8_t DETAIL_MODE_R= 0x00;

static constexpr uint8_t CMD_MODE_WR  = 0x4E; // len=4, data=[mode,reserved,flags,speedbits]

static constexpr uint8_t CMD_INV2     = 0x78; // len=3, data=F0 40 01
static constexpr uint8_t DETAIL_INV2_F0=0xF0; // Inventory2
static constexpr uint8_t RSP_UID      = 0x49; // DSFID+UIDレスポンス

static constexpr size_t  IDX_STX    = 0;
static constexpr size_t  IDX_ADDR   = 1;
static constexpr size_t  IDX_CMD    = 2;
static constexpr size_t  IDX_LEN    = 3;
static constexpr size_t  HEADER_LEN = 4;
static constexpr size_t  FOOTER_LEN = 3;

// ブザー制御は「書き込み(0x4E)」の「詳細コマンド(0x42)」
static constexpr uint8_t CMD_BUZZER = 0x42;  // ブザーの制御


//===============================
// ログユーティリティ
//===============================
static std::string now_timestamp() {
    const auto tp = system_clock::now();
    const auto ms = duration_cast<milliseconds>(tp.time_since_epoch()) % 1000;
    const std::time_t t = system_clock::to_time_t(tp);
    std::tm tm{}; localtime_s(&tm, &t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%m/%d %H:%M:%S") << "." << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}
static void log_line(const char* tag, const std::string& payload) {
    std::printf("%s  [%s]  %s\n", now_timestamp().c_str(), tag, payload.c_str());
    std::fflush(stdout);
}
static std::string to_hex_string(const std::vector<uint8_t>& buf) {
    std::ostringstream oss;
    for (size_t i = 0; i < buf.size(); ++i) {
        if (i) oss << ' ';
        oss << std::uppercase << std::hex
            << std::setw(2) << std::setfill('0') << static_cast<int>(buf[i]);
    }
    return oss.str();
}

//===============================
// フレーム生成/検証
//===============================
static uint8_t calc_sum_until(const std::vector<uint8_t>& bytes, size_t until) {
    uint32_t s = 0; for (size_t i = 0; i < until; ++i) s += bytes[i];
    return static_cast<uint8_t>(s & 0xFF);
}
static std::vector<uint8_t> make_frame(uint8_t addr, uint8_t cmd, const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> f;
    f.reserve(HEADER_LEN + payload.size() + FOOTER_LEN);
    f.push_back(STX);
    f.push_back(addr);
    f.push_back(cmd);
    f.push_back(static_cast<uint8_t>(payload.size()));
    f.insert(f.end(), payload.begin(), payload.end());
    f.push_back(ETX);
    f.push_back(calc_sum_until(f, f.size()));
    f.push_back(CR);
    return f;
}
bool tr3::verify_frame(const std::vector<uint8_t>& f) {
    if (f.size() < HEADER_LEN + FOOTER_LEN) return false;
    const uint8_t len  = f[IDX_LEN];
    const size_t  need = HEADER_LEN + len + FOOTER_LEN;
    if (f.size() != need) return false;
    if (f[need - 1] != CR)  return false;
    if (f[need - 3] != ETX) return false;
    const uint8_t sum_expect = f[need - 2];
    const uint8_t sum_calc   = calc_sum_until(f, need - 2);
    return sum_expect == sum_calc;
}

//===============================
// 送受信（ACKで止める/止めない選択）
//===============================
std::vector<uint8_t> tr3::communicate(SerialPort& sp,
                                      const std::vector<uint8_t>& command,
                                      uint32_t timeout_ms,
                                      bool stop_on_ack)
{
    log_line("send", to_hex_string(command));
    if (!sp.write(command)) { log_line("cmt", "送信エラー"); return {}; }

    std::vector<uint8_t> rxbuf;  rxbuf.reserve(256);
    std::vector<uint8_t> out;
    const auto deadline = steady_clock::now() + milliseconds(timeout_ms);

    while (steady_clock::now() < deadline) {
        uint8_t b = 0;
        if (sp.read_byte(b, std::chrono::milliseconds(10))) rxbuf.push_back(b);

        // STXまで捨てる
        while (!rxbuf.empty() && rxbuf[0] != STX) rxbuf.erase(rxbuf.begin());
        if (rxbuf.size() < HEADER_LEN) continue;

        const uint8_t dlen = rxbuf[IDX_LEN];
        const size_t  need = HEADER_LEN + dlen + FOOTER_LEN;
        if (rxbuf.size() < need) continue;

        std::vector<uint8_t> f(rxbuf.begin(), rxbuf.begin() + need);
        if (!verify_frame(f)) { rxbuf.erase(rxbuf.begin()); continue; }

        log_line("recv", to_hex_string(f));
        out.insert(out.end(), f.begin(), f.end());

        const uint8_t cmd = f[IDX_CMD];
        if (stop_on_ack && (cmd == CMD_ACK || cmd == CMD_NACK)) return out;

        rxbuf.erase(rxbuf.begin(), rxbuf.begin() + need);
    }
    log_line("cmt", "タイムアウト: レスポンスが一定時間内に受信されませんでした。");
    return out;
}

//===============================
// ROM
//===============================
std::string tr3::read_rom_version(SerialPort& sp, uint32_t timeout_ms) {
    log_line("cmt", "/* ROMバージョンの読み取り */");
    auto tx = make_frame(ADDR_DEFAULT, CMD_ROM_REQ, {DETAIL_ROM});
    auto rx = communicate(sp, tx, timeout_ms);
    if (rx.empty()) return {};

    size_t i = 0, last = 0;
    while (i + HEADER_LEN + FOOTER_LEN <= rx.size()) {
        const uint8_t len  = rx[i + IDX_LEN];
        const size_t  need = HEADER_LEN + len + FOOTER_LEN;
        if (i + need > rx.size()) break;
        last = i; i += need;
    }
    std::vector<uint8_t> f(rx.begin() + last, rx.begin() + i);
    if (!verify_frame(f) || f[IDX_CMD] != CMD_ACK || f[HEADER_LEN] != DETAIL_ROM) return {};

    std::string ascii;
    for (size_t k = HEADER_LEN + 1; k < f.size() - FOOTER_LEN; ++k)
        if (std::isprint(f[k])) ascii.push_back(static_cast<char>(f[k]));

    std::string pretty = ascii;
    if (ascii.size() >= 4) {
        pretty = ascii.substr(0,1) + "." + ascii.substr(1,2) + " " + ascii.substr(3,1);
        if (ascii.size() > 4) pretty += ascii.substr(4);
    }
    log_line("cmt", std::string("ROMバージョン : ") + pretty);
    return pretty;
}

//===============================
// NACK（簡易）
//===============================
std::string tr3::parse_nack_message(const std::vector<uint8_t>& f) {
    if (!verify_frame(f) || f[IDX_CMD] != CMD_NACK) return "Invalid NACK";
    uint8_t code = (f.size() > HEADER_LEN + 1) ? f[HEADER_LEN + 1] : 0xFF;
    switch (code) {
        case 0x42: return "SUM_ERROR: SUM不一致";
        case 0x44: return "FORMAT_ERROR: フォーマット/パラメータ不正";
        default:   return "Unknown NACK error";
    }
}

//===============================
// 動作モード 読み取り/表示
//===============================
static tr3::ReaderModePretty pretty_from_raw(const tr3::ReaderModeRaw& raw) {
    tr3::ReaderModePretty p;
    if (raw.bytes.size() >= 4) {
        const uint8_t mode  = raw.bytes[0]; // モードはデータ部先頭
        const uint8_t flags = raw.bytes[2]; // bit2..bit5
        const uint8_t spdb  = raw.bytes[3]; // 通信速度: bit6/bit7

        // モード
        switch (mode) {
            case 0x00: p.mode = "コマンドモード"; break;
            case 0x01: p.mode = "オートスキャンモード"; break;
            case 0x02: p.mode = "トリガーモード"; break;
            case 0x03: p.mode = "ポーリングモード"; break;
            case 0x24: p.mode = "EASモード"; break;
            case 0x50: p.mode = "連続インベントリモード"; break;
            case 0x58: p.mode = "RDLOOPモード"; break;
            case 0x59: p.mode = "RDLOOPモード(実行中)"; break;
            case 0x63: p.mode = "EPCインベントリモード"; break;
            case 0x64: p.mode = "EPCインベントリリードモード"; break;
            default: { std::ostringstream o; o<<"不明 (0x"<<std::uppercase<<std::hex<<std::setw(2)<<std::setfill('0')<<int(mode)<<")"; p.mode = o.str(); }
        }
        // フラグ
        p.anticollision = (flags & (1<<2)) ? "有効"          : "無効";
        p.read_behavior = (flags & (1<<3)) ? "連続読み取り"  : "1回読み取り";
        p.buzzer        = (flags & (1<<4)) ? "鳴らす"        : "鳴らさない";
        p.tx_data       = (flags & (1<<5)) ? "ユーザデータ + UID" : "ユーザデータのみ";

        // 通信速度: bit6/bit7
        switch ((spdb >> 6) & 0x03) {
            case 0b00: p.baud = "19200bps";   break;
            case 0b01: p.baud = "9600bps";    break;
            case 0b10: p.baud = "38400bps";   break;
            case 0b11: p.baud = "115200bps";  break;
        }
    }
    return p;
}

bool tr3::read_reader_mode(SerialPort& sp, ReaderModeRaw& raw, ReaderModePretty& pretty, uint32_t timeout_ms) {
    log_line("cmt", "/* リーダライタ動作モードの読み取り */");
    auto tx = make_frame(ADDR_DEFAULT, CMD_MODE_RD, {DETAIL_MODE_R});
    auto rx = communicate(sp, tx, timeout_ms);
    if (rx.empty()) return false;

    // 末尾（ACK）を抽出
    size_t i = 0, last = 0;
    while (i + HEADER_LEN + FOOTER_LEN <= rx.size()) {
        const uint8_t len  = rx[i + IDX_LEN];
        const size_t  need = HEADER_LEN + len + FOOTER_LEN;
        if (i + need > rx.size()) break;
        last = i; i += need;
    }
    std::vector<uint8_t> f(rx.begin() + last, rx.begin() + i);
    if (!verify_frame(f) || f[IDX_CMD] != CMD_ACK || f[HEADER_LEN] != DETAIL_MODE_R) return false;

    // detail(0x00) の直後から9Bを保持
    raw.bytes.assign(f.begin() + HEADER_LEN + 1, f.end() - FOOTER_LEN);
    pretty = pretty_from_raw(raw);

    log_line("cmt", std::string("リーダライタ動作モード : ") + pretty.mode);
    log_line("cmt", std::string("アンチコリジョン       : ") + pretty.anticollision);
    log_line("cmt", std::string("読み取り動作           : ") + pretty.read_behavior);
    log_line("cmt", std::string("ブザー                 : ") + pretty.buzzer);
    log_line("cmt", std::string("送信データ             : ") + pretty.tx_data);
    log_line("cmt", std::string("通信速度               : ") + pretty.baud);
    return true;
}

// ★モードのみコマンドモードへ変更（他設定は維持）
//   書き込み(4Eh)データ部は 7 バイト：
//   [0]=詳細(00h=RAM/10h=EEPROM), [1]=モード, [2]=予約, [3]=各種設定パラメータ, [4]=予約, [5]=ポーリング上位, [6]=ポーリング下位
bool tr3::write_reader_mode_to_command(SerialPort& sp, const ReaderModeRaw& current, uint32_t timeout_ms) {
    if (current.bytes.size() < 4) { // [0]=モード, [2]=各種設定パラメータ(=flags相当) を使うので最低4バイト必要
        log_line("cmt", "現行モード情報が不足しています（読み取りレスポンスのデータ部が短い）");
        return false;
    }

    // 読み取り結果から「各種設定パラメータ」を抽出
    // 読み取りACKのデータ部は [0]=モード, [1]=予約, [2]=各種設定パラメータ, [3]=速度ビット… の並びでした
    const uint8_t flags = current.bytes[2];

    // ★ログ：何をするか明記
    log_line("cmt", "/* コマンドモードへ設定します （他の設定は現状維持）*/");

    // 書き込み先は RAM（00h）。EEPROMに永続化したい場合は detail を 0x10 にしてください。
    const uint8_t detail   = 0x00; // RAM
    const uint8_t new_mode = 0x00; // コマンドモード
    const uint8_t reserved = 0x00;
    const uint8_t poll_hi  = 0x00;
    const uint8_t poll_lo  = 0x00;

    // 正しい 7 バイトの並びでペイロードを作る
    std::vector<uint8_t> payload = {
        detail,        // 1: 詳細コマンド
        new_mode,      // 2: リーダライタ動作モード（ここだけ上書き）
        reserved,      // 3: 予約
        flags,         // 4: 各種設定パラメータ（読み取り値を維持）
        reserved,      // 5: 予約
        poll_hi,       // 6: ポーリング時間（上位）
        poll_lo        // 7: ポーリング時間（下位）
    };

    // 送信
    auto tx = make_frame(ADDR_DEFAULT, CMD_MODE_WR, payload);
    auto rx = communicate(sp, tx, timeout_ms);
    if (rx.empty()) return false;

    // 末尾（ACK/NACK）判定
    size_t i = 0, last = 0;
    while (i + HEADER_LEN + FOOTER_LEN <= rx.size()) {
        const uint8_t len  = rx[i + IDX_LEN];
        const size_t  need = HEADER_LEN + len + FOOTER_LEN;
        if (i + need > rx.size()) break;
        last = i; i += need;
    }
    std::vector<uint8_t> f(rx.begin() + last, rx.begin() + i);
    if (!verify_frame(f)) return false;
    if (f[IDX_CMD] == CMD_NACK) {
        log_line("cmt", std::string("NACK: ") + parse_nack_message(f));
        return false;
    }
    return (f[IDX_CMD] == CMD_ACK);
}


//===============================
// Inventory2（順序非依存）
//===============================
tr3::InventoryResult tr3::run_inventory2(SerialPort& sp, uint32_t timeout_ms) {
    InventoryResult out;
    log_line("cmt", "/* Inventory2 */");
    auto tx = make_frame(ADDR_DEFAULT, CMD_INV2, {0xF0, 0x40, 0x01});

    // 手動送信 → 逐次フレーム化
    log_line("send", to_hex_string(tx));
    if (!sp.write(tx)) { out.error_message = "送信エラー"; return out; }

    std::vector<uint8_t> buf; buf.reserve(256);
    const auto t_end   = steady_clock::now() + milliseconds(timeout_ms);
    auto       t_quiet = steady_clock::now();
    bool       got_any_uid = false;
    int        expected = -1;

    while (steady_clock::now() < t_end) {
        uint8_t b = 0;
        if (sp.read_byte(b, std::chrono::milliseconds(10))) { t_quiet = steady_clock::now(); buf.push_back(b); }
        while (!buf.empty() && buf[0] != STX) buf.erase(buf.begin());
        if (buf.size() < HEADER_LEN) continue;

        const uint8_t dlen = buf[IDX_LEN];
        const size_t need  = HEADER_LEN + dlen + FOOTER_LEN;
        if (buf.size() < need) continue;

        std::vector<uint8_t> f(buf.begin(), buf.begin()+need);
        if (!verify_frame(f)) { buf.erase(buf.begin()); continue; }

        log_line("recv", to_hex_string(f));
        const uint8_t cmd = f[IDX_CMD];

        if (cmd == CMD_ACK && f[HEADER_LEN] == DETAIL_INV2_F0) {
            if (f.size() >= HEADER_LEN + FOOTER_LEN + 2) {
                expected = f[HEADER_LEN + 1];
                out.expected_count = expected;
                std::ostringstream oss; oss << "UID数 : " << expected; log_line("cmt", oss.str());
            }
        } else if (cmd == RSP_UID && f.size() >= HEADER_LEN + FOOTER_LEN + 9) {
            InventoryItem it;
            it.dsfid = f[HEADER_LEN + 0];
            std::vector<uint8_t> uid_lsb(f.begin()+HEADER_LEN+1, f.begin()+HEADER_LEN+9);
            it.uid.assign(uid_lsb.rbegin(), uid_lsb.rend()); // MSB→LSB
            out.items.push_back(std::move(it));
            got_any_uid = true;

            std::ostringstream d; d<<std::uppercase<<std::hex<<std::setw(2)<<std::setfill('0')<<int(out.items.back().dsfid);
            log_line("cmt", std::string("DSFID : ") + d.str());
            std::ostringstream u; for (auto x: out.items.back().uid) u<<std::uppercase<<std::hex<<std::setw(2)<<std::setfill('0')<<int(x)<<" ";
            log_line("cmt", std::string("UID   : ") + u.str());
        } else if (cmd == CMD_NACK) {
            out.error_message = parse_nack_message(f);
            return out;
        }

        buf.erase(buf.begin(), buf.begin()+need);

        // 終了条件
        if (expected >= 0 && int(out.items.size()) >= expected) break;
        if (got_any_uid) {
            if (duration_cast<milliseconds>(steady_clock::now() - t_quiet).count() > 120) break;
        }
    }

    if (out.items.empty() && out.error_message.empty()) {
        out.error_message = "UIDを取得できませんでした（タイムアウト/対象なし）";
    }
    return out;
}

// ───────────────────────────────────
// ブザー制御（CMD=0x42）
// データ: [response_type, sound_type]
//   response_type: 0x00=応答不要, 0x01=応答あり（本プログラムは 0x01 推奨）
//   sound_type   : 0x00=ピー, 0x01=ピッピッピ, 0x02=ピッピッ, ... 0x08 まで
// ───────────────────────────────────
bool tr3::buzzer(SerialPort& sp, uint8_t response_type, uint8_t sound_type, uint32_t timeout_ms) {
    // 表示用ラベル
    std::string tone = (sound_type == 0x00) ? "ピー" :
                       (sound_type == 0x01) ? "ピッピッピ" : ("type=0x" + [] (uint8_t v){
                           std::ostringstream o; o<<std::uppercase<<std::hex<<std::setw(2)<<std::setfill('0')<<int(v); return o.str();
                       }(sound_type));
    log_line("cmt", std::string("/* ブザー制御: ") + tone + " */");

    // データ部（2バイト）
    std::vector<uint8_t> payload{ response_type, sound_type };

    // フレーム生成（CMD=0x42）→ 送信（ACKで戻る）
    auto tx = make_frame(ADDR_DEFAULT, CMD_BUZZER, payload);
    auto rx = communicate(sp, tx, timeout_ms, /*stop_on_ack=*/true);
    if (rx.empty()) return false;

    // 末尾フレームの検査（ACK/NACK）
    size_t i = 0, last = 0;
    while (i + HEADER_LEN + FOOTER_LEN <= rx.size()) {
        const uint8_t len  = rx[i + IDX_LEN];
        const size_t  need = HEADER_LEN + len + FOOTER_LEN;
        if (i + need > rx.size()) break;
        last = i; i += need;
    }
    std::vector<uint8_t> f(rx.begin() + last, rx.begin() + i);
    if (!verify_frame(f)) return false;
    if (f[IDX_CMD] == CMD_NACK) {
        log_line("cmt", std::string("NACK: ") + parse_nack_message(f));
        return false;
    }
    return (f[IDX_CMD] == CMD_ACK);  // ACK(0x30) データ長 0 の想定
}
