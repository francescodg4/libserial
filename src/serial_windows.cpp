#include "serial/serial.h"

#include <cstring>
#include <devguid.h>
#include <initguid.h>
#include <setupapi.h>
#include <tchar.h>

static std::string prefix_port_if_needed(const std::string& input)
{
    static const std::string windows_com_port_prefix = "\\\\.\\";
    if (input.compare(windows_com_port_prefix) != 0) {
        return windows_com_port_prefix + input;
    }
    return input;
}

// Convert a wide Unicode string to an UTF8 string
static std::string utf8_encode(const std::wstring& wstr)
{
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

namespace serial {

Serial::Serial(const std::string& port,
    uint32_t baudrate,
    serial::Timeout timeout,
    bytesize_t bytesize,
    parity_t parity,
    stopbits_t stopbits,
    flowcontrol_t flowcontrol)
    : port_(port.begin(), port.end())
    , fd_(INVALID_HANDLE_VALUE)
    , is_open_(false)
    , baudrate_(baudrate)
    , parity_(parity)
    , bytesize_(bytesize)
    , stopbits_(stopbits)
    , flowcontrol_(flowcontrol)
{
    if (port_.empty() == false) {
        open();
    }
}

Serial::~Serial()
{
    try {
        close();
    }
    catch (...) {
    }
}

void Serial::open()
{
    if (port_.empty()) {
        throw std::invalid_argument("Empty port is invalid.");
    }

    if (is_open_ == true) {
        throw SerialException("Serial port already open.");
    }

    // See: https://github.com/wjwwood/serial/issues/84
    std::string port_with_prefix = prefix_port_if_needed(port_);

    fd_ = CreateFileA(port_with_prefix.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        0);

    if (fd_ == INVALID_HANDLE_VALUE) {
        DWORD create_file_err = GetLastError();
        std::stringstream ss;
        switch (create_file_err) {
        case ERROR_FILE_NOT_FOUND:
            // Use this->getPort to convert to a std::string
            ss << "Specified port, " << getPort() << ", does not exist.";
            THROW(IOException, ss.str().c_str());
        default:
            ss << "Unknown error opening the serial port: " << create_file_err;
            THROW(IOException, ss.str().c_str());
        }
    }

    reconfigurePort();

    is_open_ = true;
}

bool Serial::isOpen() const { return is_open_; }

const std::string& Serial::getPort() const { return port_; }

void Serial::setPort(const std::string& port)
{
    std::unique_lock read_lock(m_read_mutex);
    std::unique_lock write_lock(m_write_mutex);

    bool was_open = isOpen();

    if (was_open) {
        close();
    }

    port_ = port;

    if (was_open) {
        open();
    }
}

void Serial::close()
{
    if (is_open_ == true) {
        if (fd_ != INVALID_HANDLE_VALUE) {
            int ret;
            ret = CloseHandle(fd_);

            if (ret == 0) {
                std::stringstream ss;
                ss << "Error while closing serial port: " << GetLastError();
                THROW(IOException, ss.str().c_str());
            }
            else {
                fd_ = INVALID_HANDLE_VALUE;
            }
        }

        is_open_ = false;
    }
}

size_t Serial::available()
{
    if (!is_open_) {
        return 0;
    }

    COMSTAT cs;

    if (!ClearCommError(fd_, NULL, &cs)) {
        std::stringstream ss;
        ss << "Error while checking status of the serial port: " << GetLastError();
        THROW(IOException, ss.str().c_str());
    }

    return static_cast<size_t>(cs.cbInQue);
}

void Serial::setTimeout(const serial::Timeout& timeout)
{
    timeout_ = timeout;

    if (is_open_) {
        reconfigurePort();
    }
}

serial::Timeout Serial::getTimeout() const { return timeout_; }

bool Serial::waitReadable(uint32_t /*timeout*/)
{
    THROW(IOException, "waitReadable is not implemented on Windows.");
    return false;
}

void Serial::waitByteTimes(size_t /*count*/)
{
    THROW(IOException, "waitByteTimes is not implemented on Windows.");
}

void Serial::setBaudrate(uint32_t baudrate)
{
    baudrate_ = baudrate;

    if (is_open_) {
        reconfigurePort();
    }
}

uint32_t Serial::getBaudrate() const { return uint32_t(baudrate_); }

void Serial::setBytesize(bytesize_t bytesize)
{
    bytesize_ = bytesize;

    if (is_open_) {
        reconfigurePort();
    }
}

bytesize_t Serial::getBytesize() const { return bytesize_; }

void Serial::setParity(parity_t parity)
{
    parity_ = parity;

    if (is_open_) {
        reconfigurePort();
    }
}

parity_t Serial::getParity() const { return parity_; }

void Serial::setStopbits(stopbits_t stopbits)
{
    stopbits_ = stopbits;

    if (is_open_) {
        reconfigurePort();
    }
}

stopbits_t Serial::getStopbits() const
{
    return stopbits_;
}

void Serial::setFlowcontrol(flowcontrol_t flowcontrol)
{
    flowcontrol_ = flowcontrol;

    if (is_open_) {
        reconfigurePort();
    }
}

flowcontrol_t Serial::getFlowcontrol() const { return flowcontrol_; }

bool Serial::waitForChange()
{
    if (!is_open_) {
        throw PortNotOpenedException("Serial::waitForChange");
    }

    DWORD dwCommEvent;

    if (!SetCommMask(fd_, EV_CTS | EV_DSR | EV_RING | EV_RLSD)) {
        // Error setting communications mask
        return false;
    }

    if (!WaitCommEvent(fd_, &dwCommEvent, NULL)) {
        // An error occurred waiting for the event.
        return false;
    }
    else {
        // Event has occurred.
        return true;
    }
}

bool Serial::getCTS()
{
    if (!is_open_) {
        throw PortNotOpenedException("Serial::getCTS");
    }

    DWORD dwModemStatus;
    if (!GetCommModemStatus(fd_, &dwModemStatus)) {
        THROW(IOException, "Error getting the status of the CTS line.");
    }

    return (MS_CTS_ON & dwModemStatus) != 0;
}

bool Serial::getDSR()
{
    if (!is_open_) {
        throw PortNotOpenedException("Serial::getDSR");
    }

    DWORD dwModemStatus;
    if (!GetCommModemStatus(fd_, &dwModemStatus)) {
        THROW(IOException, "Error getting the status of the DSR line.");
    }

    return (MS_DSR_ON & dwModemStatus) != 0;
}

bool Serial::getRI()
{
    if (!is_open_) {
        throw PortNotOpenedException("Serial::getRI");
    }

    DWORD dwModemStatus;
    if (!GetCommModemStatus(fd_, &dwModemStatus)) {
        THROW(IOException, "Error getting the status of the RI line.");
    }

    return (MS_RING_ON & dwModemStatus) != 0;
}

bool Serial::getCD()
{
    if (!is_open_) {
        throw PortNotOpenedException("Serial::getCD");
    }

    DWORD dwModemStatus;
    if (!GetCommModemStatus(fd_, &dwModemStatus)) {
        THROW(IOException, "Error getting the status of the CD line.");
    }

    return (MS_RLSD_ON & dwModemStatus) != 0;
}

void Serial::sendBreak(int /*duration*/)
{
    THROW(IOException, "sendBreak is not supported on Windows.");
}

void Serial::setBreak(bool level)
{
    if (!is_open_) {
        throw PortNotOpenedException("Serial::setBreak");
    }
    if (level) {
        EscapeCommFunction(fd_, SETBREAK);
    }
    else {
        EscapeCommFunction(fd_, CLRBREAK);
    }
}

void Serial::setRTS(bool level)
{
    if (!is_open_) {
        throw PortNotOpenedException("Serial::setRTS");
    }
    if (level) {
        EscapeCommFunction(fd_, SETRTS);
    }
    else {
        EscapeCommFunction(fd_, CLRRTS);
    }
}

void Serial::setDTR(bool level)
{
    if (!is_open_) {
        throw PortNotOpenedException("Serial::setDTR");
    }
    if (level) {
        EscapeCommFunction(fd_, SETDTR);
    }
    else {
        EscapeCommFunction(fd_, CLRDTR);
    }
}

void Serial::flush()
{
    std::unique_lock read_lock(m_read_mutex);
    std::unique_lock write_lock(m_write_mutex);

    if (!is_open_) {
        throw PortNotOpenedException("Serial::flush");
    }

    FlushFileBuffers(fd_);
}

void Serial::flushInput()
{
    std::unique_lock read_lock(m_read_mutex);

    if (!is_open_) {
        throw PortNotOpenedException("Serial::flushInput");
    }

    PurgeComm(fd_, PURGE_RXCLEAR);
}

void Serial::flushOutput()
{
    std::unique_lock write_lock(m_write_mutex);

    if (!is_open_) {
        throw PortNotOpenedException("Serial::flushOutput");
    }

    PurgeComm(fd_, PURGE_TXCLEAR);
}

size_t Serial::read(uint8_t* buffer, size_t size)
{
    std::unique_lock read_lock(m_read_mutex);

    if (!is_open_) {
        throw PortNotOpenedException("Serial::read");
    }

    DWORD bytes_read;

    if (!ReadFile(fd_, buffer, static_cast<DWORD>(size), &bytes_read, NULL)) {
        std::stringstream ss;
        ss << "Error while reading from the serial port: " << GetLastError();
        THROW(IOException, ss.str().c_str());
    }

    return (size_t)(bytes_read);
}

size_t Serial::write(const uint8_t* data, size_t size)
{
    std::unique_lock write_lock(m_write_mutex);

    if (is_open_ == false) {
        throw PortNotOpenedException("Serial::write");
    }

    DWORD bytes_written;

    if (!WriteFile(fd_, data, static_cast<DWORD>(size), &bytes_written, NULL)) {
        std::stringstream ss;
        ss << "Error while writing to the serial port: " << GetLastError();
        THROW(IOException, ss.str().c_str());
    }

    return (size_t)(bytes_written);
}

void Serial::reconfigurePort()
{
    if (fd_ == INVALID_HANDLE_VALUE) {
        // Can only operate on a valid file descriptor
        THROW(IOException, "Invalid file descriptor, is the serial port open?");
    }

    DCB dcbSerialParams = { 0 };

    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(fd_, &dcbSerialParams)) {
        // error getting state
        THROW(IOException, "Error getting the serial port state.");
    }

    // setup baud rate
    switch (baudrate_) {
#ifdef CBR_0
    case 0:
        dcbSerialParams.BaudRate = CBR_0;
        break;
#endif
#ifdef CBR_50
    case 50:
        dcbSerialParams.BaudRate = CBR_50;
        break;
#endif
#ifdef CBR_75
    case 75:
        dcbSerialParams.BaudRate = CBR_75;
        break;
#endif
#ifdef CBR_110
    case 110:
        dcbSerialParams.BaudRate = CBR_110;
        break;
#endif
#ifdef CBR_134
    case 134:
        dcbSerialParams.BaudRate = CBR_134;
        break;
#endif
#ifdef CBR_150
    case 150:
        dcbSerialParams.BaudRate = CBR_150;
        break;
#endif
#ifdef CBR_200
    case 200:
        dcbSerialParams.BaudRate = CBR_200;
        break;
#endif
#ifdef CBR_300
    case 300:
        dcbSerialParams.BaudRate = CBR_300;
        break;
#endif
#ifdef CBR_600
    case 600:
        dcbSerialParams.BaudRate = CBR_600;
        break;
#endif
#ifdef CBR_1200
    case 1200:
        dcbSerialParams.BaudRate = CBR_1200;
        break;
#endif
#ifdef CBR_1800
    case 1800:
        dcbSerialParams.BaudRate = CBR_1800;
        break;
#endif
#ifdef CBR_2400
    case 2400:
        dcbSerialParams.BaudRate = CBR_2400;
        break;
#endif
#ifdef CBR_4800
    case 4800:
        dcbSerialParams.BaudRate = CBR_4800;
        break;
#endif
#ifdef CBR_7200
    case 7200:
        dcbSerialParams.BaudRate = CBR_7200;
        break;
#endif
#ifdef CBR_9600
    case 9600:
        dcbSerialParams.BaudRate = CBR_9600;
        break;
#endif
#ifdef CBR_14400
    case 14400:
        dcbSerialParams.BaudRate = CBR_14400;
        break;
#endif
#ifdef CBR_19200
    case 19200:
        dcbSerialParams.BaudRate = CBR_19200;
        break;
#endif
#ifdef CBR_28800
    case 28800:
        dcbSerialParams.BaudRate = CBR_28800;
        break;
#endif
#ifdef CBR_57600
    case 57600:
        dcbSerialParams.BaudRate = CBR_57600;
        break;
#endif
#ifdef CBR_76800
    case 76800:
        dcbSerialParams.BaudRate = CBR_76800;
        break;
#endif
#ifdef CBR_38400
    case 38400:
        dcbSerialParams.BaudRate = CBR_38400;
        break;
#endif
#ifdef CBR_115200
    case 115200:
        dcbSerialParams.BaudRate = CBR_115200;
        break;
#endif
#ifdef CBR_128000
    case 128000:
        dcbSerialParams.BaudRate = CBR_128000;
        break;
#endif
#ifdef CBR_153600
    case 153600:
        dcbSerialParams.BaudRate = CBR_153600;
        break;
#endif
#ifdef CBR_230400
    case 230400:
        dcbSerialParams.BaudRate = CBR_230400;
        break;
#endif
#ifdef CBR_256000
    case 256000:
        dcbSerialParams.BaudRate = CBR_256000;
        break;
#endif
#ifdef CBR_460800
    case 460800:
        dcbSerialParams.BaudRate = CBR_460800;
        break;
#endif
#ifdef CBR_921600
    case 921600:
        dcbSerialParams.BaudRate = CBR_921600;
        break;
#endif
    default:
        // Try to blindly assign it
        dcbSerialParams.BaudRate = baudrate_;
    }

    // setup char len
    if (bytesize_ == eightbits)
        dcbSerialParams.ByteSize = 8;
    else if (bytesize_ == sevenbits)
        dcbSerialParams.ByteSize = 7;
    else if (bytesize_ == sixbits)
        dcbSerialParams.ByteSize = 6;
    else if (bytesize_ == fivebits)
        dcbSerialParams.ByteSize = 5;
    else
        throw std::invalid_argument("invalid char len");

    // setup stopbits
    if (stopbits_ == stopbits_one)
        dcbSerialParams.StopBits = ONESTOPBIT;
    else if (stopbits_ == stopbits_one_point_five)
        dcbSerialParams.StopBits = ONE5STOPBITS;
    else if (stopbits_ == stopbits_two)
        dcbSerialParams.StopBits = TWOSTOPBITS;
    else
        throw std::invalid_argument("invalid stop bit");

    // setup parity
    if (parity_ == parity_none) {
        dcbSerialParams.Parity = NOPARITY;
    }
    else if (parity_ == parity_even) {
        dcbSerialParams.Parity = EVENPARITY;
    }
    else if (parity_ == parity_odd) {
        dcbSerialParams.Parity = ODDPARITY;
    }
    else if (parity_ == parity_mark) {
        dcbSerialParams.Parity = MARKPARITY;
    }
    else if (parity_ == parity_space) {
        dcbSerialParams.Parity = SPACEPARITY;
    }
    else {
        throw std::invalid_argument("invalid parity");
    }

    // setup flowcontrol
    if (flowcontrol_ == flowcontrol_none) {
        dcbSerialParams.fOutxCtsFlow = false;
        dcbSerialParams.fRtsControl = RTS_CONTROL_DISABLE;
        dcbSerialParams.fOutX = false;
        dcbSerialParams.fInX = false;
    }
    if (flowcontrol_ == flowcontrol_software) {
        dcbSerialParams.fOutxCtsFlow = false;
        dcbSerialParams.fRtsControl = RTS_CONTROL_DISABLE;
        dcbSerialParams.fOutX = true;
        dcbSerialParams.fInX = true;
    }
    if (flowcontrol_ == flowcontrol_hardware) {
        dcbSerialParams.fOutxCtsFlow = true;
        dcbSerialParams.fRtsControl = RTS_CONTROL_HANDSHAKE;
        dcbSerialParams.fOutX = false;
        dcbSerialParams.fInX = false;
    }

    // activate settings
    if (!SetCommState(fd_, &dcbSerialParams)) {
        CloseHandle(fd_);
        THROW(IOException, "Error setting serial port settings.");
    }

    // Setup timeouts
    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = timeout_.inter_byte_timeout;
    timeouts.ReadTotalTimeoutConstant = timeout_.read_timeout_constant;
    timeouts.ReadTotalTimeoutMultiplier = timeout_.read_timeout_multiplier;
    timeouts.WriteTotalTimeoutConstant = timeout_.write_timeout_constant;
    timeouts.WriteTotalTimeoutMultiplier = timeout_.write_timeout_multiplier;

    if (!SetCommTimeouts(fd_, &timeouts)) {
        THROW(IOException, "Error setting timeouts.");
    }
}

std::vector<PortInfo> serial::list_ports()
{
    static const DWORD kPortNameMaxLength = 256;
    static const DWORD kFriendlyNameMaxLength = 256;
    static const DWORD kHardwareIdMaxLength = 256;

    std::vector<PortInfo> devices_found;

    HDEVINFO device_info_set = SetupDiGetClassDevs(
        (const GUID*)&GUID_DEVCLASS_PORTS,
        NULL,
        NULL,
        DIGCF_PRESENT);

    unsigned int device_info_set_index = 0;

    SP_DEVINFO_DATA device_info_data;
    device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);

    while (SetupDiEnumDeviceInfo(device_info_set, device_info_set_index, &device_info_data)) {
        device_info_set_index++;

        // Get port name
        HKEY hkey = SetupDiOpenDevRegKey(
            device_info_set,
            &device_info_data,
            DICS_FLAG_GLOBAL,
            0,
            DIREG_DEV,
            KEY_READ);

        TCHAR port_name[kPortNameMaxLength];
        DWORD port_name_length = kPortNameMaxLength;

        LONG return_code = RegQueryValueEx(
            hkey,
            _T("PortName"),
            NULL,
            NULL,
            (LPBYTE)port_name,
            &port_name_length);

        RegCloseKey(hkey);

        if (return_code != EXIT_SUCCESS) {
            continue;
        }

        if (port_name_length > 0 && port_name_length <= kPortNameMaxLength) {
            port_name[port_name_length - 1] = '\0';
        }
        else {
            port_name[0] = '\0';
        }

        // Ignore parallel ports
        if (_tcsstr(port_name, _T("LPT")) != NULL) {
            continue;
        }

        // Get port friendly name
        TCHAR friendly_name[kFriendlyNameMaxLength];
        DWORD friendly_name_actual_length = 0;

        BOOL got_friendly_name = SetupDiGetDeviceRegistryProperty(
            device_info_set,
            &device_info_data,
            SPDRP_FRIENDLYNAME,
            NULL,
            (PBYTE)friendly_name,
            kFriendlyNameMaxLength,
            &friendly_name_actual_length);

        if (got_friendly_name == TRUE && friendly_name_actual_length > 0) {
            friendly_name[friendly_name_actual_length - 1] = '\0';
        }
        else {
            friendly_name[0] = '\0';
        }

        // Get hardware ID
        TCHAR hardware_id[kHardwareIdMaxLength];
        DWORD hardware_id_actual_length = 0;

        BOOL got_hardware_id = SetupDiGetDeviceRegistryProperty(
            device_info_set,
            &device_info_data,
            SPDRP_HARDWAREID,
            NULL,
            (PBYTE)hardware_id,
            kHardwareIdMaxLength,
            &hardware_id_actual_length);

        if (got_hardware_id == TRUE && hardware_id_actual_length > 0) {
            hardware_id[hardware_id_actual_length - 1] = '\0';
        }
        else {
            hardware_id[0] = '\0';
        }

#ifdef UNICODE
        std::string portName = utf8_encode(port_name);
        std::string friendlyName = utf8_encode(friendly_name);
        std::string hardwareId = utf8_encode(hardware_id);
#else
        std::string portName = port_name;
        std::string friendlyName = friendly_name;
        std::string hardwareId = hardware_id;
#endif

        PortInfo port_entry;
        port_entry.port = portName;
        port_entry.description = friendlyName;
        port_entry.hardware_id = hardwareId;

        devices_found.push_back(port_entry);
    }

    SetupDiDestroyDeviceInfoList(device_info_set);

    return devices_found;
}

} // namespace serial

