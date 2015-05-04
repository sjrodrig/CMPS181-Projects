#include <iostream>
#include <string>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdexcept>
#include <stdio.h>
#include <algorithm>
#include <bitset>
#include "RecordBasedFile.h"

/**
 * @modifier: Benjamin (Benjy) Strauss
 * @modifier: Paul Mini
 * 
 */

/**
 * @Completed
 * Delete all records in a file.
 * The file should have "DELETED" at the beginning.
 */
int
RecordBasedFileManager::deleteRecords(FileHandle &fileHandle) {
	unsigned meta = 0;
	unsigned blankThis = fileHandle.getNumberOfPages();

	for(; meta == 0 && blankThis != 0; blankThis--) {
		meta = fileHandle.writePage(blankThis, 0);
	}
	// Write that zero entries are on the page, and then put the word "DELETED"
	if (fileHandle.writePage(0, "\0\0\0\0DELETED") != SUCCESS){
		return -1;
	}
	return SUCCESS;
}

/**
 * @Completed
 * Update a record identified by the given rid.
 * Return Values
 * X where X > 0: insert failed
 * -1 = can't read the page
 * 
 * -3 = can't write to the page
 *
 */
int
RecordBasedFileManager::deleteRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid) {
	// Assume the rid does not change after update

	// Allocate space for a new page and fetch data
	void* newPage = malloc(PAGE_SIZE);
	if(fileHandle.readPage(rid.pageNum, newPage) != SUCCESS){
		return -1;
	}

	// Fetch Slot Directory Header 
	SlotDirectoryHeader slot_header = getSlotDirectoryHeader(newPage);

	// Make sure the desired slot is reachable
	if (rid.slotNum > slot_header.recordEntriesNumber){
		return -2;
	}

	// Create Dead Entry and Insert it
	SlotDirectoryRecordEntry dead_entry;
	dead_entry.recordEntryType = Dead;
	setSlotDirectoryRecordEntry(newPage, rid.slotNum, dead_entry);

	// Write to the page and free memory allocated
	if (fileHandle.writePage(rid.pageNum, newPage) != SUCCESS){
		return -3;
	}
	free(newPage);
	return SUCCESS;
}

/**
 * @Completed
 * Update a record identified by the given rid.
 * Return Values
 * X where X > 0: insert failed
 * -1 = can't delete the page
 * -2 = can't insert the page
 * -3 = can't read to the page
 * -4 = can't write to the page
 */
int 
RecordBasedFileManager::updateRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, const RID &rid) {
	// Assume the rid does not change after update
	void* newPage = malloc(PAGE_SIZE);

	// Delete the old record
	if (deleteRecord(fileHandle, recordDescriptor, rid) != SUCCESS){
		return -1;
	}

	// Insert the updated record
	
	deleteRecord(fileHandle, recordDescriptor, rid);
	RID newRID;
	if (insertRecord(fileHandle, recordDescriptor, data, newRID) != SUCCESS){
		return -2;
	}

	if(newRID != rid) {
		if(fileHandle.readPage(rid.pageNum, newPage) != SUCCESS) {
			return -3;
		}

		// Get the info from the file
		SlotDirectoryRecordEntry mySDRE;
		
		mySDRE.recordEntryType = Tombstone;
		mySDRE.tombStoneRID = newRID;

		// Set the lot SlotDirectoryRecordEntry to the new
		setSlotDirectoryRecordEntry(newPage, rid.slotNum, mySDRE);

		if(fileHandle.writePage(rid.pageNum, newPage) != SUCCESS) {
			return -4;
		}
	}
	free(newPage);
	return SUCCESS;
}

