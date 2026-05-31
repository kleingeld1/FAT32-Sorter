#include "stdafx.h"
#include "CEntry.h"

CEntry::CEntry(FATDirEntry aHexData, LFNEntry* aLFNEntries, WORD aNumLFNEntries)
{
	m_data = aHexData;
	m_numOfEntryElements = aNumLFNEntries;
	m_chainedLFNEntry = aLFNEntries;
}
CEntry::CEntry()
{
	// Incase this it the ROOT DIR - Initialize the data as an empty buffer.
	memset(&m_data,0,sizeof(FATDirEntry));
	m_chainedLFNEntry = NULL;
	m_numOfEntryElements = 0;
}

CEntry::~CEntry(void)
{
	if (m_chainedLFNEntry != NULL)
		delete[] m_chainedLFNEntry;
}

// Retrieve the total bytes amount of the short file name entry, and the LFN Entry
DWORD CEntry::getEntrySize()
{
	return sizeof(FATDirEntry) + m_numOfEntryElements*sizeof(LFNEntry);
}

bool CEntry::isDeleted()
{
	return isDeletedEntry(m_data);
}

// Returns the data of the entry
// DON'T FORGET TO FREE THE MEMORY OF THE BUFFER
BYTE* CEntry::getData()
{
	BYTE* entryData = new BYTE[getEntrySize()];
	if (getEntrySize() > sizeof(FATDirEntry))
	{
		memcpy(entryData,
				m_chainedLFNEntry, getEntrySize()-sizeof(FATDirEntry));
	}
	memcpy(entryData+getEntrySize()-sizeof(FATDirEntry), 
			&m_data, sizeof(FATDirEntry));
	return entryData;
}

void CEntry::setData(BYTE* aData)
{
	memcpy(&m_data, aData, sizeof(m_data));
}

WCHAR* CEntry::getName()
{
	WCHAR* udini = getLongName();
	if (udini != NULL)
		return udini;
	else
		return getShortName();	
}

WCHAR* CEntry::getShortName()
{
	WCHAR* ret = new WCHAR[sizeof(m_data.DIR_Name)+1];
	for (size_t i = 0; i < sizeof(m_data.DIR_Name); ++i)
		ret[i] = static_cast<unsigned char>(m_data.DIR_Name[i]);
	ret[sizeof(m_data.DIR_Name)] = L'\0';
	return ret;
}

static void appendFatUtf16Unit(std::wstring& output, WORD unit, const std::vector<WORD>& units, size_t& index)
{
	if (unit == 0x0000)
	{
		index = units.size();
		return;
	}
	if (unit == 0xFFFF)
		return;

	if (sizeof(WCHAR) == 2)
	{
		output.push_back(static_cast<WCHAR>(unit));
		return;
	}

	if (unit >= 0xD800 && unit <= 0xDBFF && index + 1 < units.size())
	{
		WORD low = units[index + 1];
		if (low >= 0xDC00 && low <= 0xDFFF)
		{
			unsigned int codepoint = 0x10000 + (((unit - 0xD800) << 10) | (low - 0xDC00));
			output.push_back(static_cast<WCHAR>(codepoint));
			++index;
			return;
		}
	}

	output.push_back(static_cast<WCHAR>(unit));
}

WCHAR* CEntry::getLongName()
{
	// If this is not an LFN
	if (m_numOfEntryElements == 0)
		return NULL;

	LFNEntry* lfnCurrEntry = m_chainedLFNEntry;
	std::vector<WORD> utf16Units;
	utf16Units.reserve(m_numOfEntryElements * 13);

	// The order of the entries is reverse to the order in this entries array (m_numOfEntryElements-1,.., 2, 1,0)
	for (int i=m_numOfEntryElements-1; i>=0; --i)
	{
		utf16Units.insert(utf16Units.end(), lfnCurrEntry[i].LDIR_Name1, lfnCurrEntry[i].LDIR_Name1 + 5);
		utf16Units.insert(utf16Units.end(), lfnCurrEntry[i].LDIR_Name2, lfnCurrEntry[i].LDIR_Name2 + 6);
		utf16Units.insert(utf16Units.end(), lfnCurrEntry[i].LDIR_Name3, lfnCurrEntry[i].LDIR_Name3 + 2);
	}

	std::wstring wideName;
	for (size_t i = 0; i < utf16Units.size(); ++i)
		appendFatUtf16Unit(wideName, utf16Units[i], utf16Units, i);

	WCHAR* wName = new WCHAR[wideName.size() + 1];
	memcpy(wName, wideName.c_str(), wideName.size() * sizeof(WCHAR));
	wName[wideName.size()] = L'\0';
	return wName;
}
