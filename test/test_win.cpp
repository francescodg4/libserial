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
        Method(mock, CreateFileW));
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

BOOL WINAPI GetCommState(_In_ HANDLE hFile, _Out_ LPDCB lpDCB)
{
    return g_mock().GetCommState(hFile, lpDCB);
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
        return FALSE;
    });

    //     BOOL WINAPI GetCommState(_In_ HANDLE hFile, _Out_ LPDCB lpDCB)
    // {
    //     return g_mock().GetCommState(hFile, lpDCB);
    // }

    REQUIRE_NOTHROW(serial::Serial());
    REQUIRE_NOTHROW(serial::Serial("Port", 115200, serial::Timeout::simpleTimeout(1000)));
}