// Read an attribute given its name and the rid.
int
RecordBasedFileManager::readAttribute(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, const string attributeName, void *data) {
	void* cur_record = malloc(PAGE_SIZE);
	if (readRecord(fileHandle, recordDescriptor, rid, cur_record) != SUCCESS){
		return -1;
	}

	int size_string = 0;
	bool attr_found = false;
	unsigned offset = 0;
	for (unsigned int i = 0; i < recordDescriptor.size() && !attr_found; i++) {
		Attribute cur_attribute = recordDescriptor[i];
		if (cur_attribute.name == attributeName) { attr_found = true; }
		switch(cur_attribute.type){
			case TypeInt:
				if (attr_found){
					memcpy(data, (char*)cur_record + offset, INT_SIZE);
				} 
				offset += INT_SIZE;
				break;
			case TypeReal:
				if (attr_found){
					memcpy(data, (char*)cur_record + offset, REAL_SIZE);
				} 
				offset += REAL_SIZE;
				break;
			case TypeVarChar:
				memcpy(&size_string, (char*)cur_record + offset, VARCHAR_LENGTH_SIZE);
				if (attr_found){
					memcpy(data, (char*)cur_record + offset, size_string + VARCHAR_LENGTH_SIZE);
				} 
				offset += size_string + VARCHAR_LENGTH_SIZE;
				break;
		}
		if (attr_found) break;
	}
	free(cur_record);
	if (!attr_found) { return -2; }
	return SUCCESS;
}



// scan returns an iterator to allow the caller to go through the results one by one. 
int
RecordBasedFileManager::scan(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const string &conditionAttribute, const CompOp compOp, const void *value, const vector<string> &attributeNames, RBFM_ScanIterator &rbfm_ScanIterator) {
	void *cur_page = malloc(PAGE_SIZE);
	SlotDirectoryHeader header;
	SlotDirectoryRecordEntry record_entry;
	vector<RID> rids;
	vector<void*> dataVector;

	for (unsigned i = 0; i < fileHandle.getNumberOfPages(); i++){

		if (fileHandle.readPage(i, cur_page) != SUCCESS){
			cout << "i = " << i << endl;
			cout << "fileHandle.getNumberOfPages() = " << fileHandle.getNumberOfPages() << endl;
			return -1;
		}
		header = getSlotDirectoryHeader(cur_page);
		for (unsigned j = 0; j < header.recordEntriesNumber; j++){
			record_entry = getSlotDirectoryRecordEntry(cur_page, j);
			if (record_entry.recordEntryType == Alive){
				void *cur_record = malloc(PAGE_SIZE);
				RID cur_RID;
				cur_RID.pageNum = i;
				cur_RID.slotNum = j;
				if (readRecord(fileHandle, recordDescriptor, cur_RID, cur_record) != SUCCESS){
					return -2;
				} else {
					void *result_record = malloc(PAGE_SIZE);
					int size_string = 0;
					unsigned read_offset = 0;
					unsigned write_offset = 0;
					bool data_found = false;
					for (unsigned int i = 0; i < recordDescriptor.size(); i++){
						Attribute cur_attribute = recordDescriptor[i];
						bool attr_found = (find(attributeNames.begin(), attributeNames.end(), recordDescriptor[i].name) != attributeNames.end());
						switch(cur_attribute.type){
							case TypeInt:
								if (attr_found){
									memcpy((char*)result_record + write_offset, (char*)cur_record + read_offset, INT_SIZE);
									write_offset += INT_SIZE;
									data_found = true;
								}
								read_offset += INT_SIZE;
								break;
							case TypeReal:
								if (attr_found){
									memcpy((char*)result_record + write_offset, (char*)cur_record + read_offset, REAL_SIZE);
									write_offset += REAL_SIZE;
									data_found = true; 
								} 
								read_offset += REAL_SIZE;
								break;
							case TypeVarChar:
								memcpy(&size_string, (char*)cur_record + read_offset, VARCHAR_LENGTH_SIZE);
								if (attr_found){
									memcpy((char*)result_record + write_offset, (char*)cur_record + read_offset, size_string + VARCHAR_LENGTH_SIZE);
									write_offset += size_string + VARCHAR_LENGTH_SIZE;
									data_found = true;
								} 
								read_offset += size_string + VARCHAR_LENGTH_SIZE;
								break;
						}
					}
					if (data_found){
						rids.push_back(cur_RID);
						dataVector.push_back(result_record);
					}
				}
				free(cur_record);
			}
		}
	}
	free(cur_page);
	return rbfm_ScanIterator.setVectors(rids, dataVector);
}

