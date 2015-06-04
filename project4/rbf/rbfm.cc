// CMPS 181 - Project 2
// Author:				Paolo Di Febbo
// File description:	Implementing the "Variable length records" page structure
//						(ref. p. 329 Ramakrishnan - Gehrke, with some modifications).

#include <iostream>
#include <string>
#include <sys/stat.h>
#include <string.h>
#include <stdexcept>
#include <stdio.h>
#include <algorithm>

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

// Configures a new record based page, and puts it in the "page" argument.
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
				size += VARCHAR_LENGTH_SIZE + varcharSize;
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
		// If we are here, there are no pages with enough space. In this case, we just create a new white page.
		// N.B.: In this case we will need to append it (see code near the end of the method).
		newRecordBasedPage(pageData);
	}

	SlotDirectoryHeader slotHeader = getSlotDirectoryHeader(pageData);

	// Return RID: setting the page number.
	rid.pageNum = i;

	// Return RID: slot used is the first one empty ("Dead" = record was deleted), or a new one.
	SlotDirectoryRecordEntry currentEntry;
	bool foundDeadRecordEntry = false;
	unsigned k;
	for (k = 0; k < slotHeader.recordEntriesNumber; k++)
	{
		currentEntry = getSlotDirectoryRecordEntry(pageData, k);
		if (currentEntry.recordEntryType == Dead)
		{
			foundDeadRecordEntry = true;
			break;
		}
	}
	rid.slotNum = k;

	// Adding the new record entry in the slot directory.
	SlotDirectoryRecordEntry newRecordEntry;
	newRecordEntry.recordEntryType = Alive;
	newRecordEntry.length = recordSize;
	newRecordEntry.offset = slotHeader.freeSpaceOffset - recordSize;
	setSlotDirectoryRecordEntry(pageData, rid.slotNum, newRecordEntry);

	// Updating the slot directory header.
	slotHeader.freeSpaceOffset = newRecordEntry.offset;
	if (!foundDeadRecordEntry)
		slotHeader.recordEntriesNumber += 1;
	setSlotDirectoryHeader(pageData, slotHeader);

	// Adding the record data.
	memcpy	(((char*) pageData + newRecordEntry.offset), data, recordSize);

	// Writing the page to disk.
	if (pageFound)
	{
		if (fileHandle.writePage(rid.pageNum, pageData) != SUCCESS)
			return 2;
	}
	else
	{
		// In this case (page with enough space not found), we need to append the new one.
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
	if(slotHeader.recordEntriesNumber <= rid.slotNum)
		return 2;

	// Gets the slot directory record entry data.
	SlotDirectoryRecordEntry recordEntry = getSlotDirectoryRecordEntry(pageData, rid.slotNum);

	switch (recordEntry.recordEntryType)
	{
		// Checks if the given RID is dead. If so, error.
		case Dead:
			free(pageData);
			return 3;
		break;

		// Checks if the given RID is a tombstone. If so, starts a new recursive read to the linked RID.
		case Tombstone:
			free(pageData);
			return readRecord(fileHandle, recordDescriptor, recordEntry.tombStoneRID, data);
		break;

		case Alive:
			// Retrieve the actual entry data.
			memcpy	((char*) data, ((char*) pageData + recordEntry.offset), recordEntry.length);

			free(pageData);
			return 0;
		break;
	}

	// We should never be here.
	return 4;
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

				cout << "Attribute " << recordDescriptor[i].name << " (integer): " << data_integer << endl;
			break;
			case TypeReal:
				memcpy(&data_real, ((char*) data + offset), REAL_SIZE);
				offset += REAL_SIZE;

				cout << "Attribute " << recordDescriptor[i].name << " (real): " << data_real << endl;
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

				cout << "Attribute " << recordDescriptor[i].name << " (string): " << stringData << endl;
				free(stringData);
			break;
		}

	return 0;
}

RC RecordBasedFileManager::deleteRecords(FileHandle &fileHandle)
{
	// Brutal approach: reset every page with a new "white" one.

	void * page = malloc(PAGE_SIZE);

	unsigned i;
	for (i = 0; i < fileHandle.getNumberOfPages(); i++)
	{
		if (fileHandle.readPage(i, page) != SUCCESS)
			return 1;

		newRecordBasedPage(page);

		if (fileHandle.writePage(i, page) != SUCCESS)
			return 2;
	}

	free(page);
	return 0;
}

