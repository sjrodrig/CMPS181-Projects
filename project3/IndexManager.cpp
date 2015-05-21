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
#define LEAF_DATA_START (sizeof(PageType) + sizeof(LeafPageHeader))
#define B1 256
#define B2 65536
#define B3 16777216

/**
 * CMPS 181 - Project 3
 * Author: Paolo Di Febbo
 * Modifier: Benjy Strauss
 * File description: Implementing a B+ tree indexing system.
 *						(ref. p. 344-351 Ramakrishnan - Gehrke)
 * 
 * METHOD PROGRESS
 * insertNonLeafRecord		--	# Moderately Tested
 * insertLeafRecord			--	# Complete but Untested
 * insert					--	# Complete but Untested
 * deleteEntryFromLeaf		--	# Complete but Untested
 * deleteEntry				--	# Complete but Untested
 * recordExistsInLeafPage	--  # Complete but Untested
 * treeSearch				--	# Completed using the TA's code provided on Piazza
 * scan						--	# Complete but Untested
 * getKeyLength				--	# Moderately Tested
 * getSonPageID				--	@ Tested and Broken
 * createFile				--	# Lightly Tested
 * compareKeys				--  # Moderately Tested
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
	freeSpaceStart += (lowerBit * B1);
	freeSpaceStart += (upperBit * B2);
	freeSpaceStart += (topBit * B3);

	void* keyPointer = newChildEntry.key;
	int childSize = getKeyLength(attribute, keyPointer) + 4;
	int netSpace = childSize + freeSpaceStart;

	//if page is full, return an error
	if(netSpace >= PAGE_SIZE) { return retVal; }

	unsigned char* newChildEntryData = new unsigned char[childSize];
	unsigned char* oldChildEntryData = new unsigned char[childSize];
	unsigned char* newChildEntryKey = new unsigned char[childSize-4];
	unsigned char* oldChildEntryKey = new unsigned char[childSize-4];

	//copy the child into an array of unsigned chars
	memcpy(newChildEntryData, newChildEntry.key, childSize-4);
	newChildEntryData[childSize-4] = newChildEntry.childPageNumber % B1;
	newChildEntryData[childSize-3] = (newChildEntry.childPageNumber % B2) / B1;
	newChildEntryData[childSize-2] = (newChildEntry.childPageNumber % B3) / B2;
	newChildEntryData[childSize-1] = newChildEntry.childPageNumber / B3;

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
	unsigned char bottomByte = netSpace % B1;
	unsigned char lowerByte = (netSpace % B2) / B1;
	unsigned char upperByte = (netSpace % B3) / B2;
	unsigned char topByte = netSpace / B3;

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

	delete[] metaPage;
	delete[] newChildEntryData;
	delete[] oldChildEntryData;
	delete[] newChildEntryKey;
	delete[] oldChildEntryKey;
	retVal = SUCCESS;

	return retVal;
}

/**
 * @Return: Whether or not the record is in a given leaf page
 */
bool
IndexManager::recordExistsInLeafPage(const Attribute &attribute, const void *key, const RID &rid, void * pageData){
	LeafPageHeader pageHeader = getLeafPageHeader(pageData);

	unsigned offset = sizeof(PageType) + sizeof(LeafPageHeader);
	for (unsigned i = 0; i < pageHeader.recordsNumber; i++){
		void* position = pageData + offset;
		unsigned key_length = getKeyLength(attribute, position);

		// Are the keys the same?
		if (compareKeys(attribute, key, position) == 0) {
			RID cur_RID;
			unsigned char* ridData = new unsigned char[8];
			memcpy(ridData, position + key_length, 8);

			cur_RID.pageNum = ridData[0];
			cur_RID.pageNum += (ridData[1] * B1);
			cur_RID.pageNum += (ridData[2] * B1);
			cur_RID.pageNum += (ridData[3] * B1);

			cur_RID.slotNum = ridData[4];
			cur_RID.slotNum += (ridData[5] * B1);
			cur_RID.slotNum += (ridData[6] * B1);
			cur_RID.slotNum += (ridData[7] * B1);

			// Are the RIDs the same?
			if (cur_RID == rid) {
				return true;
			}
		}
		offset += key_length + sizeof(RID);
	}
	return false;
}

