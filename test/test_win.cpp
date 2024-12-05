#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <setupapi.h>

#include <chrono>

#include <catch2/catch_all.hpp>
#include <fakeit.hpp>

#include "serial/serial.h"

using namespace fakeit;

TEST_CASE("serial::Timeout::simpleTimeout creates a serial::Timeout with appropriate initialization", "[serial]")
{
    auto timeout = std::chrono::milliseconds(500);

    serial::Timeout serial_timeout = serial::Timeout::simpleTimeout(static_cast<uint32_t>(timeout.count()));

    REQUIRE(serial_timeout.inter_byte_timeout == serial::Timeout::max());
    REQUIRE(serial_timeout.read_timeout_constant == static_cast<uint32_t>(timeout.count()));
    REQUIRE(serial_timeout.read_timeout_multiplier == 0);
    REQUIRE(serial_timeout.write_timeout_constant == static_cast<uint32_t>(timeout.count()));
    REQUIRE(serial_timeout.write_timeout_multiplier == 0);
}

struct WindowsApi {
    virtual BOOL SetupDiEnumDeviceInfo(HDEVINFO DeviceInfoSet, DWORD MemberIndex, PSP_DEVINFO_DATA DeviceInfoData) = 0;
    virtual HDEVINFO SetupDiGetClassDevsA(const GUID* ClassGuid, PCSTR Enumerator, HWND hwndParent, DWORD Flags) = 0;
    virtual HKEY SetupDiOpenDevRegKey(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, DWORD Scope, DWORD HwProfile, DWORD KeyType, REGSAM samDesired) = 0;
    virtual LSTATUS RegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) = 0;
    virtual LSTATUS RegCloseKey(HKEY hKey) = 0;
    virtual BOOL SetupDiGetDeviceRegistryPropertyA(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, DWORD Property, PDWORD PropertyRegDataType, PBYTE PropertyBuffer, DWORD PropertyBufferSize, PDWORD RequiredSize) = 0;
    virtual BOOL SetupDiDestroyDeviceInfoList(HDEVINFO DeviceInfoSet) = 0;
    virtual HANDLE WINAPI CreateFileW(_In_ LPCWSTR lpFileName, _In_ DWORD dwDesiredAccess, _In_ DWORD dwShareMode, _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes, _In_ DWORD dwCreationDisposition, _In_ DWORD dwFlagsAndAttributes, _In_opt_ HANDLE hTemplateFile) = 0;
    virtual BOOL WINAPI GetCommState(_In_ HANDLE hFile, _Out_ LPDCB lpDCB) = 0;
    virtual BOOL WINAPI SetCommState(_In_ HANDLE hFile, _In_ LPDCB lpDCB) = 0;
    virtual BOOL WINAPI SetCommTimeouts(_In_ HANDLE hFile, _In_ LPCOMMTIMEOUTS lpCommTimeouts) = 0;
    virtual BOOL WINAPI CloseHandle(_In_ _Post_ptr_invalid_ HANDLE hObject);
    virtual BOOL WINAPI WriteFile(_In_ HANDLE hFile, _In_reads_bytes_opt_(nNumberOfBytesToWrite) LPCVOID lpBuffer, _In_ DWORD nNumberOfBytesToWrite, _Out_opt_ LPDWORD lpNumberOfBytesWritten, _Inout_opt_ LPOVERLAPPED lpOverlapped);
    virtual _Must_inspect_result_ BOOL WINAPI ReadFile(_In_ HANDLE hFile, _Out_writes_bytes_to_opt_(nNumberOfBytesToRead, *lpNumberOfBytesRead) __out_data_source(FILE) LPVOID lpBuffer, _In_ DWORD nNumberOfBytesToRead, _Out_opt_ LPDWORD lpNumberOfBytesRead, _Inout_opt_ LPOVERLAPPED lpOverlapped);
};

Mock<WindowsApi> g_mock;