RC RecordBasedFileManager::deleteRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid)
{
	// Retrieve the specific page.
	void * pageData = malloc(PAGE_SIZE);
	if (fileHandle.readPage(rid.pageNum, pageData) != SUCCESS)
		return 1;

	// Checks if the specific slot id exists in the page.
	SlotDirectoryHeader slotHeader = getSlotDirectoryHeader(pageData);
	if(slotHeader.recordEntriesNumber <= rid.slotNum)
		return 2;

	// TODO: To be more efficient, here some recursive Tombstone deletion should be added.

	// Overwrites the specific record entry with a "Dead" one.
	SlotDirectoryRecordEntry deadRecordEntry;
	deadRecordEntry.recordEntryType = Dead;
	setSlotDirectoryRecordEntry(pageData, rid.slotNum, deadRecordEntry);

	// Writeback.
	if (fileHandle.writePage(rid.pageNum, pageData) != SUCCESS)
		return 4;

	free(pageData);
	return 0;
}

RC RecordBasedFileManager::updateRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, const RID &rid)
{
	// Aggressive approach: update = delete + insert [+ tombstone link]
	// NB: In a real reliable system, this method should be a transaction. If something goes wrong within it, I should recover everything back as it was before the call.

	if (deleteRecord(fileHandle, recordDescriptor, rid) != SUCCESS)
		return 1;

	RID newRid;
	if (insertRecord(fileHandle, recordDescriptor, data, newRid) != SUCCESS)
		return 2;

	// If the new rid is different, I put a tombstone on the original record entry linking to it.
	if (newRid != rid)
	{
		// Retrieve the specific page.
		void * pageData = malloc(PAGE_SIZE);
		if (fileHandle.readPage(rid.pageNum, pageData) != SUCCESS)
			return 3;

		// Puts the tombstone.
		SlotDirectoryRecordEntry tombstoneRecordEntry;
		tombstoneRecordEntry.recordEntryType = Tombstone;
		tombstoneRecordEntry.tombStoneRID = newRid;
		setSlotDirectoryRecordEntry(pageData, rid.slotNum, tombstoneRecordEntry);

		// Writeback.
		if (fileHandle.writePage(rid.pageNum, pageData) != SUCCESS)
			return 4;

		free(pageData);
	}

	return 0;
}

RC RecordBasedFileManager::readAttribute(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, const string attributeName, void *data)
{
	// Reading the full record data given its RID.
	// I do not know how much the read record size will be, so I allocate the whole page size.
	void * readRecordData = malloc(PAGE_SIZE);
	if (readRecord(fileHandle, recordDescriptor, rid, readRecordData) != SUCCESS)
		return 1;

	// Reading the specific attribute.
	unsigned offset = 0;
	unsigned stringLength;
	bool attributeFound = false;

	for (unsigned i = 0; i < (unsigned) recordDescriptor.size() && !attributeFound; i++)
	{
		if (recordDescriptor[i].name.compare(attributeName) == 0)
			attributeFound = true;

		switch (recordDescriptor[i].type)
		{
			case TypeInt:
				if (attributeFound)
					memcpy(data, (char*) readRecordData + offset, INT_SIZE);
				else
					offset += INT_SIZE;
			break;
			case TypeReal:
				if (attributeFound)
					memcpy(data, (char*) readRecordData + offset, REAL_SIZE);
				else
					offset += REAL_SIZE;
			break;
			case TypeVarChar:
				// We have to get the size of the string by reading the integer that precedes the string value itself.
				memcpy(&stringLength, (char*) readRecordData + offset, VARCHAR_LENGTH_SIZE);
				offset += VARCHAR_LENGTH_SIZE;

				if (attributeFound)
				{
					memcpy(data, (char*) readRecordData + offset, stringLength);
					// We also need to add the string terminator.
					((char*) data)[stringLength] = '\0';
				}
				else
					offset += stringLength;
			break;
		}
	}

	free(readRecordData);
	return attributeFound ? 0 : 2;
}

// Used for sorting record entries (by offset DESC) in the reorganizePage method.
bool RecordBasedFileManager::sortLabeledRecordEntriesByOffsetDescComparer(const LabeledSlotDirectoryRecordEntry &recordEntry1, const LabeledSlotDirectoryRecordEntry &recordEntry2)
{
  return recordEntry1.recordEntry.offset > recordEntry2.recordEntry.offset;
}

