# Terminal File Explorer

This is a simple terminal-based file explorer written in **C** that allows users to navigate directories, list files, and perform basic file system operations directly from the command line.

## Features
- **Directory Navigation**: Move up and down through directories.
- **List Contents**: Display files and folders in the current directory.
- **Change Directory**: Navigate to a specific directory using the `cd` command.
- **File and Directory Details**: View file types and sizes.
- **Cross-Platform Support**: Compatible with both Windows and Unix-like systems.

## Commands
- **q**: Quit the application.
- **u**: Move up one directory.
- **cd <path>**: Change the current directory to `<path>`.
- **Enter (on a folder/file name)**: Open the selected folder/file.

## Compilation and Execution

### On Linux/MacOS
```bash
gcc -o file_explorer main.c
./file_explorer
```

### On Windows
```cmd
gcc -o file_explorer.exe main.c
file_explorer.exe
```

## Dependencies
- **GCC Compiler**
- Standard C Libraries (stdio.h, stdlib.h, string.h, dirent.h, sys/stat.h, unistd.h)

## Known Issues
- Symbolic links are not supported on Windows.
- Some system-specific behavior may vary.


## License
This project is licensed under the **MIT License**.

## Contribution
Feel free to fork this repository, submit issues, or create pull requests.

## Author
[Maxim Vlas]   

---