/**
 * @Return: Negative: Second key is larger
 * @Return: Zero: Keys are equal
 * @Return: Positive: First key is larger
 */
int
IndexManager::compareKeys(const Attribute attribute, const void * key1, const void * key2) {
	switch(attribute.type) {
		case TypeInt:
			int key_int_1;
			int key_int_2;
			memcpy(&key_int_1, key1, INT_SIZE);
			memcpy(&key_int_2, key2, INT_SIZE);
			if (key_int_1 > key_int_2){
				return 1;
			} else if (key_int_1 < key_int_2){
				return -1;
			} else {
				return 0;
			}
			break;
		case TypeReal:
			double key_double_1;
			double key_double_2;
			memcpy(&key_double_1, key1, REAL_SIZE);
			memcpy(&key_double_2, key2, REAL_SIZE);
			if (key_double_1 > key_double_2) {
				return 1;
			} else if (key_double_1 < key_double_2) {
				return -1;
			} else {
				return 0;
			}
			break;
		case TypeVarChar:
			// Fetch the sizes of the two strings
			unsigned key_size_1;
			unsigned key_size_2;
			memcpy(&key_size_1, key1, VARCHAR_LENGTH_SIZE);
			memcpy(&key_size_2, key2, VARCHAR_LENGTH_SIZE);

			// Fetch the strings
			char* key_string_1;
			char* key_string_2;
			memcpy(key_string_1, key1 + VARCHAR_LENGTH_SIZE, key_size_1);
			memcpy(key_string_2, key2 + VARCHAR_LENGTH_SIZE, key_size_2);

			// Compare the retreived values
			return strcmp(key_string_1, key_string_2);
	}
	return SUCCESS;
}

// Given a record entry (<key, RID>), writes it into the correct position within the leaf page "pageData".
int
IndexManager::insertLeafRecord(const Attribute &attribute, const void *key, const RID &rid, void* pageData) {
	// Check if record is already present
	if (recordExistsInLeafPage(attribute, key, rid, pageData)) { return ERROR_RECORD_EXISTS; }

	// Fetch the page header
	LeafPageHeader pageHeader = getLeafPageHeader(pageData);

	// Make sure Leaf has space for given key
	unsigned keyLength = getKeyLength(attribute, key);
	if (PAGE_SIZE - pageHeader.freeSpaceOffset < keyLength + sizeof(RID)){
		return ERROR_NO_FREE_SPACE;
	}

	// Cycle existing records until the appropriate space is found for the new one
	unsigned offset = sizeof(PageType) + sizeof(LeafPageHeader);
	for(unsigned i = 0; i < pageHeader.recordsNumber; i++) {
		if (compareKeys(attribute, key, pageData + offset) < 0) { break; }
		offset += getKeyLength(attribute, pageData + offset) + sizeof(RID);
	}

	// Make space for the new record
	memmove((char *) pageData + offset + keyLength + sizeof(RID), (char *)pageData + offset, keyLength + sizeof(RID));

	// Copy new data into free'd space
	memcpy((char *) pageData + offset, &key, keyLength);
	memcpy((char *) pageData + offset + keyLength, &rid, sizeof(RID));

	// Update the page header
	setLeafPageHeader(pageData, pageHeader);
	return SUCCESS;
}

