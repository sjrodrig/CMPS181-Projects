#include <iostream>
#include <string>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdexcept>
#include <stdio.h>
#include "FileManager.h"

using namespace std;

/**
 * Built on Paolo Di Febbo's project 1
 * @modifyer: Benjamin (Benjy) Strauss
 *
 * 
 */

PagedFileManager* PagedFileManager::_pf_manager = 0;

PagedFileManager* PagedFileManager::instance() {
	if(!_pf_manager) { _pf_manager = new PagedFileManager(); }
	return _pf_manager;
}


PagedFileManager::PagedFileManager() {

}


PagedFileManager::~PagedFileManager() {

}


int
PagedFileManager::createFile(const char *fileName) {
	// If file already exists, error.
	if (FileExists(string(fileName))) {
		return 1;
	}

    FILE * pFile;
	pFile = fopen(fileName, "w");

	if (pFile == NULL) {
		// If file cannot be created, error.
		return 2;
	}

	if (fputs(PAGED_FILE_HEADER_STRING, pFile) < 0) {
		// If something goes wrong during the writing, error.
		return 3;
	}

	fclose (pFile);
	return 0;
}


int
PagedFileManager::destroyFile(const char *fileName) {
	// If file cannot be successfully removed, error.
	if (remove(fileName) != 0) {
		return 1;
	}

	return 0;
}


int
PagedFileManager::openFile(const char *fileName, FileHandle &fileHandle) {
   // Checks if fileHandle is already an handle for an open file.
	if (fileHandle.getFileDescriptor() != NULL) {
		return 1;
	}

	// If file does not exist, error.
	if (!FileExists(string(fileName))) {
		return 2;
	}

	FILE * pFile;
	pFile = fopen(fileName, "r+");
	if (pFile == NULL) {
		// File open error.
		return 3;
	}

	// Check if the file is actually a paged file by looking at its header.
	char headerCheck[PAGED_FILE_HEADER_STRING_LENGTH + 1];
	fgets(headerCheck, PAGED_FILE_HEADER_STRING_LENGTH + 1, pFile);

	if (strcmp(headerCheck, PAGED_FILE_HEADER_STRING) != 0) {
		// Not a page file, close it + error.
		fclose(pFile);
		return 4;
	}

	// Else, it is a page file. Put its descriptor into the fileHandle.
	fileHandle.setFileDescriptor(pFile);
	return 0;
}


int
PagedFileManager::closeFile(FileHandle &fileHandle) {
    FILE * pFile = fileHandle.getFileDescriptor();

	// Check if fileHandle is actually an handle for an open file.
	if (pFile == NULL) {
		return 1;
	}

	// Flushes the pending data.
	fflush(pFile);

	// Closes the file.
	fclose(pFile);

	// Destroys the fileHandle pointer.
	fileHandle.setFileDescriptor(NULL);

	return 0;
}

bool
PagedFileManager::FileExists(string fileName) {
    struct stat stFileInfo;

    if(stat(fileName.c_str(), &stFileInfo) == 0) { return true; }
    else { return false; }
}

/******************************************************************************
 *								FileHandle Methods							  *
 ******************************************************************************/

FileHandle::FileHandle() {
	_fileDescriptor = NULL;
}


FileHandle::~FileHandle(){

}


int
FileHandle::readPage(unsigned pageNum, void *data) {
	// Check if the page exists.
    if (getNumberOfPages() < pageNum) {
    	return 1;
	}

    // Seek to the start of the page.
    if (fseek(_fileDescriptor, PAGED_FILE_HEADER_STRING_LENGTH + PAGE_SIZE * pageNum, SEEK_SET) != SUCCESS) {
    	return 2;
	}

    // Read the page data.
    if (fread(data, 1, PAGE_SIZE, _fileDescriptor) == PAGE_SIZE) {
    	return 0;
    } else {
    	return 3;
	}
}


int
FileHandle::writePage(unsigned pageNum, const void *data) {
	// Check if the page exists.
	if (getNumberOfPages() < pageNum)
		return 1;

	// Seek to the start of the page.
	if (fseek(_fileDescriptor, PAGED_FILE_HEADER_STRING_LENGTH + PAGE_SIZE * pageNum, SEEK_SET) != SUCCESS) {
		return 2;
	}

	// Write the page.
	if (fwrite(data, 1, PAGE_SIZE, _fileDescriptor) == PAGE_SIZE) {
		// Immediately commits the changes to disk.
		fflush(_fileDescriptor);
		return 0;
	} else {
		return 3;
	}
}


int
FileHandle::appendPage(const void *data) {
	// Seek to the end of the file.
	if (fseek(_fileDescriptor, 0, SEEK_END) != SUCCESS) {
		return 1;
	}

	// Write the new page.
	if (fwrite(data, 1, PAGE_SIZE, _fileDescriptor) == PAGE_SIZE) {
		fflush(_fileDescriptor);
		return 0;
	}
	else {
		return 2;
	}
}


unsigned
FileHandle::getNumberOfPages() {
    // Number of pages is given by: (file size - paged file header size) / page size
	if(CORE_DEBUG) {
		cout << "seeking" << endl;
		cout << "_fileDescriptor is: " << _fileDescriptor << endl;
	}

	fseek(_fileDescriptor, 0, SEEK_END); //segfault here
	return (ftell(_fileDescriptor) - PAGED_FILE_HEADER_STRING_LENGTH) / PAGE_SIZE;
}


void FileHandle::setFileDescriptor(FILE * fileDescriptor) {
	_fileDescriptor = fileDescriptor;
}


FILE * FileHandle::getFileDescriptor() {
	return _fileDescriptor;
}


