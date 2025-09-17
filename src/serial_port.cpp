#include "../include/serial_port.hpp"
#include <sstream>

namespace tr3 {

static DCB make_dcb(uint32_t baud) {
    DCB dcb{}; dcb.DCBlength = sizeof(DCB);
    dcb.BaudRate = baud;
    dcb.fBinary  = TRUE;
    dcb.fParity  = FALSE;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.ByteSize = 8;
    return dcb;
}

bool SerialPort::open() {
    close();
    std::string path = "\\\\.\\" + port_name_;
    h_ = CreateFileA(path.c_str(), GENERIC_READ|GENERIC_WRITE, 0, nullptr,
                     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h_ == INVALID_HANDLE_VALUE) { last_error_ = "CreateFile 失敗"; return false; }

    DCB dcb = make_dcb(baud_);
    if (!SetCommState(h_, &dcb)) { last_error_ = "SetCommState 失敗"; close(); return false; }

    COMMTIMEOUTS to{};
    to.ReadIntervalTimeout         = 10;
    to.ReadTotalTimeoutConstant    = 10;
    to.ReadTotalTimeoutMultiplier  = 0;
    to.WriteTotalTimeoutConstant   = 200;
    to.WriteTotalTimeoutMultiplier = 0;
    SetCommTimeouts(h_, &to);

    SetupComm(h_, 4096, 4096);
    PurgeComm(h_, PURGE_RXCLEAR | PURGE_TXCLEAR);
    return true;
}

void SerialPort::close() {
    if (h_ != INVALID_HANDLE_VALUE) { CloseHandle(h_); h_ = INVALID_HANDLE_VALUE; }
}

bool SerialPort::write(const std::vector<uint8_t>& data) {
    if (h_ == INVALID_HANDLE_VALUE) return false;
    DWORD w = 0;
    if (!WriteFile(h_, data.data(), static_cast<DWORD>(data.size()), &w, nullptr)) return false;
    return w == data.size();
}

bool SerialPort::read_byte(uint8_t& out, std::chrono::milliseconds per_byte_timeout) {
    if (h_ == INVALID_HANDLE_VALUE) return false;

    COMMTIMEOUTS to{};
    GetCommTimeouts(h_, &to);
    to.ReadIntervalTimeout        = static_cast<DWORD>(per_byte_timeout.count());
    to.ReadTotalTimeoutConstant   = static_cast<DWORD>(per_byte_timeout.count());
    SetCommTimeouts(h_, &to);

    DWORD r = 0;
    if (!ReadFile(h_, &out, 1, &r, nullptr)) return false;
    return (r == 1);
}

} // namespace tr3
