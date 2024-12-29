# Terminal File Explorer

This is a simple terminal-based file explorer written in **C** that allows users to navigate directories, list files, and perform basic file system operations directly from the command line.

## Features
- **Directory Navigation**: Move up and down through directories.
- **List Contents**: Display files and folders in the current directory.
- **Change Directory**: Navigate to a specific directory using the `cd` command.
- **File and Directory Details**: View file types and sizes.

## Commands
- **q**: Quit the application.
- **u**: Move up one directory.
- **cd <path>**: Change the current directory to `<path>`.
- **Enter** (on a folder/file name): Open the selected folder/file.
- **filter size <min>-<max>**: Filter files by size.
    - Example: `filter size 10K-2M` (shows files between 10 KB and 2 MB).
- **filter type <f|d>**: Filter by file type.
    - `f`: Files only.
    - `d`: Directories only.
    - Example: `filter type f` (shows files only).
- **search <term>**: Search for files or directories containing the specified term/name.
    - Example: `search image`.
- **clear filter**: Reset all active filters.

## Compilation and Execution

### On Linux/MacOS
```bash
Linux support will be added later
```

### On Windows
```cmd
gcc -o file_explorer.exe main.c
file_explorer.exe
```

## Dependencies
- **GCC Compiler**
- Standard C Libraries (stdio.h, stdlib.h, string.h, dirent.h, sys/stat.h, unistd.h, errno.h, limits.h, ctype.h, time.h, windows.h, shlwapi.h)

## Known Issues
- Linux is not fully supported yet.

## License
This project is licensed under the **MIT License**.

## Contribution
Feel free to fork this repository, submit issues, or create pull requests.

## Author
[Maxim Vlas]   

---