RC RecordBasedFileManager::reorganizePage(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const unsigned pageNumber)
{
	// Reads the page.
	void * pageData = malloc(PAGE_SIZE);
	if (fileHandle.readPage(pageNumber, pageData) != SUCCESS)
		return 1;

	// Gets all the "alive" record entries.

	SlotDirectoryHeader header = getSlotDirectoryHeader(pageData);

	if (header.recordEntriesNumber == 0)
		return 0;

	vector<LabeledSlotDirectoryRecordEntry> aliveRecordEntries;
	for (unsigned i = 0; i < header.recordEntriesNumber; i++)
	{
		LabeledSlotDirectoryRecordEntry labeledRecordEntry;
		labeledRecordEntry.slotNum = i;
		labeledRecordEntry.recordEntry = getSlotDirectoryRecordEntry(pageData, i);

		if (labeledRecordEntry.recordEntry.recordEntryType == Alive)
			aliveRecordEntries.push_back(labeledRecordEntry);
	}

	// Sort the alive record entries by offset DESC (i.e. physical order in the page file).
	sort(aliveRecordEntries.begin(), aliveRecordEntries.end(), RecordBasedFileManager::sortLabeledRecordEntriesByOffsetDescComparer);

	// For each alive entry, pushes it back in order to be placed contiguous to the previous one.
	unsigned pageOffset = PAGE_SIZE;
	for(unsigned k = 0; k != (unsigned) aliveRecordEntries.size(); k++)
	{
		// Moving the current offset.
		pageOffset -= aliveRecordEntries[k].recordEntry.length;

		// Moving the actual record data in the new position.
		memmove ((char *) pageData + pageOffset, (char *) pageData + aliveRecordEntries[k].recordEntry.offset, aliveRecordEntries[k].recordEntry.length);

		// Updating the slot directory record entry with the new offset.
		aliveRecordEntries[k].recordEntry.offset = pageOffset;
		setSlotDirectoryRecordEntry(pageData, aliveRecordEntries[k].slotNum, aliveRecordEntries[k].recordEntry);
	}

	// Writeback.
	if (fileHandle.writePage(pageNumber, pageData) != SUCCESS)
		return 2;

	free(pageData);
	return 0;
}

// checkScanCondition methods are used only within the "scan" method (see its code below).

