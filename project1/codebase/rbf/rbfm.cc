// CMPS 181 - Project 1
// Author:				Paolo Di Febbo
// File description:	Implementing the "Variable length records" page structure
//						(ref. p. 329 Ramakrishnan, Gehrke).

#include <iostream>
#include <string>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdexcept>
#include <stdio.h>

#include "rbfm.h"

// ------------------------------------------------------------
// RecordBasedFileManager Class
// ------------------------------------------------------------

RecordBasedFileManager* RecordBasedFileManager::_rbf_manager = 0;
PagedFileManager* RecordBasedFileManager::_pf_manager = 0;

RecordBasedFileManager* RecordBasedFileManager::instance()
{
	// Singleton design pattern.
    if(!_rbf_manager)
        _rbf_manager = new RecordBasedFileManager();

    return _rbf_manager;
}

RecordBasedFileManager::RecordBasedFileManager()
{
	// Initialize the internal PagedFileManager instance.
	_pf_manager = PagedFileManager::instance();
}

RecordBasedFileManager::~RecordBasedFileManager()
{
}

// Configures a new record based page, and puts it in "page".
void RecordBasedFileManager::newRecordBasedPage(void * page)
{
	// Writes the slot directory header.
	SlotDirectoryHeader slotHeader;
	slotHeader.freeSpaceOffset = PAGE_SIZE;
	slotHeader.recordEntriesNumber = 0;
	setSlotDirectoryHeader(page, slotHeader);
}

RC RecordBasedFileManager::createFile(const string &fileName) {
    // Creating a new paged file.
	if (_pf_manager->createFile(fileName.c_str()) != SUCCESS)
		return 1;

	// Setting up the first page.
	void * firstPageData = malloc(PAGE_SIZE);
	newRecordBasedPage(firstPageData);

	// Adds the first record based page.
	FileHandle handle;
	_pf_manager->openFile(fileName.c_str(), handle);
	handle.appendPage(firstPageData);
	_pf_manager->closeFile(handle);

	free(firstPageData);

	return 0;
}

RC RecordBasedFileManager::destroyFile(const string &fileName) {
	return _pf_manager->destroyFile(fileName.c_str());
}

RC RecordBasedFileManager::openFile(const string &fileName, FileHandle &fileHandle) {
	return _pf_manager->openFile(fileName.c_str(), fileHandle);
}

RC RecordBasedFileManager::closeFile(FileHandle &fileHandle) {
    return _pf_manager->closeFile(fileHandle);
}

SlotDirectoryHeader RecordBasedFileManager::getSlotDirectoryHeader(void * page)
{
	// Getting the slot directory header.
	SlotDirectoryHeader slotHeader;
	memcpy (&slotHeader, page, sizeof(SlotDirectoryHeader));
	return slotHeader;
}

void RecordBasedFileManager::setSlotDirectoryHeader(void * page, SlotDirectoryHeader slotHeader)
{
	// Setting the slot directory header.
	memcpy (page, &slotHeader, sizeof(SlotDirectoryHeader));
}

SlotDirectoryRecordEntry RecordBasedFileManager::getSlotDirectoryRecordEntry(void * page, unsigned recordEntryNumber)
{
	// Getting the slot directory entry data.
	SlotDirectoryRecordEntry recordEntry;
	memcpy	(
			&recordEntry,
			((char*) page + sizeof(SlotDirectoryHeader) + recordEntryNumber * sizeof(SlotDirectoryRecordEntry)),
			sizeof(SlotDirectoryRecordEntry)
			);

	return recordEntry;
}

void RecordBasedFileManager::setSlotDirectoryRecordEntry(void * page, unsigned recordEntryNumber, SlotDirectoryRecordEntry recordEntry)
{
	// Setting the slot directory entry data.
	memcpy	(
			((char*) page + sizeof(SlotDirectoryHeader) + recordEntryNumber * sizeof(SlotDirectoryRecordEntry)),
			&recordEntry,
			sizeof(SlotDirectoryRecordEntry)
			);
}

// Computes the free space of a page (function of the free space pointer and the slot directory size).
unsigned RecordBasedFileManager::getPageFreeSpaceSize(void * page) {

	SlotDirectoryHeader slotHeader = getSlotDirectoryHeader(page);

	return slotHeader.freeSpaceOffset - slotHeader.recordEntriesNumber * sizeof(SlotDirectoryRecordEntry) - sizeof(SlotDirectoryHeader);
}

unsigned RecordBasedFileManager::getRecordSize(const vector<Attribute> &recordDescriptor, const void *data) {

	unsigned size = 0;
	unsigned varcharSize = 0;

	for (unsigned i = 0; i < (unsigned) recordDescriptor.size(); i++)
		switch (recordDescriptor[i].type)
		{
			case TypeInt:
				size += INT_SIZE;
			break;
			case TypeReal:
				size += REAL_SIZE;
			break;
			case TypeVarChar:
				// We have to get the size of the VarChar field by reading the integer that precedes the string value itself.
				memcpy(&varcharSize, (char*) data + size, VARCHAR_LENGTH_SIZE);
				// We also have to account for the overhead given by that integer.
				size += INT_SIZE + varcharSize;
			break;
		}

	return size;
}