bool RecordBasedFileManager::checkScanCondition(int dataInt, CompOp compOp, const void * value)
{
	int valueInt;
	memcpy (&valueInt, value, REAL_SIZE);

	switch (compOp)
	{
		case EQ_OP:  // =
			return dataInt == valueInt;
		break;
		case LT_OP:  // <
			return dataInt < valueInt;
		break;
		case GT_OP:  // >
			return dataInt > valueInt;
		break;
		case LE_OP:  // <=
			return dataInt <= valueInt;
		break;
		case GE_OP:  // >=
			return dataInt >= valueInt;
		break;
		case NE_OP:  // !=
			return dataInt != valueInt;
		break;
		case NO_OP:  // no condition
			return true;
		break;
	}
	// We should never get here.
	return false;
}

bool RecordBasedFileManager::checkScanCondition(float dataFloat, CompOp compOp, const void * value)
{
	float valueFloat;
	memcpy (&valueFloat, value, REAL_SIZE);

	switch (compOp)
	{
		case EQ_OP:  // =
			return dataFloat == valueFloat;
		break;
		case LT_OP:  // <
			return dataFloat < valueFloat;
		break;
		case GT_OP:  // >
			return dataFloat > valueFloat;
		break;
		case LE_OP:  // <=
			return dataFloat <= valueFloat;
		break;
		case GE_OP:  // >=
			return dataFloat >= valueFloat;
		break;
		case NE_OP:  // !=
			return dataFloat != valueFloat;
		break;
		case NO_OP:  // no condition
			return true;
		break;
	}
	// We should never get here.
	return false;
}

bool RecordBasedFileManager::checkScanCondition(char* dataString, CompOp compOp, const void * value)
{
	char* valueString;
	memcpy (&valueString, value, REAL_SIZE);

	switch (compOp)
	{
		case EQ_OP:  // =
			return strcmp(dataString, valueString) == 0;
		break;
		case LT_OP:  // <
			return strcmp(dataString, valueString) < 0;
		break;
		case GT_OP:  // >
			return strcmp(dataString, valueString) > 0;
		break;
		case LE_OP:  // <=
			return (strcmp(dataString, valueString) < 0) || (strcmp(dataString, valueString) == 0);
		break;
		case GE_OP:  // >=
			return (strcmp(dataString, valueString) > 0) || (strcmp(dataString, valueString) == 0);
		break;
		case NE_OP:  // !=
			return strcmp(dataString, valueString) != 0;
		break;
		case NO_OP:  // no condition
			return true;
		break;
	}
	// We should never get here.
	return false;
}


// optional
// Push the free space towards the end of the page.
int
RecordBasedFileManager::reorganizePage(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const unsigned pageNumber) {

	return -1;
}

// optional
// Reorganize the records in the file such that the records are collected towards the beginning of the file.
int
RecordBasedFileManager::reorganizeFile(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor) {
	return -1;
}

int
RBFM_ScanIterator::setVectors(vector<RID> rids, vector<void*> dataVector){
	if (rids.size() != dataVector.size()) return -1;
	this->currentSize = rids.size();
	this->rids = rids;
	this->dataVector = dataVector;
	return SUCCESS;
}

/**
 * Return:
 * 0: more in list
 * -1: end of list
 */
int
RBFM_ScanIterator::getNextRecord(RID &rid, void *data) {
	if (currentPosition >= currentSize) { return -1; }
	rid = this->rids.at(currentPosition);
	data = this->dataVector.at(currentPosition);
	this->currentPosition++;
	return SUCCESS;
}

int
RBFM_ScanIterator::close() {
	for (auto it = begin(this->dataVector); it != end(this->dataVector); ++it) {
    	free(*it);
	}
	this->currentPosition = 0;
	this->currentSize = 0;
	return SUCCESS;
}

/*********************************************************************************
 *									From Project 1								 *
 *********************************************************************************/

RecordBasedFileManager* RecordBasedFileManager::_rbf_manager = 0;

RecordBasedFileManager* RecordBasedFileManager::instance() {
	if(!_rbf_manager) { _rbf_manager = new RecordBasedFileManager(); }
	return _rbf_manager;
}