bool RecordBasedFileManager::checkScanCondition(int dataInt, CompOp compOp, const void * value)
{
	// Checking a condition on an integer is the same as checking it on a float with the same value.
	int valueInt;
	memcpy (&valueInt, value, INT_SIZE);
	float convertedInt = (float)valueInt;

	return RecordBasedFileManager::checkScanCondition((float) dataInt, compOp, &convertedInt);
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

bool RecordBasedFileManager::checkScanCondition(char * dataString, CompOp compOp, const void * value)
{
	switch (compOp)
	{
		case EQ_OP:  // =
			return strcmp(dataString, (char*) value) == 0;
		break;
		case LT_OP:  // <
			return strcmp(dataString, (char*) value) < 0;
		break;
		case GT_OP:  // >
			return strcmp(dataString, (char*) value) > 0;
		break;
		case LE_OP:  // <=
			return strcmp(dataString, (char*) value) <= 0;
		break;
		case GE_OP:  // >=
			return strcmp(dataString, (char*) value) >= 0;
		break;
		case NE_OP:  // !=
			return strcmp(dataString, (char*) value) != 0;
		break;
		case NO_OP:  // no condition
			return true;
		break;
	}

	// We should never get here.
	return false;
}

// N.B.: This implementation of the scan method is probably the fastest, because it works directly on the page data.
// But it has a limitation (not a problem, since this is not specified in the project description): projected attributes are sorted in the same order as they appear in the recordDescriptor.
RC RecordBasedFileManager::scan(FileHandle &fileHandle,
      const vector<Attribute> &recordDescriptor,
      const string &conditionAttribute,
      const CompOp compOp,                  // comparision type such as "<" and "="
      const void *value,                    // used in the comparison
      const vector<string> &attributeNames, // a list of projected attributes
      RBFM_ScanIterator &rbfm_ScanIterator)
{

	void * recordData;				// Retrieved complete record data.
	int recordDataOffset;
	void * recordOutputData;		// Data output considering the attributeNames filter.
	int recordOutputDataOffset;

	// Attribute-specific data.
	int dataInteger;
	float dataReal;
	unsigned stringLength;
	char * dataString;

	// Initialize the vectors that will be returned through the RBFM_ScanIterator.
	vector<RID> rids;
	vector<int> dataVectorSizes;
	vector<void*> dataVector;

	// Cycles through all the pages.
	void * pageData = malloc(PAGE_SIZE);
	SlotDirectoryHeader header;
	SlotDirectoryRecordEntry recordEntry;
	for (unsigned i = 0; i < fileHandle.getNumberOfPages(); i++)
	{
		if (fileHandle.readPage(i, pageData) != SUCCESS)
			return 1;

		// Cycles through all the "alive" record entries within the page.
		header = getSlotDirectoryHeader(pageData);
		for (unsigned j = 0; j < header.recordEntriesNumber; j++)
		{
			recordEntry = getSlotDirectoryRecordEntry(pageData, j);
			if (recordEntry.recordEntryType == Alive)
			{
				// Retrieve the actual record data.
				recordData = malloc(recordEntry.length);
				recordDataOffset = 0;
				memcpy	((char*) recordData, ((char*) pageData + recordEntry.offset), recordEntry.length);

				// Set up the record output data, which will contain the projected record entry in function of attributeNames.
				recordOutputData = malloc(recordEntry.length);
				recordOutputDataOffset = 0;

				// For every attribute of the record, in function of its type we basically do the same 3 operations:
				// 1. Copy the attribute value in a local variable
				// 2. Check if we have to exclude this whole record entry according to the scan condition.
				// 3. If the attribute is requested in the output projection, append it into recordOutputData variable.
				bool excludeThisRecord = false;
				for (unsigned k = 0; k < (unsigned) recordDescriptor.size() && !excludeThisRecord; k++)
				{
					// An attribute is projected into the output data if its name appears in the attributeNames string vector.
					bool includeThisAttribute = (find(attributeNames.begin(), attributeNames.end(), recordDescriptor[k].name) != attributeNames.end());

					// Checks if this attribute is the condition attribute.
					bool isConditionAttribute = (recordDescriptor[k].name.compare(conditionAttribute) == 0);

					switch (recordDescriptor[k].type)
					{
						case TypeInt:
							// 1.
							memcpy(&dataInteger, (char*) recordData + recordDataOffset, INT_SIZE);

							// 2.
							excludeThisRecord = isConditionAttribute && !checkScanCondition(dataInteger, compOp, value);

							// 3.
							if (includeThisAttribute)
							{
								memcpy((char*) recordOutputData + recordOutputDataOffset, (char*) recordData + recordDataOffset, INT_SIZE);
								recordOutputDataOffset += INT_SIZE;
							}

							recordDataOffset += INT_SIZE;
						break;
						case TypeReal:
							// 1.
							memcpy(&dataReal, (char*) recordData + recordDataOffset, REAL_SIZE);

							// 2.
							excludeThisRecord = isConditionAttribute && !checkScanCondition(dataReal, compOp, value);

							// 3.
							if (includeThisAttribute)
							{
								memcpy((char*) recordOutputData + recordOutputDataOffset, (char*) recordData + recordDataOffset, REAL_SIZE);
								recordOutputDataOffset += REAL_SIZE;
							}

							recordDataOffset += REAL_SIZE;
						break;
						case TypeVarChar:
							// 1.
							// We need to get the size of the string by reading the integer that precedes the string value itself.
							memcpy(&stringLength, (char*) recordData + recordDataOffset, VARCHAR_LENGTH_SIZE);

							// Then, we get the string and add the terminator.
							dataString = (char*) malloc(stringLength + 1);
							memcpy(dataString, (char*) recordData + recordDataOffset + VARCHAR_LENGTH_SIZE, stringLength);
							dataString[stringLength] = '\0';

							// 2.
							excludeThisRecord = isConditionAttribute && !checkScanCondition(dataString, compOp, value);

							// 3.
							if (includeThisAttribute)
							{
								memcpy((char*) recordOutputData + recordOutputDataOffset, (char*) recordData + recordDataOffset, VARCHAR_LENGTH_SIZE + stringLength);
								recordOutputDataOffset += VARCHAR_LENGTH_SIZE + stringLength;
							}

							recordDataOffset += VARCHAR_LENGTH_SIZE + stringLength;
						break;
					}
				}

				// If this record is not excluded by the condition, include it in the output result vector.
				if (!excludeThisRecord)
				{
					RID returnRid;
					returnRid.pageNum = i;
					returnRid.slotNum = j;
					rids.push_back(returnRid);

					dataVectorSizes.push_back(recordOutputDataOffset);
					dataVector.push_back(recordOutputData);
				}
				else
					free(recordOutputData);

				free (recordData);
			}
		}
	}

	// Returns the result set through the iterator.
	rbfm_ScanIterator.setVectors(rids, dataVectorSizes, dataVector);

	free(pageData);
	return 0;
}

// NOT MANDATORY. TODO: Implement it?
RC RecordBasedFileManager::reorganizeFile(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor)
{
	return -1;
}
