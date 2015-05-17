#include <iostream>
#include <string.h>
#include <stdexcept>
#include <stdio.h>
#include <algorithm>
#include <fstream>
#include <cstring>
#include <bitset>
#include "IndexManager.h"

#define DEFAULT_ROOT_LOCATION 1

/**
 * CMPS 181 - Project 3
 * Author: Paolo Di Febbo
 * Modifier: Benjy Strauss
 * File description: Implementing a B+ tree indexing system.
 *						(ref. p. 344-351 Ramakrishnan - Gehrke)
 * 
 * METHOD PROGRESS
 * insertNonLeafRecord		--	Moderately Tested
 * insertLeafRecord			--	unstarted
 * insert					--	unstarted
 * deleteEntryFromLeaf		--	unstarted
 * deleteEntry				--	unstarted
 * treeSearch				--	Completed using the TA's code provided on Piazza
 * scan						--	unstarted
 * getKeyLength				--	Moderately Tested
 * getSonPageID				--	Completed but Untested
 * createFile				--	Lightly Tested
 * compareKeys				--  Moderately Tested
 */

using namespace std;

/**
 * Given a ChildEntry structure (<key, child page id>), writes it into the correct position within the non leaf page "pageData".
 * Need to put it in order...  
 * going to assume pageData has a size of 4096
 * 
 * 0 = success
 * -1 = page is full
 * 
 * ChildEntry PAIRS START AT INDEX 16
 *
 *
 * BROKEN -- data entry not being put in the child
 */
int
IndexManager::insertNonLeafRecord(const Attribute &attribute, ChildEntry &newChildEntry, void* pageData) {
	int retVal = -1;
	bool inserted = false;

	unsigned char* metaPage = new unsigned char[PAGE_SIZE];
	memcpy(metaPage, pageData, PAGE_SIZE);

	unsigned char bottomBit = metaPage[8];
	unsigned char lowerBit = metaPage[9];
	unsigned char upperBit = metaPage[10];
	unsigned char topBit = metaPage[11];

	int freeSpaceStart = 0;
	freeSpaceStart = bottomBit;
	freeSpaceStart += (lowerBit * 256);
	freeSpaceStart += (upperBit * 65536);
	freeSpaceStart += (topBit * 16777216);

	void* keyPointer = newChildEntry.key;
	int childSize = getKeyLength(attribute, keyPointer) + 4;
	int netSpace = childSize + freeSpaceStart;

	//if page is full, return an error
	if(netSpace >= PAGE_SIZE) { return retVal; }

	unsigned char* newChildEntryData = new unsigned char[childSize];
	unsigned char* oldChildEntryData = new unsigned char[childSize];

	cout << "newChildEntry.childPageNumber: " << newChildEntry.childPageNumber << endl;

	//copy the child into an array of unsigned chars
	memcpy(newChildEntryData, newChildEntry.key, childSize-4);
	newChildEntryData[childSize-4] = newChildEntry.childPageNumber % 256;
	newChildEntryData[childSize-3] = (newChildEntry.childPageNumber % 65536) / 256;
	newChildEntryData[childSize-2] = (newChildEntry.childPageNumber % 16777216) / 65536;
	newChildEntryData[childSize-1] = newChildEntry.childPageNumber / 16777216;

	for(int ucIndex = 0; ucIndex < childSize; ucIndex++) {
		cout << "ucIndex: " << ucIndex << endl;
		cout << (bitset<8>) newChildEntryData[ucIndex] << endl;
	}

	int keyIndex;
	for(keyIndex = 16; keyIndex < freeSpaceStart; keyIndex += childSize) {

		//load the key from the file into "oldChildEntryData" to compare it
		for(int loadIndex = 0; loadIndex < childSize; loadIndex++) {
			oldChildEntryData[loadIndex] = metaPage[keyIndex+loadIndex];
		}

		unsigned char* newChildEntryKey = new unsigned char[childSize-4];
		unsigned char* oldChildEntryKey = new unsigned char[childSize-4];
		memcpy(newChildEntryKey, newChildEntryData, childSize-4);
		memcpy(oldChildEntryKey, oldChildEntryData, childSize-4);

		/**
		 * compare the keys
		 * since they both have the childEntryData in the same place,
		 * it should be irrelevant since this data is the same
		 * (they're the same attribute)
		 */
		if(compareKeys(attribute, newChildEntryKey, oldChildEntryKey) < 0) {
			continue;
		} else { //insert the key

			//compute the amount of memory to move
			int moveBytes = freeSpaceStart - keyIndex;
			int moveOffset = keyIndex + childSize;

			//first move memory
			memmove(metaPage+moveOffset, metaPage+keyIndex, moveBytes);

			//do the copying of the data
			for(int ucIndex = 0; ucIndex < childSize; ucIndex++) {
				metaPage[ucIndex+keyIndex] = newChildEntryData[ucIndex];
			}
			inserted = true;
		}
	}

	//if the key is the largest, so it wasn't inserted
	if(keyIndex >= freeSpaceStart) {
		//write the newChildEntry onto the page
		for(int ucIndex = freeSpaceStart; ucIndex < netSpace; ucIndex++) {

			metaPage[ucIndex] = newChildEntryData[ucIndex-freeSpaceStart];
		}
	}

	//Update the place where the free space starts and write it back

	//increase free space offset by length inserted
	unsigned char bottomByte = netSpace % 256;
	unsigned char lowerByte = (netSpace % 65536) / 256;
	unsigned char upperByte = (netSpace % 16777216) / 65536;
	unsigned char topByte = netSpace / 16777216;

	metaPage[8] = bottomByte;
	metaPage[9] = lowerByte;
	metaPage[10] = upperByte;
	metaPage[11] = topByte;

	//Update the number of records on the page and write it back
	//bytes 4-7
	int recordsOnPage = 0;
	int mult = 1;
	for(int index = 4; index < 8; index++) {
		recordsOnPage += metaPage[index] * mult;
		mult *= 256;
	}

	//increment
	recordsOnPage++;
	for(int index = 4, mult = 1; index < 8; index++) {
		metaPage[index] = recordsOnPage / mult;
		mult *= 256;
	}

	//copy back the modified memory
	memcpy(pageData, metaPage, PAGE_SIZE);
	debugTool.fprintNBytes("after_insert.dump", metaPage, PAGE_SIZE);

	delete[] newChildEntryData;
	delete[] metaPage;
	retVal = SUCCESS;

	return retVal;
}

