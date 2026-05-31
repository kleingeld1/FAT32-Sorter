#pragma once

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <windows.h>
#include <crtdbg.h>
#else
#include <cerrno>
#include <cstdint>
typedef std::uint8_t BYTE;
typedef std::uint16_t WORD;
typedef std::uint32_t DWORD;
typedef wchar_t WCHAR;
typedef BYTE byte;

inline unsigned long GetLastError()
{
	return static_cast<unsigned long>(errno);
}

#define _CRTDBG_ALLOC_MEM_DF 0
#define _CRTDBG_LEAK_CHECK_DF 0
#define _CrtSetDbgFlag(flags) ((void)0)
#endif

#include <algorithm>
#include <cerrno>
#include <clocale>
#include <codecvt>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cwchar>
#include <fstream>
#include <iostream>
#include <locale>
#include <string>
#include <vector>

#if defined(_WIN32)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>

#ifdef _DEBUG
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif
#endif

using namespace std;