/*
	IS NODE A PARENT (NON-LEAF)? getPageType(pageData)
		TRUE
			Find i? Cycle tree using key info? getSonPageID()
			Call insert recursivly
			IS newChildEntry == null
				TRUE
					Return;
				FALSE
					Does parent page have space?
						TRUE
							insert newChildEntry
							set newChildEntry = null
							Return;
						FALSE
							Split page? 
							Dont forget to setPageType()
		FALSE
			Does leaf page have space? (using page header and its space offset)
				TRUE
					Insert Key & RID?
					Set newChildEntry to null
				FALSE
					Split page in half
					setPageType() of new page
					Insert into first or second half?
					Set newChildEntry to middle value being copied up
*/
			

/**
 * Recursive insert of the record <key, rid> into the (current) page "pageID".
 * newChildEntry will store the return information of the "child" insert call.
 * Following the exact implementation described in Ramakrishnan - Gehrke, p.349.
 *
 * Complete for leaf pages
 * Incomplete for branch (non-leaf) pages
 * 
 */
int
IndexManager::insert(const Attribute &attribute, const void *key, const RID &rid, FileHandle &fileHandle, unsigned pageID, ChildEntry &newChildEntry) {
	// Allocate & Read-in the desired page
	void* pageData = malloc(PAGE_SIZE);
	if (fileHandle.readPage(pageID, pageData) != SUCCESS) {
		cout << "ERROR insert(): Failed to read page " << pageID << "." << endl;
		return ERROR_PFM_READPAGE;
	}

	// Check the page type
	if(isLeafPage(pageData)) {
		// Attempt to insert new leaf record
		unsigned insert_return = insertLeafRecord(attribute, key, rid, pageData);
		if (insert_return == SUCCESS){
			if (fileHandle.writePage(pageID, pageData) != SUCCESS) {
				cout << "ERROR insert(): Failed to write page " << pageID << "." << endl;
				return ERROR_PFM_WRITEPAGE;
			}
			free(pageData);
		} else if (insert_return == ERROR_NO_FREE_SPACE){
			// Allocate space for a second page
			void* new_pageData = malloc(PAGE_SIZE);

			// Fetch the current page header
			LeafPageHeader pageHeader = getLeafPageHeader(pageData);

			// Find place to split page
			unsigned offset = sizeof(PageType) + sizeof(LeafPageHeader);
			unsigned i = 0;
			for(; i < pageHeader.recordsNumber; i++) {
				if (offset > PAGE_SIZE / 2) break;
				offset += getKeyLength(attribute, (char*) pageData + offset) + sizeof(RID);
			}

			// Copy out the middle value
			unsigned middleValue_length = getKeyLength(attribute, (char*) pageData + offset) + sizeof(RID);
			void* middleValue = malloc(middleValue_length);
			memcpy(middleValue, (char*) pageData + offset, middleValue_length);

			// Copy the middle value and further records into the new page
			memcpy(new_pageData, (char*) pageData + offset, PAGE_SIZE - offset);

			// Set the new page's information
			setPageType(new_pageData, LeafPage);
			LeafPageHeader new_leafPageHeader;
			new_leafPageHeader.prevPage = pageID;
			new_leafPageHeader.nextPage = pageHeader.nextPage;
			new_leafPageHeader.recordsNumber = pageHeader.recordsNumber - (i + 1);
			new_leafPageHeader.freeSpaceOffset = sizeof(PageType) + sizeof(LeafPageHeader) + pageHeader.freeSpaceOffset - offset; 
			setLeafPageHeader(new_pageData, new_leafPageHeader);

			// Adapt the existing page's information
			pageHeader.nextPage = fileHandle.getNumberOfPages();
			pageHeader.recordsNumber = i;
			pageHeader.freeSpaceOffset = offset;
			setLeafPageHeader(pageData, pageHeader);

			// Write/Append the pages
			fileHandle.writePage(pageID, pageData);
			fileHandle.appendPage(new_pageData);

			// Set the "return" paramaters & clean up
			newChildEntry.key = middleValue;
			newChildEntry.childPageNumber = pageHeader.nextPage;
			free(new_pageData);
			free(pageData);
		} else {
			cout << "ERROR insert(): Failed to insert new leaf." << endl;
			return -1;
		}
	} else {
		unsigned targetPageID = getSonPageID(attribute, key, pageData);
		int insert_return = insert(attribute, key, rid, fileHandle, targetPageID, newChildEntry);
		if (insert_return != SUCCESS){
			cout << "ERROR insert(): Failed to recursivly call method. (" << insert_return << ")" << endl;
			free(pageData);
			return -2;
		}

		if(newChildEntry.key == NULL){
			free(pageData);
		} else {
			// Insert the new child entry returned
			insert_return = insertNonLeafRecord(attribute, newChildEntry, pageData);
			if (insert_return == SUCCESS){
				free(newChildEntry.key);
				newChildEntry.key = NULL;
			} else if (insert_return == ERROR_NO_FREE_SPACE){
				// Allocate space for a new page
				void* new_pageData = malloc(PAGE_SIZE);

				// Fetch the current page's header
				NonLeafPageHeader pageHeader = getNonLeafPageHeader(pageData);

				// Find place to split page
				unsigned offset = sizeof(PageType) + sizeof(NonLeafPageHeader) + sizeof(unsigned);
				unsigned i = 0;
				for(; i < pageHeader.recordsNumber; i++) {
					if (offset > PAGE_SIZE / 2) break;
					offset = getKeyLength(attribute, (char*) pageData + offset) + sizeof(unsigned);
				}

				// Copy out the middle value
				unsigned middleValue_key_length = getKeyLength(attribute, (char*) pageData + offset);
				void* middleValue = malloc(middleValue_key_length + sizeof(unsigned));
				memcpy(middleValue, (char*) pageData + offset - sizeof(unsigned), sizeof(unsigned));
				memcpy((char*) middleValue + sizeof(unsigned), (char*) pageData + offset, middleValue_key_length);

				// Offset where the second page's data starts
				unsigned true_offset = offset + middleValue_key_length;

				// Copy out the records after the middle value into the new page
				memcpy(new_pageData, (char*) pageData + true_offset, PAGE_SIZE - true_offset);

				// Set new page information
				setPageType(new_pageData, NonLeafPage);
				NonLeafPageHeader new_pageHeader;
				new_pageHeader.recordsNumber = pageHeader.recordsNumber - i;
				new_pageHeader.freeSpaceOffset = sizeof(PageType) + sizeof(NonLeafPageHeader) + pageHeader.freeSpaceOffset - true_offset;
				setNonLeafPageHeader(new_pageData, new_pageHeader);

				// Update the current page's information
				pageHeader.recordsNumber = i;
				pageHeader.freeSpaceOffset = offset - sizeof(unsigned);
				setNonLeafPageHeader(pageData, pageHeader);

				unsigned new_pageID = fileHandle.getNumberOfPages();

				// Write/Append the pages
				fileHandle.writePage(pageID, pageData);
				fileHandle.appendPage(new_pageData);

				// Set the "return" paramaters & clean up
				newChildEntry.key = middleValue;
				newChildEntry.childPageNumber = new_pageID;
				free(new_pageData);
				free(pageData);
			}
		}
	}
	return SUCCESS;
}