static void initialize(Mock<WindowsApi>& mock)
{
    Fake(
        Method(mock, SetupDiEnumDeviceInfo),
        Method(mock, SetupDiGetClassDevsA),
        Method(mock, SetupDiOpenDevRegKey),
        Method(mock, RegQueryValueExA),
        Method(mock, RegCloseKey),
        Method(mock, SetupDiGetDeviceRegistryPropertyA),
        Method(mock, SetupDiDestroyDeviceInfoList),
        Method(mock, CreateFileW),
        Method(mock, GetCommState),
        Method(mock, SetCommState),
        Method(mock, SetCommTimeouts),
        Method(mock, CloseHandle),
        Method(mock, WriteFile),
        Method(mock, ReadFile));

    When(Method(g_mock, CreateFileW)).AlwaysDo([](_In_ LPCWSTR lpFileName, _In_ DWORD dwDesiredAccess, _In_ DWORD dwShareMode, _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes, _In_ DWORD dwCreationDisposition, _In_ DWORD dwFlagsAndAttributes, _In_opt_ HANDLE hTemplateFile) {
        return (HANDLE)1;
    });

    When(Method(g_mock, GetCommState)).AlwaysDo([](_In_ HANDLE hFile, _Out_ LPDCB lpDCB) {
        return TRUE;
    });

    When(Method(g_mock, SetCommState)).AlwaysDo([](_In_ HANDLE hFile, _In_ LPDCB lpDCB) {
        return TRUE;
    });

    When(Method(g_mock, SetCommTimeouts)).AlwaysDo([](_In_ HANDLE hFile, _In_ LPCOMMTIMEOUTS lpCommTimeouts) {
        return TRUE;
    });

    When(Method(g_mock, SetCommTimeouts)).AlwaysDo([](_In_ HANDLE hFile, _In_ LPCOMMTIMEOUTS lpCommTimeouts) {
        return TRUE;
    });

    When(Method(g_mock, CloseHandle)).AlwaysDo([](_In_ _Post_ptr_invalid_ HANDLE hObject) {
        return TRUE;
    });
}

BOOL SetupDiEnumDeviceInfo(
    HDEVINFO DeviceInfoSet,
    DWORD MemberIndex,
    PSP_DEVINFO_DATA DeviceInfoData)
{
    return g_mock().SetupDiEnumDeviceInfo(DeviceInfoSet, MemberIndex, DeviceInfoData);
}

HDEVINFO SetupDiGetClassDevsA(const GUID* ClassGuid, PCSTR Enumerator, HWND hwndParent, DWORD Flags)
{
    return g_mock().SetupDiGetClassDevsA(ClassGuid, Enumerator, hwndParent, Flags);
}

BOOL SetupDiDestroyDeviceInfoList(HDEVINFO DeviceInfoSet) { return g_mock().SetupDiDestroyDeviceInfoList(DeviceInfoSet); }

HKEY SetupDiOpenDevRegKey(
    HDEVINFO DeviceInfoSet,
    PSP_DEVINFO_DATA DeviceInfoData,
    DWORD Scope,
    DWORD HwProfile,
    DWORD KeyType,
    REGSAM samDesired)
{
    return g_mock().SetupDiOpenDevRegKey(DeviceInfoSet, DeviceInfoData, Scope, HwProfile, KeyType, samDesired);
}

