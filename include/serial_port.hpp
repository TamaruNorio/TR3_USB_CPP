#pragma once
// Windows 専用・簡易シリアルポートラッパ
#include <string>
#include <vector>
#include <chrono>
#include <windows.h>

namespace tr3 {

class SerialPort {
public:
    SerialPort(std::string port_name, uint32_t baud)
        : port_name_(std::move(port_name)), baud_(baud) {}
    ~SerialPort() { close(); }

    bool open();
    void close();
    bool write(const std::vector<uint8_t>& data);

    // 1バイト読み取り（個別タイムアウト）
    bool read_byte(uint8_t& out, std::chrono::milliseconds per_byte_timeout);

    std::string last_error() const { return last_error_; }

private:
    std::string port_name_;
    uint32_t    baud_;
    HANDLE      h_ = INVALID_HANDLE_VALUE;
    std::string last_error_;
};

} // namespace tr3
