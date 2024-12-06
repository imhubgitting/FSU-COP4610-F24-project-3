# FAT32 File System Utility

This project implements a user-space, shell-like utility capable of interpreting a FAT32 file system image. The utility allows users to manipulate the given file system image using basic commands without compromising its integrity.

## Group Members
- **Gavin Antonacci**: gaa21b@fsu.edu

## Division of Labor

### Part 1: Mounting the Image
- **Responsibilities**: Implement the functionality to mount the FAT32 image file, read its structure, and initialize the shell interface.
- **Assigned to**: Gavin

### Part 2: Navigation
- **Responsibilities**: Implement commands to navigate the file system, such as `cd` and `ls`.
- **Assigned to**: Gavin

### Part 3: Create
- **Responsibilities**: Implement commands to create directories and files, such as `mkdir` and `creat`.
- **Assigned to**: Gavin

### Part 4: Read
- **Responsibilities**: Implement commands to open, close, list, seek, and read files, such as `open`, `close`, `lsof`, `lseek`, and `read`.
- **Assigned to**: Gavin

### Part 5: Update
- **Responsibilities**: Implement commands to write to files and rename files or directories, such as `write` and `rename`.
- **Assigned to**: Gavin

### Part 6: Delete
- **Responsibilities**: Implement commands to delete files and directories, such as `rm` and `rmdir`.
- **Assigned to**: Gavin

### Extra Credit
- **Responsibilities**: Implement extra credit features such as recursive deletion and hex dump of directories or files.
- **Assigned to**: Gavin

## File Listing
filesys/
│
├── src/
│   ├── main.c               # Main program file
│   ├── fat32.c              # FAT32 related functions implementation
│   ├── commands.c           # Command handling functions
│   ├── lexer.c              # Lexer functions for parsing user input
│   └── ...                  # Other source files as needed
│
├── include/
│   ├── fat32.h              # Header file for FAT32 structure and functions
│   ├── commands.h           # Header file for command functions
│   ├── lexer.h              # Header file for lexer functions
│   └── ...                  # Other header files as needed
│
├── bin/
│   └── filesys              # Compiled executable
│
├── README.md                # Project documentation
├── Makefile                 # Makefile for building the project
└── ...                      # Any additional files or directories





## How to Compile & Execute

### Requirements
- **Compiler**: `gcc` for C, `rustc` for Rust.
- **Dependencies**: Standard C library, Standard Rust library.

### Compilation
For a C example:
```bash
make

This will build the executable in the bin/ directory.
./bin/filesys <FAT32_IMAGE>
Replace <FAT32_IMAGE> with the path to the FAT32 file system image you want to interact with.