// Given a record entry (<key, RID>), writes it into the correct position within the leaf page "pageData".
int
IndexManager::insertLeafRecord(const Attribute &attribute, const void *key, const RID &rid, void* pageData) {
	return -1;
}

/**
 * Recursive insert of the record <key, rid> into the (current) page "pageID".
 * newChildEntry will store the return information of the "child" insert call.
 * Following the exact implementation described in Ramakrishnan - Gehrke, p.349.
 */
int
IndexManager::insert(const Attribute &attribute, const void *key, const RID &rid, FileHandle &fileHandle, unsigned pageID, ChildEntry &newChildEntry) {
	return -1;
}



// Given a record entry <key, rid>, deletes it from the leaf page "pageData".
int
IndexManager::deleteEntryFromLeaf(const Attribute &attribute, const void *key, const RID &rid, void * pageData) {
	return -1;
}

int
IndexManager::deleteEntry(FileHandle &fileHandle, const Attribute &attribute, const void *key, const RID &rid) {
	// Simplification: Lazy deletion.

	return -1;
}

// Recursive search through the tree, returning the page ID of the leaf page that should contain the input key.
int
IndexManager::treeSearch(FileHandle &fileHandle, const Attribute attribute, const void * key, unsigned currentPageID, unsigned &returnPageID) {
	void * pageData = malloc(PAGE_SIZE);

	// Gets the current page.
	if (fileHandle.readPage(currentPageID, pageData) != SUCCESS) {
		return ERROR_PFM_READPAGE;
	}

	// If it's a leaf page, we're done. Returns its id.
	if (isLeafPage(pageData)) {
		returnPageID = currentPageID;
		free(pageData);
		return SUCCESS;
	}

	// Otherwise, we go one level below (towards the correct son page) and call the method again.
	unsigned sonPageID = getSonPageID(attribute, key, pageData);

	free(pageData);

	return treeSearch(fileHandle, attribute, key, sonPageID, returnPageID);
}

int
IndexManager::scan(FileHandle &fileHandle, const Attribute &attribute, const void *lowKey, const void *highKey, bool lowKeyInclusive, bool highKeyInclusive, IX_ScanIterator &ix_ScanIterator) {
	return -1;
}

/**
 * should work...
 * feels like it's missing something though
 */
unsigned
IndexManager::getKeyLength(const Attribute &attribute, const void * key) {
	return attribute.length;
}

/**
 * Given a non-leaf page and a key, finds the correct (direct) son page ID in which the key "fits".
 *
 * As the method name says, you are returning only the page ID (unsigned). It should receive a non-leaf page and a key as an argument, scan through all the records within the page and stop when your input key is lower than the current record key: by doing this, you can get/return the correct pointer ("son" page ID) that refers to the son page in which your input key should fit.
 *
 *
 *
 */
