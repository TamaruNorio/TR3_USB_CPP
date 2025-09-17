// 1) COM/ボーレート選択（日本語UI）
// 2) ROMバージョン表示
// 3) 動作モードの読み取り → 「モードのみコマンドモードへ」書き込み → 再読取り
// 4) Inventory2 実行（★試行回数を入力して繰り返し実行）

#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <chrono>

#include "../include/serial_port.hpp"
#include "../include/tr3_protocol.hpp"

//----------------------------------------------
static std::vector<std::string> enum_com_ports() {
    std::vector<std::string> ports;
    char deviceName[16], target[512];
    for (int i = 1; i <= 256; ++i) {
        std::snprintf(deviceName, sizeof(deviceName), "COM%d", i);
        if (::QueryDosDeviceA(deviceName, target, sizeof(target))) ports.emplace_back(deviceName);
    }
    return ports;
}
static int ask_number(const std::string& prompt, int minVal, int maxVal, int defVal) {
    while (true) {
        std::cout << prompt;
        std::string s; std::getline(std::cin, s);
        if (s.empty()) return defVal;
        try {
            int v = std::stoi(s);
            if (v < minVal || v > maxVal) { std::cout<<"範囲外です。\n"; continue; }
            return v;
        } catch (...) { std::cout<<"数字で入力してください。\n"; }
    }
}
//----------------------------------------------
int main(int, char**) {
    // === COM選択 ===
    auto coms = enum_com_ports();
    if (coms.empty()) { std::cerr << "COMポートが見つかりません。\n"; return 1; }
    std::cout << "=== 利用可能なCOMポート ===\n";
    for (size_t i=0;i<coms.size();++i) std::cout<<"  ["<<i<<"] "<<coms[i]<<"\n";
    int idx = ask_number("使用する番号（Enterで0）: ", 0, int(coms.size()-1), 0);
    std::string com = coms[size_t(idx)];

    // === ボーレート選択（既定19200） ===
    const std::vector<uint32_t> baudList = {19200,38400,57600,115200,9600};
    std::cout << "=== ボーレート（Enterで19200） ===\n";
    for (size_t i=0;i<baudList.size();++i) std::cout<<"  ["<<i<<"] "<<baudList[i]<<" bps\n";
    int bidx = ask_number("番号を入力: ",0,int(baudList.size()-1),0);
    uint32_t baud = baudList[size_t(bidx)];

    tr3::SerialPort sp(com, baud);
    if (!sp.open()) { std::cerr<<"オープン失敗: "<<sp.last_error()<<"\n"; return 2; }

    // === ROM（疎通） ===
    if (tr3::read_rom_version(sp, 600).empty()) { std::cerr<<"ROM取得失敗\n"; return 3; }

    // === 動作モード 読み取り → 「モードのみコマンドモードへ」設定 → 再読取り ===
    tr3::ReaderModeRaw raw{}; tr3::ReaderModePretty pretty{};
    if (!tr3::read_reader_mode(sp, raw, pretty, 600))
        std::cerr<<"動作モードの取得に失敗しました。\n";

    if (!tr3::write_reader_mode_to_command(sp, raw, 600))
        std::cerr<<"動作モードの設定に失敗しました。\n";

    tr3::read_reader_mode(sp, raw, pretty, 600);

    // === ★インベントリの試行回数を入力 ===
    int tries = ask_number("インベントリの試行回数（Enterで1）: ", 1, 1000000, 1);

    // === インベントリを指定回数繰り返し実行 ===
    for (int t = 1; t <= tries; ++t) {
        if (tries > 1) {
            std::cout << "\n--- インベントリ試行 " << t << " / " << tries << " ---\n";
        }
        auto r = tr3::run_inventory2(sp, 2000);
        if (!r.error_message.empty()) {
            std::cout << "NACK/エラー: " << r.error_message << "\n";
            // 見つからなかった扱いにして「ピッピッピ」を鳴らす（応答あり）
            tr3::buzzer(sp, /*response_type=*/0x01, /*sound_type=*/0x01, 600);
        } else {
            std::cout << "取得UID数: " << r.items.size() << "\n";
            for (size_t i=0;i<r.items.size();++i) {
                std::cout << "  ["<<i<<"] ";
                for (auto b: r.items[i].uid)
                    std::cout<<std::uppercase<<std::hex<<std::setw(2)<<std::setfill('0')<<int(b)<<" ";
                std::cout<<std::dec<<"\n";
            }
            // 取得件数に応じてブザー
            if (!r.items.empty()) {
                // タグが1件以上 → ピー
                tr3::buzzer(sp, /*response_type=*/0x01, /*sound_type=*/0x00, 600);
            } else {
                // タグ0件 → ピッピッピ
                tr3::buzzer(sp, /*response_type=*/0x01, /*sound_type=*/0x01, 600);
            }
        }
        if (t < tries) ::Sleep(100);
    }

    return 0;
}
