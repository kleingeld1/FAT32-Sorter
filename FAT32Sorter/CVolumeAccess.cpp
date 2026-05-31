#include "stdafx.h"
#include "CVolumeAccess.h"
#include <iostream>
#include <fstream>

#if !defined(_WIN32)
#include <fcntl.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(__linux__)
#include <linux/fs.h>
#elif defined(__APPLE__)
#include <sys/disk.h>
#endif
#endif

using namespace std;

namespace
{
#if defined(_WIN32)
	const HANDLE INVALID_VOLUME_HANDLE = INVALID_HANDLE_VALUE;
#else
	const int INVALID_VOLUME_HANDLE = -1;
#endif

	char* duplicateString(const char* value)
	{
		if (value == NULL)
			return NULL;

		size_t size = strlen(value) + 1;
		char* copy = new char[size];
		memcpy(copy, value, size);
		return copy;
	}

	std::string buildNativeVolumePath(const char* volume)
	{
		if (volume == NULL)
			return std::string();

		std::string path(volume);
#if defined(_WIN32)
		if (path.size() == 1)
			return std::string("\\\\.\\") + path + ":";
		if (path.size() == 2 && path[1] == ':')
			return std::string("\\\\.\\") + path;
#endif
		return path;
	}
}

CVolumeAccess* CVolumeAccess::s_instance = NULL;
char* CVolumeAccess::s_driveLetter = NULL;

CVolumeAccess* CVolumeAccess::getInstance()
{
	try
	{
		if (s_driveLetter == NULL || s_driveLetter[0] == '\0')
		{
			printf("No FAT32 volume was selected.\n");
			return NULL;
		}

		if (s_instance == NULL)
			s_instance = new CVolumeAccess(CVolumeAccess::s_driveLetter);
		return s_instance; 
	}
	// If we have trouble accessing the volume
	catch (...)
	{
		return NULL;
	}
}