unsigned
IndexManager::getSonPageID(const Attribute attribute, const void * key, void * pageData) {
	unsigned retVal = 0;

	//get the page as unsigned chars
	unsigned char* ucPage = new unsigned char[PAGE_SIZE];
	memcpy(ucPage, pageData, PAGE_SIZE);

	//the length of the key in bytes
	int keyLen = getKeyLength(attribute,key);
	int valSize = keyLen + 4; //take the unsigned int into account

	for(int keyIndex = 16; true; keyIndex += valSize) {
		unsigned char* oldChildEntryKey = new unsigned char[keyLen];

		int loadIndex;
		//load the key from the file into "oldChildEntryData" to compare it
		for(loadIndex = 0; loadIndex < keyLen; loadIndex++) {
			oldChildEntryKey[loadIndex] = ucPage[keyIndex+loadIndex];
		}

		if(compareKeys(attribute, key, oldChildEntryKey) <= 0) {
			continue;
		} else { //read the value

			int mult = 1;
			for(int aa = 0; aa < 4; aa++) {
				retVal += (mult * ucPage[keyIndex+loadIndex]);
				loadIndex++;
				mult *= 256;
			}
			return retVal;
		}
	}
	return retVal;
}

/**
 * Page 0 is neither a leaf nor a branch
 * Page 0 contains ONE four-byte number, which is the location of the root page.
 * 
 * This method creates a file with a root pointer, root, and a single leaf
 * 
 * untested -- in beta
 */
int
IndexManager::createFile(const string &fileName) {
	FileHandle handle;
	int retVal = -1;
	retVal = _pf_manager->createFile(fileName.c_str());
	//if we couldn't create the file, return.
	if(retVal != SUCCESS) {
		cout << "Could not create file: \"" << fileName << "\"" << endl;
		cout << "Returning on: " << retVal << endl;
		return retVal;
	}

	retVal = _pf_manager->openFile(fileName.c_str(), handle);
	if(retVal != SUCCESS) {
		cout << "Could not open newly created file: \"" << fileName << "\"" << endl;
		return retVal;
	}

	//write the location of the root on the first page
	unsigned* defaultRootLocation = new unsigned[1];
	defaultRootLocation[0] = DEFAULT_ROOT_LOCATION;
	handle.appendPage(defaultRootLocation);

	//write the root on page 1
	unsigned char* rootPage = new unsigned char[PAGE_SIZE];

	//set the page's type to non-leaf
	setPageType(rootPage, NonLeafPage);

	//write the header
	NonLeafPageHeader rootHeader;
	rootHeader.recordsNumber = 0;
	rootHeader.freeSpaceOffset = (sizeof(PageType) + sizeof(NonLeafPageHeader) + sizeof(unsigned));
	setNonLeafPageHeader(rootPage, rootHeader);

	unsigned linkOffset = rootHeader.freeSpaceOffset - sizeof(unsigned);

	//write the first leaf number
	rootPage[linkOffset] = 0x2;

	/*unsigned char meta0;
	for(int aa = 0; aa < 32; aa++) {
		meta0 = rootPage[aa];
		cout << "[" << aa << "]";
 		cout << (bitset<8>) meta0;
		if ((aa+1) % 4 == 0) { cout << endl; }
	}*/

	handle.appendPage(rootPage);
	debugTool.fprintNBytes("meta.dump", rootPage, PAGE_SIZE);

	//write the first non-leaf page
	unsigned char* firstLeaf = new unsigned char[PAGE_SIZE];
	setPageType(firstLeaf, LeafPage);

	LeafPageHeader leafHeader1;
	leafHeader1.prevPage = NULL_PAGE_ID;
	leafHeader1.nextPage = NULL_PAGE_ID;
	leafHeader1.recordsNumber = 0;
	leafHeader1.freeSpaceOffset = sizeof(PageType) + sizeof(LeafPageHeader);
	setLeafPageHeader(firstLeaf, leafHeader1);

	handle.appendPage(firstLeaf);

	_pf_manager->closeFile(handle);

	return retVal;
}

/**
 * negative if first key is smaller
 * positive if second key is smaller
 */
int
IndexManager::compareKeys(Attribute attr, const void* key1, const void* key2) {
	int attrLen = getKeyLength(attr, key1);
	int retVal;

	unsigned char* ucKey1 = new unsigned char[attrLen];
	unsigned char* ucKey2 = new unsigned char[attrLen];
	
	memcpy(ucKey1, key1, attrLen);
	memcpy(ucKey2, key2, attrLen);

	//compare the data
	for(int byte = 0; byte < attrLen; byte++) {
		if( ucKey1[byte] < ucKey2[byte] ) { return -1; }
		if( ucKey1[byte] > ucKey2[byte] ) { return 1; }
	}

	//timeout means keys are equal
	return 0;
}

/*********************************************************************************
 *								Already Implemented								 *
 *********************************************************************************/

IndexManager* IndexManager::_index_manager = 0;
PagedFileManager* IndexManager::_pf_manager = 0;

IndexManager*
IndexManager::instance() {
    if(!_index_manager) {
        _index_manager = new IndexManager();
	}

    return _index_manager;
}