RecordBasedFileManager::RecordBasedFileManager() {
	_pf_manager = PagedFileManager::instance();
}

RecordBasedFileManager::~RecordBasedFileManager() {

}

// Configures a new record based page, and puts it in "page".
void RecordBasedFileManager::newRecordBasedPage(void * page) {
	// Writes the slot directory header.
	SlotDirectoryHeader slotHeader;
	slotHeader.freeSpaceOffset = PAGE_SIZE;
	slotHeader.recordEntriesNumber = 0;
	setSlotDirectoryHeader(page, slotHeader);
}


int
RecordBasedFileManager::createFile(const string &fileName) {
    // Creating a new paged file.
	if (_pf_manager->createFile(fileName.c_str()) != SUCCESS) {
		return 1;
	}

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

int
RecordBasedFileManager::destroyFile(const string &fileName) {
    return _pf_manager->destroyFile(fileName.c_str());
}

int
RecordBasedFileManager::openFile(const string &fileName, FileHandle &fileHandle) {
    return _pf_manager->openFile(fileName.c_str(), fileHandle);
}

int
RecordBasedFileManager::closeFile(FileHandle &fileHandle) {
    return _pf_manager->closeFile(fileHandle);
}

SlotDirectoryHeader RecordBasedFileManager::getSlotDirectoryHeader(void * page)
{
	// Getting the slot directory header.
	SlotDirectoryHeader slotHeader;
	memcpy (&slotHeader, page, sizeof(SlotDirectoryHeader));
	return slotHeader;
}

void 
RecordBasedFileManager::setSlotDirectoryHeader(void * page, SlotDirectoryHeader slotHeader) {
	// Setting the slot directory header.
	memcpy (page, &slotHeader, sizeof(SlotDirectoryHeader));
}

SlotDirectoryRecordEntry RecordBasedFileManager::getSlotDirectoryRecordEntry(void * page, unsigned recordEntryNumber) {
	// Getting the slot directory entry data.
	SlotDirectoryRecordEntry recordEntry;
	memcpy	(
			&recordEntry,
			((char*) page + sizeof(SlotDirectoryHeader) + recordEntryNumber * sizeof(SlotDirectoryRecordEntry)),
			sizeof(SlotDirectoryRecordEntry)
			);

	return recordEntry;
}

void RecordBasedFileManager::setSlotDirectoryRecordEntry(void * page, unsigned recordEntryNumber, SlotDirectoryRecordEntry recordEntry) {
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

	for (unsigned i = 0; i < (unsigned) recordDescriptor.size(); i++) {;
		switch (recordDescriptor[i].type) {
			case TypeInt: {
				size += INT_SIZE;
//cout << "int+" << endl;
			break; }
			case TypeReal: {
				size += REAL_SIZE;
//cout << "rl+" << endl;
			break; }
			case TypeVarChar: {
				// We have to get the size of the VarChar field by reading the integer that precedes the string value itself.
				memcpy(&varcharSize, (char*) data + size, VARCHAR_LENGTH_SIZE);
				// We also have to account for the overhead given by that integer.
				//cout << "varcharSize: " << varcharSize << endl;
				size += INT_SIZE + varcharSize;
			break; }
		}
	}

	return size;
}

int
RecordBasedFileManager::insertRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, RID &rid) {
    // Gets the size of the record.
	unsigned recordSize = getRecordSize(recordDescriptor, data);

	// Cycles through pages looking for enough free space for the new entry.
	void * pageData = malloc(PAGE_SIZE);

	bool pageFound = false;
	unsigned i;

	for (i = 0; i < fileHandle.getNumberOfPages(); i++) {
		if (fileHandle.readPage(i, pageData) != SUCCESS) {
			return -1;
		}

		/* When we find a page with enough space
		 * (accounting also for the size that will be added to the slot directory),
		 * we stop the loop.
		 */
		if (getPageFreeSpaceSize(pageData) >= sizeof(SlotDirectoryRecordEntry) + recordSize) {
			pageFound = true;
			break;
		}
	}

	if(!pageFound) {
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
	newRecordEntry.recordEntryType = Alive;
	newRecordEntry.length = recordSize;
	newRecordEntry.recordEntryType = Alive;
	newRecordEntry.offset = slotHeader.freeSpaceOffset - recordSize;
	setSlotDirectoryRecordEntry(pageData, rid.slotNum, newRecordEntry);

	// Updating the slot directory header.
	slotHeader.freeSpaceOffset = newRecordEntry.offset;
	slotHeader.recordEntriesNumber += 1;
	setSlotDirectoryHeader(pageData, slotHeader);

	// Adding the record data.
	if (!memcpy(((char*) pageData + newRecordEntry.offset), data, recordSize) > 0){
		cout << "ERROR: Failed to insert record data." << endl;
		free(pageData);
		return -2;
	}

	// Writing the page to disk.
	if (pageFound){
		if (fileHandle.writePage(i, pageData) != SUCCESS) {
			return -3;
		}
	} else {
		if (fileHandle.appendPage(pageData) != SUCCESS) {
			return -4;
		}
	}
	free(pageData);
	return SUCCESS;
}

int
RecordBasedFileManager::readRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, void *data) {
    
	// Retrieve the specific page.
	void * pageData = malloc(PAGE_SIZE);
	if (fileHandle.readPage(rid.pageNum, pageData) != SUCCESS) {
		return 1;
	}

	// Checks if the specific slot id exists in the page.
	SlotDirectoryHeader slotHeader = getSlotDirectoryHeader(pageData);
	if(slotHeader.recordEntriesNumber < rid.slotNum) {
		return 2;
	}

//cout << "-0-" << endl;

	// Gets the slot directory record entry data.
	SlotDirectoryRecordEntry recordEntry = getSlotDirectoryRecordEntry(pageData, rid.slotNum);

//cout << "-1-" << endl;

	// Cycle through any tombstones
	RID old_rid = rid;
	while(recordEntry.recordEntryType == Tombstone){
		RID new_rid = recordEntry.tombStoneRID;
		if (old_rid.pageNum != new_rid.pageNum){
			if (fileHandle.readPage(new_rid.pageNum, pageData) != SUCCESS){
				return 3;
			}
			slotHeader = getSlotDirectoryHeader(pageData);
		}
		if (slotHeader.recordEntriesNumber < new_rid.slotNum){
			return 4;
		}
		recordEntry = getSlotDirectoryRecordEntry(pageData, new_rid.slotNum);
		old_rid = new_rid;
	}

//cout << "-2-" << endl;

	if (recordEntry.recordEntryType == Dead) {
		return 5;
	}


cout << "-3-" << endl;
cout << "Diagnostics" << endl;
cout << "recordEntry.offset: " << recordEntry.offset << endl;
cout << "recordEntry.length: " << recordEntry.length << endl;
cout << "sizeof data: " << sizeof( data ) << endl;
cout << "data: " << (unsigned char*) data << endl;
cout << "sizeof pageData: " << sizeof( pageData ) << endl;
cout << "pageData: " << (unsigned char*) pageData << endl;

cout << "sizeof pageData + offset: " << sizeof((char*) pageData + recordEntry.offset) << endl;

	// Retrieve the actual entry data.
	memcpy	((char*) data, ((char*) pageData + recordEntry.offset), recordEntry.length);

//cout << "-4-" << endl;

	free(pageData);
	return 0;
}

int
RecordBasedFileManager::printRecord(const vector<Attribute> &recordDescriptor, const void *data) {
	unsigned offset = 0;
	int data_integer;
	float data_real;
	unsigned stringLength;
	char * stringData;

	for (unsigned i = 0; i < (unsigned) recordDescriptor.size(); i++)
		switch (recordDescriptor[i].type) {
			case TypeInt: {
				memcpy(&data_integer, ((char*) data + offset), INT_SIZE);
				offset += INT_SIZE;

				cout << "Attribute " << i << " (integer): " << data_integer << endl;
			break; }
			case TypeReal: {
				memcpy(&data_real, ((char*) data + offset), REAL_SIZE);
				offset += REAL_SIZE;

				cout << "Attribute " << i << " (real): " << data_real << endl;
			break; }
			case TypeVarChar: {
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
			break; }
		}

	return 0;
}
