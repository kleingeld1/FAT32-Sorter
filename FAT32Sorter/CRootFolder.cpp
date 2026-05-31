#include "stdafx.h"
#include "CRootFolder.h"

CRootFolder::CRootFolder()
:CFolderEntry()
{
}

DWORD CRootFolder::getFirstClusterInDataChain()
{
	return CVolumeAccess::getInstance()->getRootDirCluster();
}

WCHAR* CRootFolder::getName()
{
	WCHAR* ret = new WCHAR[5];
	wcscpy(ret, L"ROOT");
	return ret;
}

bool CRootFolder::dumpDirTable(const char* aFileName)
{
	ofstream file(aFileName,ios::binary | ios::out);
	bool ret = dumpData(&file);
	file.close();

	return ret;
}
