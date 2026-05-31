#include "stdafx.h"
#include "CFileSystem.h"

CFileSystem::CFileSystem(const char* aDriveLetter)
{
	CVolumeAccess::setWorkingDriveLetter(aDriveLetter);
	m_rootDir = NULL;
}

CFileSystem::~CFileSystem(void)
{
	CVolumeAccess::cleanResources();
	CVolumeAccess::setWorkingDriveLetter(NULL);

	if (m_rootDir != NULL)
	{
		delete m_rootDir;
		m_rootDir = NULL;
	}
}

bool CFileSystem::initFDT()
{	
	// Cleans any older data
	if (m_rootDir != NULL)
	{
		delete m_rootDir;
		m_rootDir = NULL;
	}

	if (CVolumeAccess::getInstance() == NULL)
	{
		printf("The device is not ready..\n");
		return false;
	}
	else
	{
		m_rootDir = new CRootFolder();
		m_rootDir->load();
		return true;
	}
}

void CFileSystem::sort()
{
	printf("Sorting the files table...");
	m_rootDir->sortEntries();
	printf("DONE!\n");
}

void CFileSystem::flushDataToDevice()
{
	if (m_rootDir->writeData())
	{
		printf("Data flushed to device successfully!\n");
	}
}

void CFileSystem::exportFoldersList(const char* aFileName)
{
	if (!initFDT())
		return;

	printf("Exporting Files list to %s\n", aFileName);

	// Resets the folders' counter
	CFolderEntry::g_runningNum = 0;

	FILE* exportFile = fopen(aFileName, "w");
	if (exportFile == NULL)
	{
		printf("Could not open %s for writing.\n", aFileName);
		return;
	}

	fprintf(exportFile, "Exporting The drive's table to a tree-list:\n\n");
	WCHAR* rootName = m_rootDir->getName();
	std::string rootNameUtf8 = wideToUtf8(rootName);
	fprintf(exportFile, "%s\n", rootNameUtf8.c_str());
	delete[] rootName;
	m_rootDir->exportToFile(exportFile, 0);
	fclose(exportFile);
	printf("Files list saved to %s\n", aFileName);
}

void CFileSystem::dumpFilesTable(const char* aFileName)
{
	if (!initFDT())
		return;

	printf("Saving backup of the files table..\n");
		
	if (m_rootDir == NULL)
	{
		printf("\nError! Tried to dump files table before loading it\n");
	}
	else
	{
		if (!m_rootDir->dumpDirTable(aFileName))
		{
			printf("Error encountered while backing up the table\n");
		}
		else
		{
			printf("Backup saved to file: %s!..\n", aFileName);
		}
	}
}
void CFileSystem::loadFilesTable(const char* aFileName)
{
	ifstream fatFile(aFileName, ios::binary|ios::in);
	if (!fatFile.is_open())
	{
		printf("The file \"%s\" does not exist!", aFileName);
		return;
	}
	if (!initFDT())
		return;

	printf("\nLoading into the device the files data from file: %s\n", aFileName);
	
	bool isError = false;


	// Read the header
	BYTE header[16];
	fatFile.read((char*)header, 16);

	while (!fatFile.eof() && !isError)
	{
		DWORD sizeOfData = 0;
		DWORD startClusterNum = 0;

		memcpy(&sizeOfData, header+0x8, sizeof(DWORD));
		memcpy(&startClusterNum, header+0xC, sizeof(DWORD));

		BYTE* dataLoaded = new BYTE[sizeOfData];
		fatFile.read((char*)dataLoaded, sizeOfData);

		printf("Loading 0x%4X bytes, starting cluster number 0x%4X...", sizeOfData, startClusterNum);
		
		// Writing the data to the device
		if (!CVolumeAccess::getInstance()->writeChainedClusters(startClusterNum, dataLoaded, sizeOfData))
		{
			printf("\nError loading data to the device. Error Code: 0x%2lX. Cluster: 0x%4X\n", GetLastError(), startClusterNum);
			isError = true;
		}
		else
		{
			printf("DONE!\n");
			fatFile.read((char*)header, 16);
		}

		delete[] dataLoaded;
	}

	if (isError)
	{
		printf("Error while loading the table from the file\n");
	}
	else
	{
		printf("Table loaded successfully!\n");
	}
}

void CFileSystem::dumpFatsTable(const char* aDestFolder)
{
	CVolumeAccess::getInstance()->dumpFatsData(aDestFolder);
}
void CFileSystem::changeDriveLetter(const char* aDriveLetter)
{
	CVolumeAccess::setWorkingDriveLetter(aDriveLetter);
}
const char* CFileSystem::getCurrentDriveLetter()
{
	return CVolumeAccess::getWorkingDriveLetter();
}
