/**
 * FileManager.h
 * CMPS181 - Spring 2015
 * Project 2
 *
 * Benjamin (Benjy) Strauss
 * Paul-Valentin Mini (pcamille)
 */


#include <string>

#ifndef _pfm_h_
#define _pfm_h_

typedef int RC;
typedef unsigned PageNum;

using std::string;

#define PAGE_SIZE 4096
#define PAGED_FILE_HEADER_STRING "PAGED_FILE__"
#define PAGED_FILE_HEADER_STRING_LENGTH 12
#define SUCCESS 0
#define CORE_DEBUG 0
#define DEBUG 0

class FileHandle {
private:
    FILE* _fileDescriptor;											//Project 1 Imported
public:
	string fname;
    FileHandle();
    ~FileHandle();

	int readPage(unsigned pageNum, void *data);						// Get a specific page
	int writePage(unsigned pageNum, const void *data);				// Write a specific page
	int appendPage(const void *data);								// Append a specific page
	unsigned getNumberOfPages();									// Get the number of pages in the file

    void setFileDescriptor(FILE * fileDescriptor);					//Project 1 Imported
    FILE * getFileDescriptor();										//Project 1 Imported
};

class PagedFileManager {
private:
	static PagedFileManager *_pf_manager;
	bool FileExists(string fileName);								//Project 1 Imported
protected:
	PagedFileManager();
	~PagedFileManager();
public:
	static PagedFileManager* instance();							// Access to the _pf_manager instance

	int createFile (const char *fileName);							// Create a new file
	int destroyFile (const char *fileName);                         // Destroy a file
	int openFile (const char *fileName, FileHandle &fileHandle);	// Open a file
	int closeFile (FileHandle &fileHandle);							// Close a file
};

 #endif
