#define NOMINMAX
#include <windows.h>

/*
 * Copyright (c) 2014 Craig Lilley <cralilley@gmail.com>
 * This software is made available under the terms of the MIT licence.
 * A copy of the licence can be obtained from:
 * http://opensource.org/licenses/MIT
 */

#include "serial/serial.h"

#include <cstring>
#include <array>

#include <devguid.h>
#include <initguid.h>
#include <setupapi.h>
#include <tchar.h>

static const size_t STRING_MAX_LENGTH = 256;

static std::string parse_device_registry_property(HDEVINFO device_info_set, SP_DEVINFO_DATA device_info_data, DWORD Property)
{
    std::array<char, STRING_MAX_LENGTH> buffer;
    std::fill(std::begin(buffer), std::end(buffer), '\0');

    DWORD length;

    BOOL success = SetupDiGetDeviceRegistryPropertyA(
        device_info_set,
        &device_info_data,
        Property,
        NULL,
        (PBYTE)buffer.data(),
        (DWORD)buffer.size(),
        &length);

    if (success == TRUE && length > 0) {
        return std::string(buffer.data(), strnlen(buffer.data(), buffer.size()));
    } else {
        return std::string();
    }
}

namespace serial {

std::vector<serial::PortInfo> serial::list_ports()
{
    std::vector<PortInfo> devices_found;

    HDEVINFO device_info_set = SetupDiGetClassDevsA((const GUID*)&GUID_DEVCLASS_PORTS, NULL, NULL, DIGCF_PRESENT);

    SP_DEVINFO_DATA device_info_data;
    device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);

    unsigned int device_info_set_index = 0;

    while (SetupDiEnumDeviceInfo(device_info_set, device_info_set_index, &device_info_data)) {
        device_info_set_index++;

        // retrieve port name
        HKEY hkey = SetupDiOpenDevRegKey(device_info_set, &device_info_data, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);

        std::array<char, STRING_MAX_LENGTH> buffer;
        std::fill(std::begin(buffer), std::end(buffer), '\0');

        DWORD length;

        LONG return_code = RegQueryValueExA(hkey, _T("PortName"), NULL, NULL, (LPBYTE)buffer.data(), &length);
        RegCloseKey(hkey);

        if (return_code != EXIT_SUCCESS) {
            continue;
        }

        std::string port_name = (length > 0) ? std::string(buffer.data(), strnlen(buffer.data(), buffer.size()))
                                             : std::string();

        // ignore parallel ports
        if (port_name.find("LPT") != std::string::npos) {
            continue;
        }

        PortInfo port_entry;
        port_entry.port = port_name;
        port_entry.description = parse_device_registry_property(device_info_set, device_info_data, SPDRP_FRIENDLYNAME);
        port_entry.hardware_id = parse_device_registry_property(device_info_set, device_info_data, SPDRP_HARDWAREID);

        devices_found.emplace_back(port_entry);
    }

    SetupDiDestroyDeviceInfoList(device_info_set);

    return devices_found;
}

} // namespace serial
