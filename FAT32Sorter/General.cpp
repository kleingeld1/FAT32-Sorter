#include "stdafx.h"
#include "General.h"

bool isFolderEntry(FATDirEntryUn aEntryToCheck)
{
	return ((aEntryToCheck.ShortEntry.DIR_Attr & ATTR_DIRECTORY) == ATTR_DIRECTORY);
}

bool isLFNEntry(FATDirEntryUn aEntryToCheck)
{
	return ((aEntryToCheck.LongEntry.LDIR_ATT & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME);
}

bool isSpecialEntry(FATDirEntryUn aEntryToCheck)
{
	// Checks that this is a Volume label entry
	if (((aEntryToCheck.LongEntry.LDIR_ATT & ATTR_LONG_NAME_MASK) != ATTR_LONG_NAME) && (aEntryToCheck.LongEntry.LDIR_Ord != 0xE5))
	{
		if ((aEntryToCheck.ShortEntry.DIR_Attr & (ATTR_DIRECTORY | ATTR_VOLUME_ID)) == ATTR_VOLUME_ID)
		{
			return true;
		}
		// If the first byte is the 0x2E (".") than this is probably one of the "." or ".." entries 
		// In the start of each folder
		else if (((aEntryToCheck.ShortEntry.DIR_Attr & (ATTR_DIRECTORY | ATTR_VOLUME_ID)) == ATTR_DIRECTORY) &&
				(aEntryToCheck.ShortEntry.DIR_Name[0] == (char)0x2E))
		{
			return true;
		}
	}

	return false;
}

bool isDeletedEntry(FATDirEntry aEntryToCheck)
{
	// The entry is considered "deleted" if the first byte is 0xE5
	return (aEntryToCheck.DIR_Name[0] == (char)0xE5);
}

std::string wideToUtf8(const WCHAR* aValue)
{
	if (aValue == NULL)
		return std::string();

	try
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t> > converter;
		return converter.to_bytes(aValue);
	}
	catch (...)
	{
		std::string fallback;
		for (const WCHAR* curr = aValue; *curr != L'\0'; ++curr)
		{
			fallback.push_back((*curr >= 0 && *curr < 0x80) ? static_cast<char>(*curr) : '?');
		}
		return fallback;
	}
}

int compareWideCaseInsensitive(const WCHAR* aLeft, const WCHAR* aRight)
{
#if defined(_WIN32)
	return _wcsicmp(aLeft, aRight);
#else
	return wcscasecmp(aLeft, aRight);
#endif
}