/**
 * outputs a vector of sorted RIDs when given a vector of unsorted RIDs
 */
vector<RID>
IndexManager::SortRIDs(vector<RID> input) {
	vector<RID> output;

	RID smallest;
	unsigned smallest_location;
	
	while(input.size() != 0) {
		for(unsigned aa = 0; aa < input.size(); aa++) {
	
			if(smallest.pageNum > input.at(aa).pageNum) {
				smallest = input.at(aa);
				smallest_location = aa;
			} else if (smallest.pageNum == input.at(aa).pageNum && smallest.slotNum > input.at(aa).slotNum) {
				smallest = input.at(aa);
				smallest_location = aa;
			}			
		}
		output.push_back(smallest);
		input.erase(output.begin()+smallest_location);
	}
	return output;
}

/**
 * Given a record entry <key, rid>, deletes it from the leaf page "pageData".
 * keys start at byte 16 (LEAF_DATA_START)
 * 0 = Delete Success
 * -2 = Key not found
 */
int
IndexManager::deleteEntryFromLeaf(const Attribute &attribute, const void *key, const RID &rid, void * pageData) {
	int retVal = -1;
	//a page of unsigned chars
	unsigned char* ucdata = new unsigned char[PAGE_SIZE];
	//copy the data
	memcpy(ucdata, pageData, PAGE_SIZE);

	int keyLength = getKeyLength(attribute, key);
	int keyRIDPairLength = keyLength + 8;

	//specifically to hold the key
	unsigned char* indexKey = new unsigned char[keyLength]; 
	RID indexRID;

	for(int pageIndex = LEAF_DATA_START; true; pageIndex += keyRIDPairLength) {
		int keyDataIndex;

		//load the key
		for(keyDataIndex = 0; keyDataIndex < keyLength; keyLength++) {
			indexKey[keyDataIndex] = ucdata[pageIndex + keyDataIndex];
		}

		if(compareKeys(attribute, indexKey, key) > 0) {
			//haven't found the place yet
			continue;
		} else if(compareKeys(attribute, indexKey, key) < 0) {
			cerr << "Error: Key not found." << endl;
			retVal = -2;
			break;
		} else {
			indexRID.pageNum = ucdata[pageIndex + keyDataIndex];
			indexRID.pageNum += (ucdata[pageIndex + keyDataIndex + 1] * B1);
			indexRID.pageNum += (ucdata[pageIndex + keyDataIndex + 2] * B2);
			indexRID.pageNum += (ucdata[pageIndex + keyDataIndex + 3] * B3);
			indexRID.slotNum = ucdata[pageIndex + keyDataIndex + 4];
			indexRID.slotNum += (ucdata[pageIndex + keyDataIndex + 5] * B1);
			indexRID.slotNum += (ucdata[pageIndex + keyDataIndex + 6] * B2);
			indexRID.slotNum += (ucdata[pageIndex + keyDataIndex + 7] * B3);

			//if the RIDs match, then it's the record we want to delete
			if(indexRID.pageNum == rid.pageNum && indexRID.slotNum == rid.slotNum) {
				/**
				 * pageIndex = record start
				 * pageIndex + keyRIDPairLength = 1st byte of next record
				 * 
				 */

				//compute the amount of memory to move
				int moveBytes = PAGE_SIZE - (pageIndex + keyRIDPairLength);
				int src_offset = pageIndex + keyRIDPairLength;

				// move memory to close gap
				memmove(ucdata+pageIndex, ucdata+src_offset, moveBytes);

				//decrement the number of entries on the page
				int records = ucdata[LEAF_DATA_START - 8];
				records += (ucdata[LEAF_DATA_START - 7] * B1);
				records += (ucdata[LEAF_DATA_START - 6] * B2);
				records += (ucdata[LEAF_DATA_START - 5] * B3);

				records--;
				ucdata[LEAF_DATA_START - 8] = records % B1;
				ucdata[LEAF_DATA_START - 7] = (records % B2) / B1;
				ucdata[LEAF_DATA_START - 6] = (records % B3) / B2;
				ucdata[LEAF_DATA_START - 5] = records / B3;

				//decrement the free space offset
				int freeOffset = ucdata[LEAF_DATA_START - 4];
				freeOffset += (ucdata[LEAF_DATA_START - 3] * B1);
				freeOffset += (ucdata[LEAF_DATA_START - 2] * B2);
				freeOffset += (ucdata[LEAF_DATA_START - 1] * B3);

				freeOffset -= keyRIDPairLength;
				ucdata[LEAF_DATA_START - 4] = freeOffset % B1;
				ucdata[LEAF_DATA_START - 3] = (freeOffset % B2) / B1;
				ucdata[LEAF_DATA_START - 2] = (freeOffset % B3) / B2;
				ucdata[LEAF_DATA_START - 1] = freeOffset / B3;

				memcpy(pageData, ucdata, PAGE_SIZE);

				retVal = SUCCESS;
				break;
			}
		}
	}

	delete[] indexKey;
	delete[] ucdata;
	return retVal;
}

