#pragma once
#include "CEntry.h"

class CSpecialEntry :
	public CEntry
{
public:
	CSpecialEntry(FATDirEntry aDirEntry);
	~CSpecialEntry(void);
};
