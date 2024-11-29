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
};

Mock<WindowsApi> g_mock;

WINSETUPAPI BOOL SetupDiEnumDeviceInfo(
    HDEVINFO DeviceInfoSet,
    DWORD MemberIndex,
    PSP_DEVINFO_DATA DeviceInfoData)
{
    return g_mock().SetupDiEnumDeviceInfo(DeviceInfoSet, MemberIndex, DeviceInfoData);
}

WINSETUPAPI HDEVINFO SetupDiGetClassDevsA(const GUID* ClassGuid, PCSTR Enumerator, HWND hwndParent, DWORD Flags)
{
    return g_mock().SetupDiGetClassDevsA(ClassGuid, Enumerator, hwndParent, Flags);
}

WINSETUPAPI BOOL SetupDiDestroyDeviceInfoList(HDEVINFO DeviceInfoSet) { return g_mock().SetupDiDestroyDeviceInfoList(DeviceInfoSet); }

WINSETUPAPI HKEY SetupDiOpenDevRegKey(
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

WINSETUPAPI BOOL SetupDiGetDeviceRegistryPropertyA(
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

TEST_CASE("serial::list_ports enumerates valid port devices. Ignores parallel ports", "[serial]")
{
    g_mock.Reset();

    Fake(
        Method(g_mock, SetupDiEnumDeviceInfo),
        Method(g_mock, SetupDiGetClassDevsA),
        Method(g_mock, SetupDiOpenDevRegKey),
        Method(g_mock, RegQueryValueExA),
        Method(g_mock, RegCloseKey),
        Method(g_mock, SetupDiGetDeviceRegistryPropertyA),
        Method(g_mock, SetupDiDestroyDeviceInfoList));

    When(Method(g_mock, SetupDiEnumDeviceInfo)).AlwaysDo([](HDEVINFO, DWORD MemberIndex, PSP_DEVINFO_DATA) {
        return (MemberIndex < 5) ? TRUE : FALSE;
    });

    When(Method(g_mock, RegQueryValueExA)).AlwaysDo([index = 0](HKEY, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) mutable {
        if (index < 3) {
            snprintf((char*)lpData, 256, "Port-%d", index);
        } else {
            // Ignore parallel ports
            snprintf((char*)lpData, 256, "LPT-%d", index);
        }

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

    REQUIRE(ports.size() == 3);
    for (int i = 0; i < ports.size(); i++) {
        REQUIRE(ports[i].hardware_id == "HardwareID-" + std::to_string(i));
        REQUIRE(ports[i].description == "FriendlyName-" + std::to_string(i));
        REQUIRE(ports[i].port == "Port-" + std::to_string(i));
    }
}