// #include <sstream>

// #include "serial/impl/win.h"

// using serial::bytesize_t;
// using serial::flowcontrol_t;
// using serial::IOException;
// using serial::parity_t;
// using serial::PortNotOpenedException;
// using serial::Serial;
// using serial::SerialException;
// using serial::stopbits_t;
// using serial::Timeout;
// using std::invalid_argument;
// using std::string;
// using std::stringstream;
// using std::wstring;

// void Serial::SerialImpl::open()
// {
//     if (port_.empty()) {
//         throw invalid_argument("Empty port is invalid.");
//     }
//     if (is_open_ == true) {
//         throw SerialException("Serial port already open.");
//     }

//     // See: https://github.com/wjwwood/serial/issues/84
//     wstring port_with_prefix = _prefix_port_if_needed(port_);
//     LPCWSTR lp_port = port_with_prefix.c_str();
//     fd_ = CreateFileW(lp_port,
//         GENERIC_READ | GENERIC_WRITE,
//         0,
//         0,
//         OPEN_EXISTING,
//         FILE_ATTRIBUTE_NORMAL,
//         0);

//     if (fd_ == INVALID_HANDLE_VALUE) {
//         DWORD create_file_err = GetLastError();
//         stringstream ss;
//         switch (create_file_err) {
//         case ERROR_FILE_NOT_FOUND:
//             // Use this->getPort to convert to a std::string
//             ss << "Specified port, " << this->getPort() << ", does not exist.";
//             THROW(IOException, ss.str().c_str());
//         default:
//             ss << "Unknown error opening the serial port: " << create_file_err;
//             THROW(IOException, ss.str().c_str());
//         }
//     }