CVolumeAccess::CVolumeAccess(const char* aVolumeDriveLetter)
	: m_hDevice(INVALID_VOLUME_HANDLE),
	  m_sectorSize(512),
	  m_clusterSizeBytes(0),
	  m_FAT1Data(NULL),
	  m_FAT2Data(NULL)
{
	std::string volumePath = buildNativeVolumePath(aVolumeDriveLetter);

#if defined(_WIN32)
	HANDLE hDevice =
		CreateFileA(volumePath.c_str(),
					GENERIC_READ|GENERIC_WRITE,
					FILE_SHARE_READ|FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	if (hDevice == INVALID_HANDLE_VALUE)
#else
	int hDevice = open(volumePath.c_str(), O_RDWR);

	if (hDevice == -1)
#endif
	{
		printf("Error opening connection to volume [%s]\n", aVolumeDriveLetter);
		throw "Error accessing the volume";
	}

	m_hDevice = hDevice;

	if (!lockAndDismount())
	{
		clean();
		throw "Error locking/dismounting the device";
	}

	// Initialize mandatory data needed for communication with the volume.
	initData();
}

CVolumeAccess::~CVolumeAccess()
{
	clean();
}

void CVolumeAccess::cleanResources()
{
	if (s_instance != NULL)
	{
		delete s_instance;
		s_instance = NULL;
	}
	if (s_driveLetter != NULL)
	{
		delete[] s_driveLetter;
		s_driveLetter = NULL;
	}
}

void CVolumeAccess::clean()
{
#if defined(_WIN32)
	if (m_hDevice != INVALID_HANDLE_VALUE && m_hDevice != NULL)
		CloseHandle(m_hDevice);
#else
	if (m_hDevice != -1)
		close(m_hDevice);
#endif
	m_hDevice = INVALID_VOLUME_HANDLE;

	if (m_FAT1Data != NULL)
	{
		delete[] m_FAT1Data;
		m_FAT1Data = NULL;
	}
	if (m_FAT2Data != NULL)
	{
		delete[] m_FAT2Data;
		m_FAT2Data = NULL;
	}
}

void CVolumeAccess::setWorkingDriveLetter(const char* aDriveToUse)
{
	if (CVolumeAccess::s_instance != NULL)
	{
		delete CVolumeAccess::s_instance;
		CVolumeAccess::s_instance = NULL;
	}

	if (CVolumeAccess::s_driveLetter != NULL)
		delete[] CVolumeAccess::s_driveLetter;

	CVolumeAccess::s_driveLetter = duplicateString(aDriveToUse);
}

const char* CVolumeAccess::getWorkingDriveLetter()
{
	return CVolumeAccess::s_driveLetter == NULL ? "" : CVolumeAccess::s_driveLetter;
}

bool CVolumeAccess::lockAndDismount()
{
#if defined(_WIN32)
	printf("Dismounting and locking the volume...");

	DWORD dwReturned;
	BOOL bRes = DeviceIoControl( m_hDevice, FSCTL_DISMOUNT_VOLUME, 0, 0, 0, 0, &dwReturned, 0 );

	if(!bRes )
	{
		printf("Error dismounting the volume (Error=0x%lX)\n", GetLastError());
		return false;
	}
	else
	{
		bRes = DeviceIoControl( m_hDevice, FSCTL_LOCK_VOLUME, 0, 0, 0, 0, &dwReturned, 0 );
		if (!bRes)
		{
			printf("Error locking the volume (Error=0x%lX)\n", GetLastError());
			return false;
		}
		else
		{
			printf("Done!\n");
			return true;
		}
	}
#else
	printf("Preparing raw volume access...");
	if (flock(m_hDevice, LOCK_EX | LOCK_NB) != 0)
	{
		printf("\nWarning: could not acquire an advisory lock (Error=0x%lX). Continuing anyway.\n", GetLastError());
	}
	else
	{
		printf("Done!\n");
	}

	printf("Warning: this program cannot unmount volumes on Unix. Unmount the FAT32 filesystem before writing.\n");
	return true;
#endif
}

bool CVolumeAccess::setDeviceOffset(unsigned long long aOffset)
{
#if defined(_WIN32)
	LARGE_INTEGER liPos;
	liPos.QuadPart = static_cast<LONGLONG>(aOffset);
	if (!SetFilePointerEx(m_hDevice, liPos, NULL, FILE_BEGIN))
	{
		printf("SetPointer FAILED! 0x%lX\n", GetLastError());
		return false;
	}
#else
	if (lseek(m_hDevice, static_cast<off_t>(aOffset), SEEK_SET) == static_cast<off_t>(-1))
	{
		printf("Seek FAILED! 0x%lX\n", GetLastError());
		return false;
	}
#endif
	return true;
}

bool CVolumeAccess::readRawBytes(BYTE* aBuffer, DWORD aSizeOfData)
{
	DWORD totalRead = 0;
	while (totalRead < aSizeOfData)
	{
#if defined(_WIN32)
		DWORD bytesRead = 0;
		if (!ReadFile(m_hDevice, aBuffer + totalRead, aSizeOfData - totalRead, &bytesRead, NULL))
		{
			printf("Error reading from the device, Code: 0x%lX\n", GetLastError());
			return false;
		}
#else
		ssize_t bytesRead = read(m_hDevice, aBuffer + totalRead, aSizeOfData - totalRead);
		if (bytesRead < 0 && errno == EINTR)
			continue;
		if (bytesRead < 0)
		{
			printf("Error reading from the device, Code: 0x%lX\n", GetLastError());
			return false;
		}
#endif
		if (bytesRead == 0)
		{
			printf("Unexpected end of device while reading.\n");
			return false;
		}
		totalRead += static_cast<DWORD>(bytesRead);
	}
	return true;
}

bool CVolumeAccess::writeRawBytes(const BYTE* aBuffer, DWORD aSizeOfData)
{
	DWORD totalWritten = 0;
	while (totalWritten < aSizeOfData)
	{
#if defined(_WIN32)
		DWORD bytesWritten = 0;
		if (!WriteFile(m_hDevice, aBuffer + totalWritten, aSizeOfData - totalWritten, &bytesWritten, NULL))
		{
			printf("Error writing to the device, Code: 0x%lX\n", GetLastError());
			return false;
		}
#else
		ssize_t bytesWritten = write(m_hDevice, aBuffer + totalWritten, aSizeOfData - totalWritten);
		if (bytesWritten < 0 && errno == EINTR)
			continue;
		if (bytesWritten < 0)
		{
			printf("Error writing to the device, Code: 0x%lX\n", GetLastError());
			return false;
		}
#endif
		if (bytesWritten == 0)
		{
			printf("Unexpected end of device while writing.\n");
			return false;
		}
		totalWritten += static_cast<DWORD>(bytesWritten);
	}
	return true;
}

void CVolumeAccess::readBootSector()
{
	printf("Boot sector size = %lu Bytes\n", static_cast<unsigned long>(sizeof(FATBootSector)));

	if (!setDeviceOffset(0))
		return;

	BYTE* lTemp = new BYTE[m_sectorSize];
	memset(lTemp, 0, m_sectorSize);

	if (readRawBytes(lTemp, m_sectorSize))
		memcpy(&m_bootSector, lTemp, sizeof(m_bootSector));

	delete[] lTemp;
}

void CVolumeAccess::initData()
{
#if defined(_WIN32)
	DISK_GEOMETRY_EX lDiskGeo;
	DWORD lBytes;
	
	if (DeviceIoControl(m_hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &lDiskGeo, sizeof(DISK_GEOMETRY_EX), &lBytes, NULL))
		m_sectorSize = lDiskGeo.Geometry.BytesPerSector;
#elif defined(__linux__)
	int logicalSectorSize = 0;
	if (ioctl(m_hDevice, BLKSSZGET, &logicalSectorSize) == 0 && logicalSectorSize > 0)
		m_sectorSize = static_cast<DWORD>(logicalSectorSize);
#elif defined(__APPLE__)
	uint32_t logicalSectorSize = 0;
	if (ioctl(m_hDevice, DKIOCGETBLOCKSIZE, &logicalSectorSize) == 0 && logicalSectorSize > 0)
		m_sectorSize = static_cast<DWORD>(logicalSectorSize);
#endif

	printf("Sector size is %lu bytes\n", static_cast<unsigned long>(m_sectorSize));

	// Initialize the boot sector data member.
	readBootSector();
	if (m_bootSector.BPB_BytsPerSec != 0)
		m_sectorSize = m_bootSector.BPB_BytsPerSec;

	readFatsData();

	// Init the cluster size.
	m_clusterSizeBytes = m_bootSector.PBP_SecPerClus * m_sectorSize;
}

void CVolumeAccess::readFatsData()
{
	printf("Reading FATs Data...");

	// Calc the size in BYTES!
	long lFatTableSize = m_bootSector.BPB_FATsz32 * m_sectorSize;

	// The start sector of FAT1 is the first sector available.
	m_FAT1Data = (DWORD*)new BYTE[lFatTableSize];
	readBytesFromDeviceSector((BYTE*)m_FAT1Data, lFatTableSize, 0);

	// The FAT32 start sector is right after FAT1.
	m_FAT2Data = (DWORD*)new BYTE[lFatTableSize];
	readBytesFromDeviceSector((BYTE*)m_FAT2Data, lFatTableSize, m_bootSector.BPB_FATsz32);

	printf("DONE!\n");
}

void CVolumeAccess::dumpFatsData(const char* aDestPath)
{
	// Calc the size in BYTES!
	long lFatTableSize = m_bootSector.BPB_FATsz32 * m_sectorSize;

	std::string basePath = aDestPath == NULL ? "" : aDestPath;
	std::string file1Path = basePath + "fat1.dat";
	printf("Dumping Fat1 data to file %s...", file1Path.c_str());
	writeDataToFile((BYTE*)m_FAT1Data, lFatTableSize, file1Path.c_str());
	printf("DONE!\n");
	
	std::string file2Path = basePath + "fat2.dat";
	printf("Dumping Fat2 data to file %s...", file2Path.c_str());
	writeDataToFile((BYTE*)m_FAT2Data, lFatTableSize, file2Path.c_str());
	printf("DONE!\n");
}

bool CVolumeAccess::readBytesFromDeviceCluster(BYTE* aBuffer, DWORD aSizeOfData, DWORD aStartCluster)
{
	// We need to compute the position in the file, so first convert cluster num -> sector num.
	DWORD startSectorNum = getSectorNumFromCluster(aStartCluster);

	return readBytesFromDeviceSector(aBuffer, aSizeOfData, startSectorNum);
}

bool CVolumeAccess::readBytesFromDeviceSector(BYTE* aBuffer, DWORD aSizeOfData, DWORD aStartSector)
{
	if (!goToSector(aStartSector))
		return false;

	return readRawBytes(aBuffer, aSizeOfData);
}

bool CVolumeAccess::writeBytesToDeviceCluster(BYTE* aBuffer, DWORD aSizeOfData, DWORD aStartCluster)
{
	// We need to compute the position in the file, so first convert cluster num -> sector num.
	DWORD startSectorNum = getSectorNumFromCluster(aStartCluster);

	return writeBytesToDeviceSector(aBuffer, aSizeOfData, startSectorNum);
}

bool CVolumeAccess::writeBytesToDeviceSector(BYTE* aBuffer, DWORD aSizeOfData, DWORD aStartSector)
{
	if (!goToSector(aStartSector))
		return false;

	return writeRawBytes(aBuffer, aSizeOfData);
}

// Sets the device's pointer to the argumented sector number.
bool CVolumeAccess::goToSector(DWORD aSectorNum)
{
	// The argumented sector is relative to the first available sector,
	// which is after the reserved sectors.
	unsigned long long pos =
		(static_cast<unsigned long long>(m_bootSector.BPB_RsvdSecCnt) + aSectorNum) *
		static_cast<unsigned long long>(m_sectorSize);

	if (!setDeviceOffset(pos))
	{
		printf("Failed accessing sector number %lu\n", static_cast<unsigned long>(aSectorNum));
		return false;
	}

	return true;
}

bool CVolumeAccess::isEndOfClusterChain(DWORD aClusterNum)
{
	return (aClusterNum & 0x0FFFFFFF) >= 0x0FFFFFF8;
}

bool CVolumeAccess::writeChainedClusters(DWORD aStartClusterNum, BYTE* aiChainedClustersData, DWORD aSizeOfData)
{
	DWORD dwNextClusterNum = aStartClusterNum & 0x0FFFFFFF;
	DWORD dwNumClustersPassed = 0;

	while ((aSizeOfData - (dwNumClustersPassed*m_clusterSizeBytes)) >= m_clusterSizeBytes)
	{
		if (isEndOfClusterChain(dwNextClusterNum))
		{
			printf("Error getting the next cluster while writing data. Error code: 0x%lX\n", GetLastError());
			return false;
		}

		// Writes a whole cluster from the current free position in the buffer.
		if (!writeBytesToDeviceCluster(aiChainedClustersData+(dwNumClustersPassed*m_clusterSizeBytes), 
										m_clusterSizeBytes,
										dwNextClusterNum))
		{
			return false;
		}
		dwNextClusterNum = m_FAT1Data[dwNextClusterNum] & 0x0FFFFFFF;
		++dwNumClustersPassed;
	}

	DWORD bytesLeft = aSizeOfData - (dwNumClustersPassed*m_clusterSizeBytes);
	if (bytesLeft > 0 && bytesLeft < m_clusterSizeBytes)
	{
		if (isEndOfClusterChain(dwNextClusterNum))
		{
			printf("Error getting the next cluster while writing data. Error code: 0x%lX\n", GetLastError());
			return false;
		}

		BYTE* clusterComplete = new BYTE[m_clusterSizeBytes];
		memset(clusterComplete, 0, m_clusterSizeBytes);

		memcpy(clusterComplete,
				aiChainedClustersData+(dwNumClustersPassed*m_clusterSizeBytes),
				bytesLeft);
		
		bool success = writeBytesToDeviceCluster(clusterComplete,
											m_clusterSizeBytes,
											dwNextClusterNum);
		delete[] clusterComplete;
		if (!success)
			return false;
	}

	return true;
}

// Reads the entire cluster chain data, starting from the argumented cluster num.
// The reading will use the FAT tables to find the next cluster each time.
bool CVolumeAccess::readChainedClusters(DWORD aStartClusterNum, BYTE* aoChainedClustersData, DWORD* aoSizeOfData)
{
	// Gets only the size of the buffer needed.
	if (aoChainedClustersData == NULL)
	{
		DWORD dwChainTotalNumClusters = 0;
		DWORD dwNextClusterNum = aStartClusterNum & 0x0FFFFFFF;

		// Stop at end-of-chain or at an empty spot, which means the FDT entry
		// exists but the FAT entry was not restored.
		while (!isEndOfClusterChain(dwNextClusterNum) && dwNextClusterNum != 0)
		{
			++dwChainTotalNumClusters;
			dwNextClusterNum = m_FAT1Data[dwNextClusterNum] & 0x0FFFFFFF;
		}

		if (dwNextClusterNum == 0)
			*aoSizeOfData = 0;
		else
			*aoSizeOfData = dwChainTotalNumClusters * m_clusterSizeBytes;
	}
	else
	{
		DWORD dwNextClusterNum = aStartClusterNum & 0x0FFFFFFF;
		DWORD dwNumClustersRead = 0;

		while (!isEndOfClusterChain(dwNextClusterNum))
		{
			if (!readBytesFromDeviceCluster(aoChainedClustersData+(dwNumClustersRead*m_clusterSizeBytes), 
										m_clusterSizeBytes,
										dwNextClusterNum))
			{
				return false;
			}
			dwNextClusterNum = m_FAT1Data[dwNextClusterNum] & 0x0FFFFFFF;
			++dwNumClustersRead;
		}
	}
	return true;
}

void CVolumeAccess::printData(BYTE* aData, long aSize)
{
	for (int i=0;i<aSize;i++)
	{
		if (i % 0x10 == 0) printf("\n");
		printf("0x%02X ", aData[i]);
	}
}

void CVolumeAccess::writeDataToFile(BYTE* aData, long aSize, const char* aFileName)
{
	ofstream lFile(aFileName, ios::binary|ios::out);
	lFile.write((const char*)aData, aSize);
	lFile.close();
}

DWORD CVolumeAccess::getRootDirCluster()
{
	return  m_bootSector.BPB_RootClus;
}

DWORD CVolumeAccess::getSectorNumFromCluster(DWORD adwClusterNum)
{
	// FDT start sector =
	// sector number in each FAT * FAT number +
	// (FDT start cluster - 2) * num sectors per cluster.
	DWORD dwSectorNum = m_bootSector.BPB_NumFATs * m_bootSector.BPB_FATsz32 +
						(adwClusterNum - 2) * m_bootSector.PBP_SecPerClus;

	// Reserved sectors are added by the read/write methods.
	return dwSectorNum;
}