RC RecordBasedFileManager::insertRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, RID &rid) {
	// Gets the size of the record.
	unsigned recordSize = getRecordSize(recordDescriptor, data);

	// Cycles through pages looking for enough free space for the new entry.
	void * pageData = malloc(PAGE_SIZE);
	bool pageFound = false;
	unsigned i;
	for (i = 0; i < fileHandle.getNumberOfPages(); i++)
	{
		if (fileHandle.readPage(i, pageData) != SUCCESS)
			return 1;

		// When we find a page with enough space (accounting also for the size that will be added to the slot directory), we stop the loop.
		if (getPageFreeSpaceSize(pageData) >= sizeof(SlotDirectoryRecordEntry) + recordSize)
		{
			pageFound = true;
			break;
		}
	}

	if(!pageFound)
	{
		// If we are here, there are no pages with enough space.
		// TODO (Project 2?): implementing the reorganizePage method and try to squeeze the records to get enough space
		// In this case, we just create a new white page.
		newRecordBasedPage(pageData);
	}

	SlotDirectoryHeader slotHeader = getSlotDirectoryHeader(pageData);

	// Setting the return RID.
	rid.pageNum = i;
	rid.slotNum = slotHeader.recordEntriesNumber;

	// Adding the new record reference in the slot directory.
	SlotDirectoryRecordEntry newRecordEntry;
	newRecordEntry.length = recordSize;
	newRecordEntry.offset = slotHeader.freeSpaceOffset - recordSize;
	setSlotDirectoryRecordEntry(pageData, rid.slotNum, newRecordEntry);

	// Updating the slot directory header.
	slotHeader.freeSpaceOffset = newRecordEntry.offset;
	slotHeader.recordEntriesNumber += 1;
	setSlotDirectoryHeader(pageData, slotHeader);

	// Adding the record data.
	memcpy	(((char*) pageData + newRecordEntry.offset), data, recordSize);

	// Writing the page to disk.
	if (pageFound)
	{
		if (fileHandle.writePage(i, pageData) != SUCCESS)
			return 2;
	}
	else
	{
		if (fileHandle.appendPage(pageData) != SUCCESS)
			return 3;
	}

	free(pageData);
	return 0;
}

RC RecordBasedFileManager::readRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, void *data) {

	// Retrieve the specific page.
	void * pageData = malloc(PAGE_SIZE);
	if (fileHandle.readPage(rid.pageNum, pageData) != SUCCESS)
		return 1;

	// Checks if the specific slot id exists in the page.
	SlotDirectoryHeader slotHeader = getSlotDirectoryHeader(pageData);
	if(slotHeader.recordEntriesNumber < rid.slotNum)
		return 2;

	// Gets the slot directory record entry data.
	SlotDirectoryRecordEntry recordEntry = getSlotDirectoryRecordEntry(pageData, rid.slotNum);

	// Retrieve the actual entry data.
	memcpy	((char*) data, ((char*) pageData + recordEntry.offset), recordEntry.length);

	free(pageData);
	return 0;
}

RC RecordBasedFileManager::printRecord(const vector<Attribute> &recordDescriptor, const void *data) {

	unsigned offset = 0;

	int data_integer;
	float data_real;
	unsigned stringLength;
	char * stringData;

	for (unsigned i = 0; i < (unsigned) recordDescriptor.size(); i++)
		switch (recordDescriptor[i].type)
		{
			case TypeInt:
				memcpy(&data_integer, ((char*) data + offset), INT_SIZE);
				offset += INT_SIZE;

				cout << "Attribute " << i << " (integer): " << data_integer << endl;
			break;
			case TypeReal:
				memcpy(&data_real, ((char*) data + offset), REAL_SIZE);
				offset += REAL_SIZE;

				cout << "Attribute " << i << " (real): " << data_real << endl;
			break;
			case TypeVarChar:
				// First VARCHAR_LENGTH_SIZE bytes describe the varchar length.
				memcpy(&stringLength, ((char*) data + offset), VARCHAR_LENGTH_SIZE);
				offset += VARCHAR_LENGTH_SIZE;

				// Gets the actual string.
				stringData = (char*) malloc(stringLength + 1);
				memcpy((void*) stringData, ((char*) data + offset), stringLength);
				// Adds the string terminator.
				stringData[stringLength] = '\0';
				offset += stringLength;

				cout << "Attribute " << i << " (string): " << stringData << endl;
				free(stringData);
			break;
		}

	return 0;
}