LSTATUS RegQueryValueExA(
    HKEY hKey,
    LPCSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData)
{
    return g_mock().RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

LSTATUS RegCloseKey(HKEY hKey)
{
    return g_mock().RegCloseKey(hKey);
}

BOOL SetupDiGetDeviceRegistryPropertyA(
    HDEVINFO DeviceInfoSet,
    PSP_DEVINFO_DATA DeviceInfoData,
    DWORD Property,
    PDWORD PropertyRegDataType,
    PBYTE PropertyBuffer,
    DWORD PropertyBufferSize,
    PDWORD RequiredSize)
{
    return g_mock().SetupDiGetDeviceRegistryPropertyA(DeviceInfoSet, DeviceInfoData, Property, PropertyRegDataType, PropertyBuffer, PropertyBufferSize, RequiredSize);
}

HANDLE WINAPI CreateFileW(
    _In_ LPCWSTR lpFileName,
    _In_ DWORD dwDesiredAccess,
    _In_ DWORD dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_ DWORD dwCreationDisposition,
    _In_ DWORD dwFlagsAndAttributes,
    _In_opt_ HANDLE hTemplateFile)
{
    return g_mock().CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

WINBASEAPI BOOL WINAPI GetCommState(_In_ HANDLE hFile, _Out_ LPDCB lpDCB)
{
    return g_mock().GetCommState(hFile, lpDCB);
}

WINBASEAPI BOOL WINAPI SetCommState(_In_ HANDLE hFile, _In_ LPDCB lpDCB)
{
    return g_mock().SetCommState(hFile, lpDCB);
}

WINBASEAPI BOOL WINAPI SetCommTimeouts(_In_ HANDLE hFile, _In_ LPCOMMTIMEOUTS lpCommTimeouts)
{
    return g_mock().SetCommTimeouts(hFile, lpCommTimeouts);
}

WINBASEAPI BOOL WINAPI CloseHandle(_In_ _Post_ptr_invalid_ HANDLE hObject)
{
    return g_mock().CloseHandle(hObject);
}

WINBASEAPI BOOL WINAPI WriteFile(_In_ HANDLE hFile, _In_reads_bytes_opt_(nNumberOfBytesToWrite) LPCVOID lpBuffer, _In_ DWORD nNumberOfBytesToWrite, _Out_opt_ LPDWORD lpNumberOfBytesWritten, _Inout_opt_ LPOVERLAPPED lpOverlapped)
{
    return g_mock().WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

WINBASEAPI _Must_inspect_result_ BOOL WINAPI ReadFile(
    _In_ HANDLE hFile,
    _Out_writes_bytes_to_opt_(nNumberOfBytesToRead, *lpNumberOfBytesRead) __out_data_source(FILE) LPVOID lpBuffer,
    _In_ DWORD nNumberOfBytesToRead,
    _Out_opt_ LPDWORD lpNumberOfBytesRead,
    _Inout_opt_ LPOVERLAPPED lpOverlapped)
{
    return g_mock().ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}

TEST_CASE("serial::list_ports enumerates port devicesa and retrieves device info", "[serial]")
{
    g_mock.Reset();

    initialize(g_mock);

    When(Method(g_mock, SetupDiEnumDeviceInfo)).AlwaysDo([](HDEVINFO, DWORD MemberIndex, PSP_DEVINFO_DATA) {
        return (MemberIndex < 5) ? TRUE : FALSE;
    });

    When(Method(g_mock, RegQueryValueExA))
        .AlwaysDo([index = 0](HKEY, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) mutable {
            snprintf((char*)lpData, 256, "Port-%d", index);
            *lpcbData = static_cast<DWORD>(strnlen((char*)lpData, 256)) + 1;
            index++;
            return ERROR_SUCCESS;
        });

    auto match = [](DWORD property) {
        return [property](HDEVINFO, PSP_DEVINFO_DATA, DWORD Property, PDWORD, PBYTE, DWORD, PDWORD) { return Property == property; };
    };

    When((Method(g_mock, SetupDiGetDeviceRegistryPropertyA)).Matching(match(SPDRP_FRIENDLYNAME)))
        .AlwaysDo([index = 0](HDEVINFO, PSP_DEVINFO_DATA, DWORD Property, PDWORD, PBYTE PropertyBuffer, DWORD PropertyBufferSize, PDWORD RequiredSize) mutable {
            snprintf((char*)PropertyBuffer, PropertyBufferSize, "FriendlyName-%d", index++);
            *RequiredSize = static_cast<DWORD>(strnlen((char*)PropertyBuffer, PropertyBufferSize)) + 1;
            return TRUE;
        });

    When((Method(g_mock, SetupDiGetDeviceRegistryPropertyA)).Matching(match(SPDRP_HARDWAREID)))
        .AlwaysDo([index = 0](HDEVINFO, PSP_DEVINFO_DATA, DWORD Property, PDWORD, PBYTE PropertyBuffer, DWORD PropertyBufferSize, PDWORD RequiredSize) mutable {
            snprintf((char*)PropertyBuffer, PropertyBufferSize, "HardwareID-%d", index++);
            *RequiredSize = static_cast<DWORD>(strnlen((char*)PropertyBuffer, PropertyBufferSize)) + 1;
            return TRUE;
        });

    std::vector<serial::PortInfo> ports = serial::list_ports();

    REQUIRE(ports.size() == 5);

    for (int i = 0; i < ports.size(); i++) {
        REQUIRE(ports[i].hardware_id == "HardwareID-" + std::to_string(i));
        REQUIRE(ports[i].description == "FriendlyName-" + std::to_string(i));
        REQUIRE(ports[i].port == "Port-" + std::to_string(i));
    }
}

TEST_CASE("serial::list_ports ignores parallel ports on windows", "[serial]")
{
    g_mock.Reset();

    initialize(g_mock);

    When(Method(g_mock, SetupDiEnumDeviceInfo)).AlwaysDo([](HDEVINFO, DWORD MemberIndex, PSP_DEVINFO_DATA) {
        return (MemberIndex < 5) ? TRUE : FALSE;
    });

    When(Method(g_mock, RegQueryValueExA)).AlwaysDo([index = 0](HKEY, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) mutable {
        snprintf((char*)lpData, 256, "LPT-%d", index);
        *lpcbData = static_cast<DWORD>(strnlen((char*)lpData, 256)) + 1;
        index++;

        return ERROR_SUCCESS;
    });

    std::vector<serial::PortInfo> devices = serial::list_ports();

    REQUIRE(devices.empty());
}

TEST_CASE("serial::Serial constructor", "[serial]")
{
    g_mock.Reset();

    initialize(g_mock);

    When(Method(g_mock, CreateFileW)).AlwaysDo([](_In_ LPCWSTR lpFileName, _In_ DWORD dwDesiredAccess, _In_ DWORD dwShareMode, _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes, _In_ DWORD dwCreationDisposition, _In_ DWORD dwFlagsAndAttributes, _In_opt_ HANDLE hTemplateFile) {
        return (HANDLE)1;
    });

    When(Method(g_mock, GetCommState)).AlwaysDo([](_In_ HANDLE hFile, _Out_ LPDCB lpDCB) {
        return TRUE;
    });

    When(Method(g_mock, SetCommState)).AlwaysDo([](_In_ HANDLE hFile, _In_ LPDCB lpDCB) {
        return TRUE;
    });

    When(Method(g_mock, SetCommTimeouts)).AlwaysDo([](_In_ HANDLE hFile, _In_ LPCOMMTIMEOUTS lpCommTimeouts) {
        return TRUE;
    });

    When(Method(g_mock, SetCommTimeouts)).AlwaysDo([](_In_ HANDLE hFile, _In_ LPCOMMTIMEOUTS lpCommTimeouts) {
        return TRUE;
    });

    When(Method(g_mock, CloseHandle)).AlwaysDo([](_In_ _Post_ptr_invalid_ HANDLE hObject) {
        return TRUE;
    });

    REQUIRE_NOTHROW(serial::Serial());
    REQUIRE_NOTHROW(serial::Serial("Port", 115200, serial::Timeout::simpleTimeout(1000)));
}

TEST_CASE("serial::Serial destructor raises exception", "[serial]")
{
    g_mock.Reset();

    initialize(g_mock);

    When(Method(g_mock, CloseHandle)).AlwaysDo([](_In_ _Post_ptr_invalid_ HANDLE hObject) {
        return FALSE;
    });

    REQUIRE_NOTHROW(serial::Serial("Port", 115200, serial::Timeout::simpleTimeout(1000)));
}

TEST_CASE("serial::Serial::write", "[serial]")
{
    g_mock.Reset();

    initialize(g_mock);

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    When(Method(g_mock, WriteFile))
        .AlwaysDo([&](_In_ HANDLE hFile, _In_reads_bytes_opt_(nNumberOfBytesToWrite) LPCVOID lpBuffer, _In_ DWORD nNumberOfBytesToWrite, _Out_opt_ LPDWORD lpNumberOfBytesWritten, _Inout_opt_ LPOVERLAPPED lpOverlapped) {
            *lpNumberOfBytesWritten = nNumberOfBytesToWrite;
            memcpy(buffer, lpBuffer, std::min((size_t)nNumberOfBytesToWrite, sizeof(buffer)));
            return TRUE;
        });

    SECTION("write(const uint8_t*, size_t)")
    {
        serial::Serial serial("Port", 115200, serial::Timeout::simpleTimeout(1000));

        uint8_t message[] = { 0xaa, 0xbb, 0xcc };

        size_t bytes_written = serial.write(message, sizeof(message));

        Verify(Method(g_mock, WriteFile)).Exactly(1);
        REQUIRE(memcmp(buffer, message, bytes_written) == 0);
    }

    SECTION("write(const std::vector<uint8_t>&)")
    {
        serial::Serial serial("Port", 115200, serial::Timeout::simpleTimeout(1000));

        std::vector<uint8_t> message = { 1, 2, 3, 4, 5, 6, 7, 8 };

        size_t bytes_written = serial.write(message);

        REQUIRE(std::vector<uint8_t>(buffer, buffer + bytes_written) == message);
    }

    SECTION("write(std::string)")
    {
        serial::Serial serial("Port", 115200, serial::Timeout::simpleTimeout(1000));

        std::string message = "Hello, world";

        serial.write(message);

        REQUIRE(std::string(buffer) == message);
    }
}

TEST_CASE("serial::Serial::read", "[serial]")
{
    g_mock.Reset();

    initialize(g_mock);

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    std::vector<uint8_t> pattern(1024); // the data pattern read from the fake serial
    srand(42);
    std::generate(std::begin(pattern), std::end(pattern), []() { return uint8_t(rand()); });

    When(Method(g_mock, ReadFile)).AlwaysDo([&](_In_ HANDLE hFile, _Out_writes_bytes_to_opt_(nNumberOfBytesToRead, *lpNumberOfBytesRead) __out_data_source(FILE) LPVOID lpBuffer, _In_ DWORD nNumberOfBytesToRead, _Out_opt_ LPDWORD lpNumberOfBytesRead, _Inout_opt_ LPOVERLAPPED lpOverlapped) {
        memcpy(lpBuffer, pattern.data(), nNumberOfBytesToRead);
        *lpNumberOfBytesRead = nNumberOfBytesToRead;
        return TRUE;
    });

    SECTION("read")
    {
        serial::Serial serial("Port", 115200, serial::Timeout::simpleTimeout(1000));

        size_t size = serial.read((uint8_t*)buffer, sizeof(buffer));

        REQUIRE(size == sizeof(buffer));
        REQUIRE(memcmp(buffer, pattern.data(), pattern.size()) == 0);
    }

    SECTION("read(std::vector&)")
    {
        serial::Serial serial("Port", 115200, serial::Timeout::simpleTimeout(1000));

        std::vector<uint8_t> r_buffer; // read buffer data
        serial.read(r_buffer, 42);

        REQUIRE(r_buffer.size() == 42);
        REQUIRE(std::equal(std::begin(r_buffer), std::end(r_buffer), std::begin(pattern)));
    }

    SECTION("read(std::string&)")
    {
        serial::Serial serial("Port", 115200, serial::Timeout::simpleTimeout(1000));

        std::string r_buffer;
        serial.read(r_buffer, 42);

        REQUIRE(r_buffer.size() == 42);
        REQUIRE(std::equal(std::begin(r_buffer), std::end(r_buffer), std::begin(pattern), [](char a, uint8_t b) { return a == (char)(b); }));
    }

    SECTION("read(1)")
    {
        serial::Serial serial("Port", 115200, serial::Timeout::simpleTimeout(1000));

        std::string r_buffer = serial.read(1);

        REQUIRE(r_buffer.size() == 1);
    }

    SECTION("read(size_t)")
    {
        serial::Serial serial("Port", 115200, serial::Timeout::simpleTimeout(1000));

        std::string r_buffer = serial.read(53);

        REQUIRE(r_buffer.size() == 53);
        REQUIRE(std::equal(std::begin(r_buffer), std::end(r_buffer), std::begin(pattern), [](char a, uint8_t b) { return a == (char)(b); }));
    }

    SECTION("read less")
    {
        serial::Serial serial("Port", 115200, serial::Timeout::simpleTimeout(1000));

        size_t expected_bytes_to_read = size_t(0.1 * sizeof(buffer));

        size_t actual_bytes_read = serial.read((uint8_t*)buffer, expected_bytes_to_read);

        REQUIRE(actual_bytes_read == expected_bytes_to_read);
        REQUIRE(memcmp(buffer, pattern.data(), actual_bytes_read) == 0);
    }
}
