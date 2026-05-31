// FAT32Sorter.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <time.h>
#include "General.h"
#include "CFileSystem.h"
#include <string>

#define DRIVE_CHOOSE 1
#define DUMP_TABLES 2
#define LOAD_TABLES 3
#define EXPORT_LIST 4
#define SORT 5
#define EXIT 6

#if defined(_WIN32)
static const char* DEFAULT_VOLUME = "F";
#else
static const char* DEFAULT_VOLUME = "";
#endif

char* getString(char* line, int size);
bool getNum(int *result);

int menu(const char* aCurrDriveLetter)
{
	int userPick = -1;

	printf("\n\n\t**************************************************\n");
	printf("\n\t\t-= FAT Sorter =-\n");
	printf("\t1. Choose volume (Current is \"%s\")\n", aCurrDriveLetter);
	printf("\t2. Dump files table (Backup)\n");
	printf("\t3. Load file table from file (Recover)\n");
	printf("\t4. Export Folders list\n");
	printf("\t5. Sort the FAT Table\n");
	printf("\t6. Exit\n");
	printf("\n\nEnter your choice: ");

	bool success = getNum(&userPick);
	while (!success)
	{
		printf("Enter your choice: ");
		success = getNum(&userPick);
	}
	
	return userPick;
}

void backupFileName(char* aFileName)
{
	time_t ltime;
	struct tm Tm;

	ltime=time(NULL);
#if defined(_WIN32)
	localtime_s(&Tm, &ltime);
#else
	localtime_r(&ltime, &Tm);
#endif

	snprintf(aFileName, 20, "%04d%02d%02d_%02d%02d%02d.dat",
			Tm.tm_year+1900,
			Tm.tm_mon+1,
			Tm.tm_mday,
			Tm.tm_hour,
			Tm.tm_min,
			Tm.tm_sec);
}

char* getString(char *line, int size)
{
	if (fgets(line, size, stdin) )
	{
		char* newline = strchr(line, '\n');
		if ( newline )
		{
			*newline = '\0';
		}
	}
	return line;
}

bool getNum(int *result)
{
	char *end, buff [ 13 ];
	
	if (fgets(buff, sizeof buff, stdin) == NULL)
		return false;
	*result = strtol(buff, &end, 10);
	return !isspace(*buff) && end != buff && (*end == '\n' || *end == '\0');
}

bool areYouSureMsg(const char* text)
{
	char answer[8];
	printf("%s", text);
	getString(answer, sizeof(answer));
	return (answer[0]=='y' || answer[0]=='Y');
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); 

	const char* defaultDriveLetter = (argc >= 2) ? argv[1] : DEFAULT_VOLUME;
	CFileSystem fatFileSystem(defaultDriveLetter);
	
	int choice = -1;
	bool overrideMenu = false;

	if (argc >= 3)
	{
		overrideMenu = true;
		
		if (strcmp(argv[2], "sort") == 0)
		{
			choice = SORT;
		}
	}
	else
	{
		choice=menu(fatFileSystem.getCurrentDriveLetter());
	}

	if (choice != EXIT)
	{
		do 
		{
			printf("\n\n");
			switch (choice)
			{
				case (DRIVE_CHOOSE):
				{
					char drive[1024];
				
#if defined(_WIN32)
					printf("Please write the drive letter or volume path of the FAT32 drive (E, F, \\\\.\\F: etc.): ");
#else
					printf("Please write the FAT32 device or image path (/dev/disk2s1, /dev/sdb1, disk.img etc.): ");
#endif
					do{
						getString(drive, sizeof(drive));
					} while (strcmp(drive, "") == 0);

					printf("Selected volume is: %s\n", drive);

					fatFileSystem.changeDriveLetter(drive);
					break;
				}
				case(DUMP_TABLES):
				{
					fatFileSystem.dumpFilesTable("dirs.dat");
					break;
				}
				case (LOAD_TABLES):
				{
					if (areYouSureMsg("Are you sure you want to recover the files' table from \"dirs.dat\" (y/n) ? "))
					{
						fatFileSystem.loadFilesTable("dirs.dat");
					}
					break;
				}
				case (EXPORT_LIST):
				{
					fatFileSystem.exportFoldersList("FilesList.txt");
					break;
				}
				case (SORT):
				{
					if (overrideMenu || areYouSureMsg("Are you sure you want to sort the entire files' table (backup will be saved) [y/n] ? "))
					{			
						if (fatFileSystem.initFDT())
						{
							// Backup the current table.
							char backupFile[20];
							backupFileName(backupFile);
							fatFileSystem.dumpFilesTable(backupFile);

							fatFileSystem.sort();
							fatFileSystem.flushDataToDevice();	
							printf("\n\t***************************************************************\n");
							printf("\tBackup data was saved to \"%s\". \n", backupFile);
							printf("\tTo recover - rename to \"dirs.dat\" and apply option number %d", LOAD_TABLES);
							printf("\n\t***************************************************************\n");
						}
					}
					break;
				}
				case (EXIT):
				{
					break;
				}
				default:
				{
					printf("Option does not exist, try again..\n");
					break;
				}
			}
		} while (!overrideMenu && (choice=menu(fatFileSystem.getCurrentDriveLetter())) != EXIT);
	}	
	
	return 0;
}
