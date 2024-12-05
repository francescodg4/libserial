# TODO

- Update README file
- Add .clang-format, add .gitattributes
	git ls-files --eol
- Compile the example code
- Fix warnings
- Update cmake file

- Run the example file provided

.vscode/settings.json

"cmake.debugConfig": {
        "args": [
            "port", "115200"
        ]
    }

"cmake.debugConfig": {
        "args": [
            "-e"
        ]
    }

- Setup test
	- serial::Timeout
		- Use std::chono::milliseconds for timeout structure
	- serial::list_ports()
	- serial::Serial
	- serial::Serial::read, write, isOpen

- serial::Serial remove pimpl


5-12-2024

Removed the intermediate class SerialImpl for windows.
1. Remove src/serial.cc only from Windows, keep the compilation untouched for Linux
2. Fix the newly generated compilation errors on Windows.

Implemented read and variants

readlines?
lock?
flush?