IndexManager::IndexManager() {
	// Initialize the internal PagedFileManager instance.
	_pf_manager = PagedFileManager::instance();
}

IndexManager::~IndexManager() { }

// Returns the page type of a specific page given in input.
PageType
IndexManager::getPageType(void * pageData) {
	PageType type;
	memcpy(&type, pageData, sizeof(PageType));
	return type;
}

// Sets the page type of a specific page given in input.
void
IndexManager::setPageType(void * pageData, PageType pageType) {
	memcpy(pageData, &pageType, sizeof(PageType));
}

// Returns the header of a specific non leaf page given in input.
NonLeafPageHeader
IndexManager::getNonLeafPageHeader(void * pageData) {
	NonLeafPageHeader nonLeafHeader;
	memcpy(&nonLeafHeader, (char*) pageData + sizeof(PageType), sizeof(NonLeafPageHeader));
	return nonLeafHeader;
}

// Sets the header of a specific non leaf page given in input.
void
IndexManager::setNonLeafPageHeader(void * pageData, NonLeafPageHeader nonLeafHeader) {
	memcpy((char*) pageData + sizeof(PageType), &nonLeafHeader, sizeof(NonLeafPageHeader));
}

// Returns the header of a specific leaf page given in input.
LeafPageHeader
IndexManager::getLeafPageHeader(void * pageData) {
	LeafPageHeader leafHeader;
	memcpy(&leafHeader, (char*) pageData + sizeof(PageType), sizeof(LeafPageHeader));
	return leafHeader;
}

// Sets the header of a specific leaf page given in input.
void
IndexManager::setLeafPageHeader(void * pageData, LeafPageHeader leafHeader) {
	memcpy((char*) pageData + sizeof(PageType), &leafHeader, sizeof(LeafPageHeader));
}

// Checks if a specific page is a leaf or not.
bool
IndexManager::isLeafPage(void * pageData) {
	return getPageType(pageData) == LeafPage;
}

// Gets the current root page ID by reading the first page of the file (which only task is containing it).
unsigned
IndexManager::getRootPageID(FileHandle fileHandle) {
	void * pageData = malloc(PAGE_SIZE);

	// Root page ID is stored in the first bytes of page 0.
	fileHandle.readPage(0, pageData);
	unsigned rootPageID;
	memcpy(&rootPageID, pageData, sizeof(unsigned));

	free(pageData);

	return rootPageID;
}

int
IndexManager::destroyFile(const string &fileName) {
	if (_pf_manager->destroyFile(fileName.c_str()) != SUCCESS) {
		return ERROR_PFM_DESTROY;
	}

	return SUCCESS;
}

int
IndexManager::openFile(const string &fileName, FileHandle &fileHandle) {
	if (_pf_manager->openFile(fileName.c_str(), fileHandle) != SUCCESS) {
		return ERROR_PFM_OPEN;
	}

	return SUCCESS;
}

int
IndexManager::closeFile(FileHandle &fileHandle) {
	if (_pf_manager->closeFile(fileHandle) != SUCCESS) {
		return ERROR_PFM_CLOSE;
	}

	return SUCCESS;
}

int
IndexManager::insertEntry(FileHandle &fileHandle, const Attribute &attribute, const void *key, const RID &rid) {
	ChildEntry newChildEntry;
	newChildEntry.key = NULL;

	// Recursive insert, starting from the root page.
	return insert(attribute, key, rid, fileHandle, getRootPageID(fileHandle), newChildEntry);
}

void
IX_PrintError (int rc) {
	switch (rc) {
		case ERROR_PFM_CREATE:
			cout << "Paged File Manager error: create file." << endl;
		break;
		case ERROR_PFM_DESTROY:
			cout << "Paged File Manager error: destroy file." << endl;
		break;
		case ERROR_PFM_OPEN:
			cout << "Paged File Manager error: open file." << endl;
		break;
		case ERROR_PFM_CLOSE:
			cout << "Paged File Manager error: close file." << endl;
		break;
		case ERROR_PFM_READPAGE:
			cout << "Paged File Manager error: read page." << endl;
		break;
		case ERROR_PFM_WRITEPAGE:
			cout << "Paged File Manager error: write page." << endl;
		break;
		case ERROR_PFM_FILEHANDLE:
			cout << "Paged File Manager error: FileHandle problem." << endl;
		break;
		case ERROR_NO_SPACE_AFTER_SPLIT:
			cout << "Tree split error: There is no space for the new entry, even after the split." << endl;
		break;
		case ERROR_RECORD_EXISTS:
			cout << "Index insert error: record already exists." << endl;
		break;
		case ERROR_RECORD_NOT_EXISTS:
			cout << "Index delete error: record does not exists." << endl;
		break;
		case ERROR_UNKNOWN:
			cout << "Unknown error." << endl;
		break;
	}
}
