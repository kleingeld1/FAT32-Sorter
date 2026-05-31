cmake_minimum_required(VERSION 3.16)

project(FAT32Sorter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Keep this list to the actively used sorter sources.
add_executable(FAT32Sorter
    FAT32Sorter/CEntry.cpp
    FAT32Sorter/CFileEntry.cpp
    FAT32Sorter/CFileSystem.cpp
    FAT32Sorter/CFolderEntry.cpp
    FAT32Sorter/CRootFolder.cpp
    FAT32Sorter/CSpecialEntry.cpp
    FAT32Sorter/CVolumeAccess.cpp
    FAT32Sorter/FAT32Sorter.cpp
    FAT32Sorter/General.cpp
)

target_include_directories(FAT32Sorter PRIVATE FAT32Sorter)

if(MSVC)
    target_compile_definitions(FAT32Sorter PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
    target_compile_options(FAT32Sorter PRIVATE /W3)
else()
    target_compile_options(FAT32Sorter PRIVATE -Wall -Wextra)
endif()