//     reconfigurePort();
//     is_open_ = true;
// }

// size_t
// Serial::SerialImpl::read(uint8_t* buf, size_t size)
// {
//     if (!is_open_) {
//         throw PortNotOpenedException("Serial::read");
//     }
//     DWORD bytes_read;
//     if (!ReadFile(fd_, buf, static_cast<DWORD>(size), &bytes_read, NULL)) {
//         stringstream ss;
//         ss << "Error while reading from the serial port: " << GetLastError();
//         THROW(IOException, ss.str().c_str());
//     }
//     return (size_t)(bytes_read);
// }

// size_t
// Serial::SerialImpl::write(const uint8_t* data, size_t length)
// {
//     if (is_open_ == false) {
//         throw PortNotOpenedException("Serial::write");
//     }
//     DWORD bytes_written;
//     if (!WriteFile(fd_, data, static_cast<DWORD>(length), &bytes_written, NULL)) {
//         stringstream ss;
//         ss << "Error while writing to the serial port: " << GetLastError();
//         THROW(IOException, ss.str().c_str());
//     }
//     return (size_t)(bytes_written);
// }

// void Serial::SerialImpl::setBaudrate(unsigned long baudrate)
// {
//     baudrate_ = baudrate;
//     if (is_open_) {
//         reconfigurePort();
//     }
// }

