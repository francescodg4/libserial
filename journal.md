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