/**
 * Finds the leaf page in the tree and calls deleteEntryFromLeaf() on it
 * No merging is ever done for the sake of simplicity and because splitting is
 * an expensive operation
 */
int
IndexManager::deleteEntry(FileHandle &fileHandle, const Attribute &attribute, const void *key, const RID &rid) {
	unsigned char* targetPage = new unsigned char[PAGE_SIZE];
	unsigned char* rootLocPage = new unsigned char[PAGE_SIZE];
	int retVal = -1;

	if (fileHandle.readPage(0, rootLocPage) != SUCCESS) {
		cout << "Error Searching: Couldn't read page 0 (Zero)." << endl;
		return ERROR_PFM_READPAGE;
	}

	unsigned rootLoc = 0;					//location of the root
	rootLoc += rootLocPage[0];
	rootLoc += (rootLocPage[1] * B1);		//256^1
	rootLoc += (rootLocPage[2] * B2);		//256^2
	rootLoc += (rootLocPage[3] * B3);		//256^3

	delete[] rootLocPage;					//don't need it anymore
	unsigned targetPageID;

	retVal = treeSearch(fileHandle, attribute, key, rootLoc, targetPageID);
	if(retVal != SUCCESS) {
		cout << "Error Searching B+ Tree: error code " << retVal << "." << endl;
		return retVal;
	}

	if (fileHandle.readPage(targetPageID, targetPage) != SUCCESS) {
		cout << "Error Searching: Couldn't read page " << targetPageID << "." << endl;
		return ERROR_PFM_READPAGE;
	}

	retVal = deleteEntryFromLeaf(attribute, key, rid, targetPage);
	//write the page to the file.
	fileHandle.writePage(targetPageID, targetPage);

	delete[] targetPage;
	return retVal;
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

/**
 * Scan
 */
int
IndexManager::scan(FileHandle &fileHandle, const Attribute &attribute, const void *lowKey, const void *highKey, bool lowKeyInclusive, bool highKeyInclusive, IX_ScanIterator &ix_ScanIterator) {
	int retVal = -1;

	//get the size (all of the keys that are the same size)
	int keySize = getKeyLength(attribute, lowKey);

	//going to nest this iterator in the IX_ScanIterator
	RBFM_ScanIterator nestMe;
	vector<RID> rids;
	vector<int> dataVectorSizes;
	vector<void*> dataVector;

	//the location of the root
	unsigned rootLoc;
	unsigned char* page0 = new unsigned char[PAGE_SIZE];

	retVal = fileHandle.readPage(0, page0);
	if (retVal != SUCCESS) {
		cerr << "Error: Page 0 (zero) could not be read: Thus, the root could not be found." << endl;
		return retVal;
	}

	rootLoc = page0[0];
	rootLoc += (page0[1] * B1);
	rootLoc += (page0[2] * B2);
	rootLoc += (page0[3] * B3);

	unsigned pageToRead;
	retVal = treeSearch(fileHandle, attribute, lowKey, rootLoc, pageToRead);
	if(retVal == ERROR_PFM_READPAGE) {
		cerr << "Page Read Error." << endl;
		return retVal;
	}

	//start getting ready to read the (RID, key) pairs, one at a time.
	unsigned char* curPage = new unsigned char[PAGE_SIZE];
	unsigned char* keyRead = new unsigned char[keySize+8];
	unsigned char* pureKey = new unsigned char[keySize];
	RID rIDRead;
	//set this flag so we read a page to start
	bool endOfPage = true;
	//the next page
	int nextPage;
	//the total number of records on the page
	unsigned recordsNumber;
	//how many records on the page we've read
	unsigned readRecNum;
	unsigned pageReadOffset;

	do {
		if(endOfPage == true) {
			//read the page
			retVal = fileHandle.readPage(pageToRead, curPage);
			pageReadOffset = LEAF_DATA_START;

			//set the next leaf page to be read
			nextPage = curPage[4];
			nextPage += (curPage[5] * B1);
			nextPage += (curPage[6] * B2);
			nextPage += (curPage[7] * B3);
 
			//get the number of records on the page
			recordsNumber = curPage[8];
			recordsNumber += (curPage[9] * B1);
			recordsNumber += (curPage[10] * B2);
			recordsNumber += (curPage[11] * B3);

			if (retVal != SUCCESS) {
				cerr << "Error: Page " << pageToRead << " could not be read." << endl;
				return retVal;
			}
			endOfPage = false;
		}

		readRecNum++;

		/**
		 * actually read the record
		 * Key, then RID
		 */
		for(int index = 0; index < (keySize+8); index++) {
			keyRead[index] = curPage[pageReadOffset + index];
			if(index < keySize) {
				pureKey[index] = curPage[pageReadOffset + index];
			}
		}

		/**
		 * @Return: Negative: Second key is larger
		 * @Return: Zero: Keys are equal
		 * @Return: Positive: First key is larger
		 */
		int lowKeyComp = compareKeys(attribute, lowKey, keyRead);
		int highKeyComp = compareKeys(attribute, highKey, keyRead);

		//equals low key and lowKeyInclusive
		if(lowKeyComp == 0 && lowKeyInclusive == true) {
			rIDRead.pageNum = keyRead[keySize];
			rIDRead.pageNum += (curPage[keySize+1] * B1);
			rIDRead.pageNum += (curPage[keySize+2] * B2);
			rIDRead.pageNum += (curPage[keySize+3] * B3);

			rIDRead.slotNum = keyRead[keySize+4];
			rIDRead.slotNum += (curPage[keySize+5] * B1);
			rIDRead.slotNum += (curPage[keySize+6] * B2);
			rIDRead.slotNum += (curPage[keySize+7] * B3);

			rids.push_back(rIDRead);
			dataVectorSizes.push_back(keySize);
			dataVector.push_back(pureKey);
		//equals high key and lowKeyInclusive
		} else if (highKeyComp == 0 && highKeyInclusive == true) {
			rIDRead.pageNum = keyRead[keySize];
			rIDRead.pageNum += (curPage[keySize+1] * B1);
			rIDRead.pageNum += (curPage[keySize+2] * B2);
			rIDRead.pageNum += (curPage[keySize+3] * B3);

			rIDRead.slotNum = keyRead[keySize+4];
			rIDRead.slotNum += (curPage[keySize+5] * B1);
			rIDRead.slotNum += (curPage[keySize+6] * B2);
			rIDRead.slotNum += (curPage[keySize+7] * B3);

			rids.push_back(rIDRead);
			dataVectorSizes.push_back(keySize);
			dataVector.push_back(pureKey);
		//within the range
		} else if (highKeyComp > 0 && lowKeyInclusive < true) {
			rIDRead.pageNum = keyRead[keySize];
			rIDRead.pageNum += (curPage[keySize+1] * B1);
			rIDRead.pageNum += (curPage[keySize+2] * B2);
			rIDRead.pageNum += (curPage[keySize+3] * B3);

			rIDRead.slotNum = keyRead[keySize+4];
			rIDRead.slotNum += (curPage[keySize+5] * B1);
			rIDRead.slotNum += (curPage[keySize+6] * B2);
			rIDRead.slotNum += (curPage[keySize+7] * B3);

			rids.push_back(rIDRead);
			dataVectorSizes.push_back(keySize);
			dataVector.push_back(pureKey);
		}

		pageReadOffset += (keySize+8);

		//reached the end of the page
		if(readRecNum == recordsNumber) {
			endOfPage = true;
			pageToRead = nextPage;
			readRecNum = 0;
		}
	} while(compareKeys(attribute, highKey, keyRead) > 0);

	//set the vectors of the RBFM_ScanIterator
	nestMe.setVectors(rids, dataVectorSizes, dataVector);

	//set the IX_ScanIterator
	IX_ScanIterator retIter(nestMe);
	ix_ScanIterator = retIter;

	delete[] page0;
	delete[] curPage;
	delete[] keyRead;
	delete[] pureKey;
	return retVal;
}

unsigned 
IndexManager::getKeyLength(const Attribute &attribute, const void *key){
	switch(attribute.type){
		case TypeInt:
			return INT_SIZE;
		case TypeReal:
			return REAL_SIZE;
		case TypeVarChar:
			unsigned key_size;
			memcpy(&key_size, key, VARCHAR_LENGTH_SIZE);
			return key_size + VARCHAR_LENGTH_SIZE;
	}
	return 0;
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

		if(compareKeys(attribute, key, oldChildEntryKey) < 0) {
			continue;
		} else { //read the value

			int mult = 1;
			for(int aa = 0; aa < 4; aa++) {
				retVal += (mult * ucPage[keyIndex+loadIndex]);
				loadIndex++;
				mult *= 256;
			}

			cout << "(1) getSonPageID() Return Value = " << retVal << endl;
			return retVal;
		}
		delete[] oldChildEntryKey;
	}

	cout << "(2) getSonPageID() Return Value = " << retVal << endl;

	delete[] ucPage;
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
	void* defaultRootLocation = malloc(PAGE_SIZE);
	unsigned rootPageNumber = DEFAULT_ROOT_LOCATION;
	memcpy(defaultRootLocation, &rootPageNumber, sizeof(unsigned));
	handle.appendPage(defaultRootLocation);

	//write the root on page 1
	void* rootPage = malloc(PAGE_SIZE);

	//set the page's type to non-leaf
	setPageType(rootPage, NonLeafPage);

	//write the header
	NonLeafPageHeader rootHeader;
	rootHeader.recordsNumber = 0;
	rootHeader.freeSpaceOffset = sizeof(PageType) + sizeof(NonLeafPageHeader) + sizeof(unsigned);
	setNonLeafPageHeader(rootPage, rootHeader);

	unsigned linkOffset = rootHeader.freeSpaceOffset - sizeof(unsigned);

	/**
	 * write the first leaf number
	 * this is the first pointer
	 * after this pointer, we write (key, pointer) pairs
	 */
	unsigned firstLeafNumber = 2;
	memcpy((char*)rootPage + linkOffset, &firstLeafNumber, sizeof(unsigned));

	handle.appendPage(rootPage);

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
			cerr << "Paged File Manager error: create file." << endl;
		break;
		case ERROR_PFM_DESTROY:
			cerr << "Paged File Manager error: destroy file." << endl;
		break;
		case ERROR_PFM_OPEN:
			cerr << "Paged File Manager error: open file." << endl;
		break;
		case ERROR_PFM_CLOSE:
			cerr << "Paged File Manager error: close file." << endl;
		break;
		case ERROR_PFM_READPAGE:
			cerr << "Paged File Manager error: read page." << endl;
		break;
		case ERROR_PFM_WRITEPAGE:
			cerr << "Paged File Manager error: write page." << endl;
		break;
		case ERROR_PFM_FILEHANDLE:
			cerr << "Paged File Manager error: FileHandle problem." << endl;
		break;
		case ERROR_NO_SPACE_AFTER_SPLIT:
			cerr << "Tree split error: There is no space for the new entry, even after the split." << endl;
		break;
		case ERROR_RECORD_EXISTS:
			cerr << "Index insert error: record already exists." << endl;
		break;
		case ERROR_RECORD_NOT_EXISTS:
			cerr << "Index delete error: record does not exists." << endl;
		break;
		case ERROR_UNKNOWN:
			cerr << "Unknown error." << endl;
		break;
	}
}
