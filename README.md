# FAT32 Sorter

FAT32 Sorter rewrites FAT32 directory tables so files and folders appear in
alphabetical order on devices that read directory entries in on-disk order.

The original project was a Visual Studio 2010 Windows application. This fork is
now a CMake-based command-line project that can be opened directly in Visual
Studio Code and built on macOS, Linux, and Windows.

Background article from the original author:
http://www.codeproject.com/Articles/95721/FAT-32-Sorter

## Important Safety Notes

This tool edits FAT32 metadata directly. Use it only on FAT32 filesystems you can
restore from backup.

Rules before sorting:

* Back up the device or disk image first.
* Unmount the FAT32 filesystem before running the sorter.
* Do not run it against a mounted filesystem.
* Prefer testing with a FAT32 image file before using a physical device.
* Run with administrator/root permissions when targeting a raw device.

The program creates a directory-table backup before sorting. The backup is a
timestamped `.dat` file in the working directory. Recovery is available from the
interactive menu by renaming that backup to `dirs.dat` and choosing the recover
option.

## Requirements

Install these tools:

* Visual Studio Code
* CMake 3.16 or newer
* A C++ compiler

Supported compiler setups:

* macOS: Xcode Command Line Tools
* Linux: GCC or Clang
* Windows: MSVC Build Tools or Visual Studio

For VS Code debugging, install the Microsoft C/C++ extension.

## Building

From VS Code:

1. Open this repository folder.
2. Run `Terminal > Run Build Task`.
3. Choose `CMake: build` if prompted.

From a terminal:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

The executable is created at:

```text
build/FAT32Sorter
```

On Windows generators that use configuration subdirectories, it is commonly:

```text
build\Debug\FAT32Sorter.exe
```

## Running

Interactive mode:

```sh
./build/FAT32Sorter /path/to/fat32.img
```

Immediate sort mode:

```sh
./build/FAT32Sorter /path/to/fat32.img sort
```

### macOS

Use a FAT32 image file:

```sh
./build/FAT32Sorter /path/to/fat32.img sort
```

Use a raw FAT32 partition:

```sh
diskutil list
diskutil unmount /dev/disk2s1
sudo ./build/FAT32Sorter /dev/disk2s1 sort
```

### Linux

Use a FAT32 image file:

```sh
./build/FAT32Sorter /path/to/fat32.img sort
```

Use a raw FAT32 partition:

```sh
lsblk
sudo umount /dev/sdb1
sudo ./build/FAT32Sorter /dev/sdb1 sort
```

### Windows

Build with CMake using an MSVC generator, then run from an elevated terminal:

```bat
build\Debug\FAT32Sorter.exe F sort
```

The app also accepts native Windows volume paths:

```bat
build\Debug\FAT32Sorter.exe \\.\F: sort
```

On Windows the program asks the OS to dismount and lock the selected volume
before writing.

## VS Code Tasks

The repository includes `.vscode/tasks.json` and `.vscode/launch.json`.

Available tasks:

* `CMake: configure`
* `CMake: build`
* `Run FAT32Sorter`

Available debug launch configurations:

* `Debug FAT32Sorter (macOS)`
* `Debug FAT32Sorter (Linux)`
* `Debug FAT32Sorter (Windows)`

Each run/debug configuration prompts for the FAT32 device path, image path, or
Windows drive letter.

## Project Layout

```text
CMakeLists.txt              CMake build definition
.vscode/                    VS Code build and debug tasks
FAT32Sorter/FAT32Sorter.cpp Command-line entry point
FAT32Sorter/CVolumeAccess.* Raw volume and image-file access
FAT32Sorter/CFileSystem.*   FAT32 directory-table workflow
FAT32Sorter/CFolderEntry.*  Directory entry loading, sorting, dumping
FAT32Sorter/CEntry.*        FAT short-name and long-name handling
FAT32Sorter/General.*       FAT32 structures and helpers
```

Legacy Visual Studio solution/project files were removed. CMake is now the
single supported build system.

## Current Validation

The current macOS build has been verified with:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/FAT32Sorter "" sort
```

The empty-volume smoke test exits cleanly with a "device is not ready" message.
Before relying on a physical device workflow, validate against a FAT32 image
created specifically for testing.

## Credits

Original author: Udi Cohen (udinic@gmail.com)

## License

Copyright 2012 Udi Cohen

Licensed under the Apache License, Version 2.0. You may obtain a copy of the
License at:

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.
