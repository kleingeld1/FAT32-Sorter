#pragma once
#include "CVolumeAccess.h"
#include "CRootFolder.h"

class CFileSystem
{
private:
	CRootFolder*	m_rootDir;
public:
	CFileSystem(const char* aDriveLetter);
	~CFileSystem(void);

	bool initFDT();
	void sort();
	void flushDataToDevice();
	void exportFoldersList(const char* aFileName);
	void changeDriveLetter(const char* aDriveLetter);
	const char* getCurrentDriveLetter();

	// Backup function for the files table
	void dumpFilesTable(const char* aFileName);
	void loadFilesTable(const char* aFileName);

	// Dumping the FAT tables to files
	void dumpFatsTable(const char* aDestFolder);
};