// unsigned long
// Serial::SerialImpl::getBaudrate() const
// {
//     return baudrate_;
// }

// void Serial::SerialImpl::setBytesize(serial::bytesize_t bytesize)
// {
//     bytesize_ = bytesize;
//     if (is_open_) {
//         reconfigurePort();
//     }
// }

// serial::bytesize_t
// Serial::SerialImpl::getBytesize() const
// {
//     return bytesize_;
// }

// void Serial::SerialImpl::setParity(serial::parity_t parity)
// {
//     parity_ = parity;
//     if (is_open_) {
//         reconfigurePort();
//     }
// }

// serial::parity_t
// Serial::SerialImpl::getParity() const
// {
//     return parity_;
// }

// void Serial::SerialImpl::setStopbits(serial::stopbits_t stopbits)
// {
//     stopbits_ = stopbits;
//     if (is_open_) {
//         reconfigurePort();
//     }
// }

// serial::stopbits_t
// Serial::SerialImpl::getStopbits() const
// {
//     return stopbits_;
// }

// void Serial::SerialImpl::setFlowcontrol(serial::flowcontrol_t flowcontrol)
// {
//     flowcontrol_ = flowcontrol;
//     if (is_open_) {
//         reconfigurePort();
//     }
// }

// serial::flowcontrol_t
// Serial::SerialImpl::getFlowcontrol() const
// {
//     return flowcontrol_;
// }

// void Serial::SerialImpl::readLock()
// {
//     if (WaitForSingleObject(read_mutex, INFINITE) != WAIT_OBJECT_0) {
//         THROW(IOException, "Error claiming read mutex.");
//     }
// }

// void Serial::SerialImpl::readUnlock()
// {
//     if (!ReleaseMutex(read_mutex)) {
//         THROW(IOException, "Error releasing read mutex.");
//     }
// }

// void Serial::SerialImpl::writeLock()
// {
//     if (WaitForSingleObject(write_mutex, INFINITE) != WAIT_OBJECT_0) {
//         THROW(IOException, "Error claiming write mutex.");
//     }
// }

// void Serial::SerialImpl::writeUnlock()
// {
//     if (!ReleaseMutex(write_mutex)) {
//         THROW(IOException, "Error releasing write mutex.");
//     }
// }
