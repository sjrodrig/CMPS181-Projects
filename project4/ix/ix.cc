// CMPS 181 - Project 3
// Author:				Paolo Di Febbo
// File description:	Implementing a B+ tree indexing system.
//						(ref. p. 344-351 Ramakrishnan - Gehrke).

#include "ix.h"

IndexManager* IndexManager::_index_manager = 0;
PagedFileManager* IndexManager::_pf_manager = 0;

IndexManager* IndexManager::instance()
{
    if(!_index_manager)
        _index_manager = new IndexManager();

    return _index_manager;
}

IndexManager::IndexManager()
{
	// Initialize the internal PagedFileManager instance.
	_pf_manager = PagedFileManager::instance();
}

IndexManager::~IndexManager()
{
}

// Returns the page type of a specific page given in input.
PageType IndexManager::getPageType(void * pageData)
{
	PageType type;
	memcpy(&type, pageData, sizeof(PageType));
	return type;
}

// Sets the page type of a specific page given in input.
void IndexManager::setPageType(void * pageData, PageType pageType)
{
	memcpy(pageData, &pageType, sizeof(PageType));
}

// Returns the header of a specific non leaf page given in input.
NonLeafPageHeader IndexManager::getNonLeafPageHeader(void * pageData)
{
	NonLeafPageHeader nonLeafHeader;
	memcpy(&nonLeafHeader, (char*) pageData + sizeof(PageType), sizeof(NonLeafPageHeader));
	return nonLeafHeader;
}

// Sets the header of a specific non leaf page given in input.
void IndexManager::setNonLeafPageHeader(void * pageData, NonLeafPageHeader nonLeafHeader)
{
	memcpy((char*) pageData + sizeof(PageType), &nonLeafHeader, sizeof(NonLeafPageHeader));
}

// Returns the header of a specific leaf page given in input.
LeafPageHeader IndexManager::getLeafPageHeader(void * pageData)
{
	LeafPageHeader leafHeader;
	memcpy(&leafHeader, (char*) pageData + sizeof(PageType), sizeof(LeafPageHeader));
	return leafHeader;
}

// Sets the header of a specific leaf page given in input.
void IndexManager::setLeafPageHeader(void * pageData, LeafPageHeader leafHeader)
{
	memcpy((char*) pageData + sizeof(PageType), &leafHeader, sizeof(LeafPageHeader));
}

// Checks if a specific page is a leaf or not.
bool IndexManager::isLeafPage(void * pageData)
{
	return getPageType(pageData) == LeafPage;
}

// Gets the current root page ID by reading the first page of the file (which only task is containing it).
unsigned IndexManager::getRootPageID(FileHandle fileHandle)
{
	void * pageData = malloc(PAGE_SIZE);

	// Root page ID is stored in the first bytes of page 0.
	fileHandle.readPage(0, pageData);
	unsigned rootPageID;
	memcpy(&rootPageID, pageData, sizeof(unsigned));

	free(pageData);

	return rootPageID;
}

// Given in input two keys and their descriptor (attribute), compares them and returns:
//  1: if key1 > key2
//  0: if key1 == key2
// -1: if key1 < key2
int IndexManager::compareKeys(const Attribute attribute, const void * key1, const void * key2)
{
	// Comparisons with NULL (key1 means -Infinity, key2 means +Infinity) are special cases.
	if (key1 == NULL || key2 == NULL)
		return -1;

	// Standard case (no NULL): Translate the void* input keys into typed variables, and makes the comparison.
	switch (attribute.type)
	{
		case TypeInt:
			int intKey1, intKey2;

			memcpy(&intKey1, key1, INT_SIZE);
			memcpy(&intKey2, key2, INT_SIZE);

			// Makes the comparison.
			if(intKey1 > intKey2)
				return 1;
			else if(intKey1 < intKey2)
				return -1;
			else
				return 0;
		break;
		case TypeReal:
			float realKey1, realKey2;

			memcpy(&realKey1, key1, REAL_SIZE);
			memcpy(&realKey2, key2, REAL_SIZE);

			// Makes the comparison.
			if(realKey1 > realKey2)
				return 1;
			else if(realKey1 < realKey2)
				return -1;
			else
				return 0;
		break;
		case TypeVarChar:
			unsigned stringKey1Length, stringKey2Length;

			// Gets the string lengths.
			memcpy(&stringKey1Length, key1, VARCHAR_LENGTH_SIZE);
			memcpy(&stringKey2Length, key2, VARCHAR_LENGTH_SIZE);

			// Copies the strings data.
			char * stringKey1 = (char*) malloc(stringKey1Length + 1);
			char * stringKey2 = (char*) malloc(stringKey2Length + 1);
			memcpy(stringKey1, (char*) key1 + VARCHAR_LENGTH_SIZE, stringKey1Length);
			memcpy(stringKey2, (char*) key2 + VARCHAR_LENGTH_SIZE, stringKey2Length);

			// Adds the string terminators.
			stringKey1[stringKey1Length] = '\0';
			stringKey2[stringKey2Length] = '\0';

			// Makes the comparison.
			int result = strcmp(stringKey1, stringKey2);

			free(stringKey1);
			free(stringKey2);

			return result;
		break;
	}

	// We should never be here.
	return ERROR_UNKNOWN;
}

// Key length computation.
unsigned IndexManager::getKeyLength(const Attribute &attribute, const void * key)
{
	unsigned keyLength;

	switch (attribute.type)
	{
		case TypeInt:
			keyLength = INT_SIZE;
		break;
		case TypeReal:
			keyLength = REAL_SIZE;
		break;
		case TypeVarChar:
			// Gets the current string length.
			memcpy(&keyLength, key, VARCHAR_LENGTH_SIZE);
			// Adds the prefix size.
			keyLength += VARCHAR_LENGTH_SIZE;
		break;
	}

	return keyLength;
}

// Given a non-leaf page and a key, finds the correct (direct) son page ID in which the key "fits".
unsigned IndexManager::getSonPageID(const Attribute attribute, const void * key, void * pageData)
{
	unsigned sonPageID;
	NonLeafPageHeader nonLeafHeader = getNonLeafPageHeader(pageData);

	// Reading of the entries starts.
	unsigned offset = sizeof(PageType) + sizeof(NonLeafPageHeader);
	bool foundSon = false;
	for(unsigned i = 0; i < nonLeafHeader.recordsNumber && !foundSon; i++)
	{
		// Reads the page ID.
		memcpy(&sonPageID, (char *) pageData + offset, sizeof(unsigned));
		offset += sizeof(unsigned);

		// Compares the input key with the current key.
		if(compareKeys(attribute, key, (char*) pageData + offset) < 0)
			foundSon = true;
		else
			// Offset increment.
			offset += getKeyLength(attribute, (char*) pageData + offset);
	}

	if(!foundSon)
		// If we are here, the son page ID is the last one.
		memcpy(&sonPageID, (char *) pageData + offset, sizeof(unsigned));

	return sonPageID;
}

RC IndexManager::createFile(const string &fileName)
{
	// Creating a new paged file.
	if (_pf_manager->createFile(fileName.c_str()) != SUCCESS)
		return ERROR_PFM_CREATE;

	FileHandle handle;
	_pf_manager->openFile(fileName.c_str(), handle);

	void * pageData = malloc(PAGE_SIZE);

	// Setting up the first page, which will contain the current root page ID (stored in the first bytes).
	unsigned defaultRootPageID = 1;
	memcpy(pageData, &defaultRootPageID, sizeof(unsigned));

	handle.appendPage(pageData);

	// Setting up the second page, which will contain the root page data itself.
	// N.B.: The initial root page also contains a pointer to the first leaf (see code below).
	PageType pageType = NonLeafPage;
	setPageType(pageData, pageType);
	NonLeafPageHeader nonLeafHeader;
	nonLeafHeader.recordsNumber = 0;
	nonLeafHeader.freeSpaceOffset = sizeof(PageType) + sizeof(NonLeafPageHeader) + sizeof(unsigned);
	setNonLeafPageHeader(pageData, nonLeafHeader);

	unsigned firstLeafPageNumber = 2;
	memcpy((char*) pageData + sizeof(PageType) + sizeof(NonLeafPageHeader), &firstLeafPageNumber, sizeof(unsigned));

	handle.appendPage(pageData);

	// Setting up the third page, which will contain the first leaf page data.
	pageType = LeafPage;
	setPageType(pageData, pageType);
	LeafPageHeader leafHeader;
	leafHeader.prevPage = NULL_PAGE_ID;
	leafHeader.nextPage = NULL_PAGE_ID;
	leafHeader.recordsNumber = 0;
	leafHeader.freeSpaceOffset = sizeof(PageType) + sizeof(LeafPageHeader);
	setLeafPageHeader(pageData, leafHeader);

	handle.appendPage(pageData);

	_pf_manager->closeFile(handle);

	free(pageData);

	return 0;
}

RC IndexManager::destroyFile(const string &fileName)
{
	if (_pf_manager->destroyFile(fileName.c_str()) != SUCCESS)
		return ERROR_PFM_DESTROY;

	return SUCCESS;
}

RC IndexManager::openFile(const string &fileName, FileHandle &fileHandle)
{
	if (_pf_manager->openFile(fileName.c_str(), fileHandle) != SUCCESS)
		return ERROR_PFM_OPEN;

	return SUCCESS;
}

RC IndexManager::closeFile(FileHandle &fileHandle)
{
	if (_pf_manager->closeFile(fileHandle) != SUCCESS)
		return ERROR_PFM_CLOSE;

	return SUCCESS;
}

// Returns true if the specific input record <key, rid> already exists within the leaf page "pageData".
bool IndexManager::recordExistsInLeafPage(const Attribute &attribute, const void *key, const RID &rid, void * pageData)
{
	LeafPageHeader header = getLeafPageHeader(pageData);

	// Scan through all the page record entries.
	unsigned offset = sizeof(PageType) + sizeof(LeafPageHeader);
	unsigned currentKeyLength;
	RID currentRID;
	for(unsigned i = 0; i < header.recordsNumber; i++)
	{
		// Reads the current RID.
		currentKeyLength = getKeyLength(attribute, (char*) pageData + offset);
		memcpy(&currentRID, (char*) pageData + offset + currentKeyLength, sizeof(RID));

		// Compares the current read key and RID with the ones given in input.
		// If we have found the same <key, rid> pair, return true.
		if(compareKeys(attribute, key, (char*) pageData + offset) == 0 && currentRID == rid)
			return true;

		offset += currentKeyLength + sizeof(RID);
	}

	return false;
}

// Given a ChildEntry structure (<key, child page id>), writes it into the correct position within the non leaf page "pageData".
RC IndexManager::insertNonLeafRecord(const Attribute &attribute, ChildEntry &newChildEntry, void * pageData)
{
	NonLeafPageHeader header = getNonLeafPageHeader(pageData);
	unsigned keyLength = getKeyLength(attribute, newChildEntry.key);

	// Check for free space for the new record to be inserted.
	if (PAGE_SIZE - header.freeSpaceOffset < keyLength + sizeof(unsigned))
		return ERROR_NO_FREE_SPACE;

	// Finds the exact place (offset) in the page where the new entry must be inserted (N.B.: ordered insert).
	unsigned offset = sizeof(PageType) + sizeof(NonLeafPageHeader) + sizeof(unsigned);
	for(unsigned i = 0; i < header.recordsNumber; i++)
	{
		if(compareKeys(attribute, newChildEntry.key, (char*) pageData + offset) < 0)
			break;

		offset += getKeyLength(attribute, (char*) pageData + offset) + sizeof(unsigned);
	}

	// Makes some room for the new entry by moving everything after the offset by the size of the input ChildEntry (to be inserted there).
	memmove((char*) pageData + offset + keyLength + sizeof(unsigned), (char*) pageData + offset, header.freeSpaceOffset - offset);

	// Copies the new entry there.
	memcpy((char*) pageData + offset, newChildEntry.key, keyLength);
	memcpy((char*) pageData + offset + keyLength, &newChildEntry.childPageNumber, sizeof(unsigned));

	// Updates the page header.
	header.freeSpaceOffset += keyLength + sizeof(unsigned);
	header.recordsNumber += 1;
	setNonLeafPageHeader(pageData, header);

	free(newChildEntry.key);
	return 0;
}

// Given a record entry (<key, RID>), writes it into the correct position within the leaf page "pageData".
RC IndexManager::insertLeafRecord(const Attribute &attribute, const void *key, const RID &rid, void * pageData)
{
	// If the same record already exists, error.
	if(recordExistsInLeafPage(attribute, key, rid, pageData))
		return ERROR_RECORD_EXISTS;

	LeafPageHeader header = getLeafPageHeader(pageData);
	unsigned keyLength = getKeyLength(attribute, key);

	// Check for free space for the new record to be inserted.
	if (PAGE_SIZE - header.freeSpaceOffset < keyLength + sizeof(RID))
		return ERROR_NO_FREE_SPACE;

	// Finds the exact place within the page where the new entry must be inserted (N.B.: ordered insert).
	unsigned offset = sizeof(PageType) + sizeof(LeafPageHeader);
	for(unsigned i = 0; i < header.recordsNumber; i++)
	{
		if(compareKeys(attribute, key, (char*) pageData + offset) < 0)
			break;

		offset += getKeyLength(attribute, (char*) pageData + offset) + sizeof(RID);
	}

	// Makes some room for the new entry by moving everything after the offset by the size of the new entry (to be inserted there).
	memmove((char*) pageData + offset + keyLength + sizeof(RID), (char*) pageData + offset, header.freeSpaceOffset - offset);

	// Copies the new entry.
	memcpy((char*) pageData + offset, key, keyLength);
	memcpy((char*) pageData + offset + keyLength, &rid, sizeof(RID));

	// Updates the header.
	header.freeSpaceOffset += keyLength + sizeof(RID);
	header.recordsNumber += 1;
	setLeafPageHeader(pageData, header);

	return 0;
}

// Recursive insert of the record <key, rid> into the (current) page "pageID".
// newChildEntry will store the return information of the "child" insert call.
// Following the exact implementation described in Ramakrishnan - Gehrke, p.349.
RC IndexManager::insert(const Attribute &attribute, const void *key, const RID &rid, FileHandle &fileHandle, unsigned pageID, ChildEntry &newChildEntry)
{
	void * pageData = malloc(PAGE_SIZE);

	// Gets the current page.
	if (fileHandle.readPage(pageID, pageData) != SUCCESS)
		return ERROR_PFM_READPAGE;

	if(!isLeafPage(pageData))
	{
		// Non-leaf page.

		// Recursive insert.
		unsigned sonPageID = getSonPageID(attribute, key, pageData);
		RC result = insert(attribute, key, rid, fileHandle, sonPageID, newChildEntry);
		if (result != SUCCESS)
			return result;

		// Return variable check.
		if(newChildEntry.key == NULL)
		{
			// Everything ok: no split occurred.
			free(pageData);
			return 0;
		}

		// If we are here, a split occurred within the "son" insert call.

		// Try to insert the new record generated by the son's split.
		result = insertNonLeafRecord(attribute, newChildEntry, pageData);
		if (result == SUCCESS)
		{
			// If success, writeback.
			if (fileHandle.writePage(pageID, pageData) != SUCCESS)
				return ERROR_PFM_WRITEPAGE;

			// Nothing to return to the parent insert call.
			newChildEntry.key = NULL;

			free(pageData);
			return 0;
		}
		// If there is not enough space: Split.
		else if(result == ERROR_NO_FREE_SPACE)
		{
			NonLeafPageHeader header = getNonLeafPageHeader(pageData);

			// Finds the (offset) border between the first half of the records and the second half.
			unsigned offset = sizeof(PageType) + sizeof(LeafPageHeader) + sizeof(unsigned);
			unsigned i;
			for(i = 0; i < header.recordsNumber; i++)
			{
				offset += getKeyLength(attribute, (char*) pageData + offset) + sizeof(unsigned);

				if(offset > PAGE_SIZE / 2)
					break;
			}
			unsigned middleKeyLength = getKeyLength(attribute, (char*) pageData + offset);
			void * middleKey = malloc(middleKeyLength);
			memcpy(middleKey, (char*) pageData + offset, middleKeyLength);

			// Copies the second half into a new leaf page, excluding the "middle" key (pushed up).

			offset += middleKeyLength;

			void * newPageData = malloc(PAGE_SIZE);

			PageType newPageType = NonLeafPage;
			setPageType(newPageData, newPageType);
			NonLeafPageHeader newNonLeafHeader;
			newNonLeafHeader.recordsNumber = header.recordsNumber - (i + 2); // + 2 is for excluding the "middle" key.
			newNonLeafHeader.freeSpaceOffset = sizeof(PageType) + sizeof(NonLeafPageHeader) + header.freeSpaceOffset - offset;
			setNonLeafPageHeader(newPageData, newNonLeafHeader);

			memcpy	(
					(char*) newPageData + sizeof(PageType) + sizeof(NonLeafPageHeader),
					(char*) pageData + offset,
					header.freeSpaceOffset - offset
					);

			unsigned newPageID = fileHandle.getNumberOfPages();

			// Updates the old page's records number.
			header.recordsNumber = i + 1;
			header.freeSpaceOffset = offset - middleKeyLength;
			setNonLeafPageHeader(pageData, header);

			// Adds the new record into the correct page.
			if(compareKeys(attribute, key, middleKey) < 0)
			{
				if(insertNonLeafRecord(attribute, newChildEntry, pageData) != SUCCESS)
					return ERROR_NO_SPACE_AFTER_SPLIT;
			}
			else
			{
				if(insertNonLeafRecord(attribute, newChildEntry, newPageData) != SUCCESS)
					return ERROR_NO_SPACE_AFTER_SPLIT;
			}

			// Writes the new page.
			if (fileHandle.appendPage(newPageData) != SUCCESS)
				return ERROR_PFM_WRITEPAGE;
			free(newPageData);

			// Writeback of the old page.
			if (fileHandle.writePage(pageID, pageData) != SUCCESS)
				return ERROR_PFM_WRITEPAGE;
			free(pageData);

			// Set newChildEntry in order to return to the "parent" insert call some useful data about the split.
			newChildEntry.key = middleKey;
			newChildEntry.childPageNumber = newPageID;

			// If we splitted the root page, we need to create a new root.
			if(pageID == getRootPageID(fileHandle))
			{
				void * newRootPageData = malloc(PAGE_SIZE);

				newPageType = NonLeafPage;
				setPageType(newRootPageData, newPageType);
				newNonLeafHeader.recordsNumber = 0;
				newNonLeafHeader.freeSpaceOffset = sizeof(PageType) + sizeof(NonLeafPageHeader) + sizeof(unsigned);
				setNonLeafPageHeader(newRootPageData, newNonLeafHeader);

				// Adds the new data.
				memcpy((char*) newRootPageData + sizeof(PageType) + sizeof(NonLeafPageHeader), &pageID, sizeof(unsigned));
				insertNonLeafRecord(attribute, newChildEntry, newRootPageData);

				// Writes the new root page.
				unsigned newRootPageID = fileHandle.getNumberOfPages();
				if (fileHandle.appendPage(newRootPageData) != SUCCESS)
					return ERROR_PFM_WRITEPAGE;

				// Updates the root page pointer, which is stored in the first page.
				memcpy(newRootPageData, &newRootPageID, sizeof(unsigned));
				if (fileHandle.writePage(0, newRootPageData) != SUCCESS)
					return ERROR_PFM_WRITEPAGE;

				free(newRootPageData);
			}

			return SUCCESS;
		}
		else
			return result;
	}
	else
	{
		// Leaf Page.

		// Try to insert the new record.
		RC result = insertLeafRecord(attribute, key, rid, pageData);
		if (result == SUCCESS)
		{
			// If success, writeback.
			if (fileHandle.writePage(pageID, pageData) != SUCCESS)
				return ERROR_PFM_WRITEPAGE;

			// Nothing to return to the parent insert call.
			newChildEntry.key = NULL;

			free(pageData);
			return 0;
		}
		// If there is not enough space: Split.
		else if (result == ERROR_NO_FREE_SPACE)
		{
			LeafPageHeader header = getLeafPageHeader(pageData);

			// Finds the (offset) border between the first half of the records and the second half.
			unsigned offset = sizeof(PageType) + sizeof(LeafPageHeader);
			unsigned i;
			for(i = 0; i < header.recordsNumber; i++)
			{
				offset += getKeyLength(attribute, (char*) pageData + offset) + sizeof(RID);

				if(offset > PAGE_SIZE / 2)
					break;
			}
			unsigned middleKeyLength = getKeyLength(attribute, (char*) pageData + offset);
			void * middleKey = malloc(middleKeyLength);
			memcpy(middleKey, (char*) pageData + offset, middleKeyLength);

			// Copies the second half into a new leaf page.

			void * newPageData = malloc(PAGE_SIZE);

			PageType newPageType = LeafPage;
			setPageType(newPageData, newPageType);
			LeafPageHeader newLeafHeader;
			newLeafHeader.prevPage = pageID;
			newLeafHeader.nextPage = header.nextPage;
			newLeafHeader.recordsNumber = header.recordsNumber - (i + 1);
			newLeafHeader.freeSpaceOffset = sizeof(PageType) + sizeof(LeafPageHeader) + header.freeSpaceOffset - offset;
			setLeafPageHeader(newPageData, newLeafHeader);

			memcpy	(
					(char*) newPageData + sizeof(PageType) + sizeof(LeafPageHeader),
					(char*) pageData + offset,
					header.freeSpaceOffset - offset
					);

			unsigned newPageID = fileHandle.getNumberOfPages();

			// Updates the old page's siblings and records number.
			header.nextPage = newPageID;
			header.recordsNumber = i + 1;
			header.freeSpaceOffset = offset;
			setLeafPageHeader(pageData, header);

			// Adds the new record into the correct page.
			if(compareKeys(attribute, key, middleKey) < 0)
			{
				if (insertLeafRecord(attribute, key, rid, pageData) != SUCCESS)
					return ERROR_NO_SPACE_AFTER_SPLIT;
			}
			else
			{
				if (insertLeafRecord(attribute, key, rid, newPageData) != SUCCESS)
					return ERROR_NO_SPACE_AFTER_SPLIT;
			}

			// Writes the new page.
			if (fileHandle.appendPage(newPageData) != SUCCESS)
				return ERROR_PFM_WRITEPAGE;
			free(newPageData);

			// Writeback of the old page.
			if (fileHandle.writePage(pageID, pageData) != SUCCESS)
				return ERROR_PFM_WRITEPAGE;
			free(pageData);

			// Set newChildEntry in order to return to the "parent" insert call some useful data about the split.
			newChildEntry.key = middleKey;
			newChildEntry.childPageNumber = newPageID;

			return 0;
		}
		else
			return result;
	}

	// We should never be here.
	return ERROR_UNKNOWN;
}

RC IndexManager::insertEntry(FileHandle &fileHandle, const Attribute &attribute, const void *key, const RID &rid)
{
	ChildEntry newChildEntry;
	newChildEntry.key = NULL;

	// Recursive insert, starting from the root page.
	return insert(attribute, key, rid, fileHandle, getRootPageID(fileHandle), newChildEntry);
}

// Given a record entry <key, rid>, deletes it from the leaf page "pageData".
RC IndexManager::deleteEntryFromLeaf(const Attribute &attribute, const void *key, const RID &rid, void * pageData)
{
	LeafPageHeader header = getLeafPageHeader(pageData);

	// Cycles through all the record entries.
	unsigned offset = sizeof(PageType) + sizeof(LeafPageHeader);
	unsigned currentKeyLength;
	unsigned i;
	RID currentRID;
	for(i = 0; i < header.recordsNumber; i++)
	{
		// Copies the current RID.
		currentKeyLength = getKeyLength(attribute, (char*) pageData + offset);
		memcpy(&currentRID, (char*) pageData + offset + currentKeyLength, sizeof(RID));

		// If we have found the <key, rid> pair, we are in the right spot.
		if(compareKeys(attribute, key, (char*) pageData + offset) == 0 && currentRID == rid)
			break;

		offset += currentKeyLength + sizeof(RID);
	}

	// If we reached the page end, the specified record does not exist.
	if (i == header.recordsNumber)
		return ERROR_RECORD_NOT_EXISTS;

	// Shrinks the page to overwrite the old record and compact space.
	memmove	(
			(char*) pageData + offset,
			(char*) pageData + offset + currentKeyLength + sizeof(RID),
			header.freeSpaceOffset - (offset + currentKeyLength + sizeof(RID))
			);

	// Updates the header.
	header.freeSpaceOffset -= currentKeyLength + sizeof(RID);
	header.recordsNumber -= 1;
	setLeafPageHeader(pageData, header);

	return 0;
}

RC IndexManager::deleteEntry(FileHandle &fileHandle, const Attribute &attribute, const void *key, const RID &rid)
{
	// Simplification: Lazy deletion.

	// Starts a recursive search on the tree, looking for the leaf page which should contain the entry to be deleted.
	unsigned leafPageID;
	RC result = treeSearch(fileHandle, attribute, key, getRootPageID(fileHandle), leafPageID);
	if (result != SUCCESS)
		return result;

	// Retrieves the found leaf page.
	void * pageData = malloc(PAGE_SIZE);
	if (fileHandle.readPage(leafPageID, pageData) != SUCCESS)
		return ERROR_PFM_READPAGE;

	// Tries to delete the entry.
	result = deleteEntryFromLeaf(attribute, key, rid, pageData);
	if (result != SUCCESS)
		return result;

	// Writeback.
	if (fileHandle.writePage(leafPageID, pageData) != SUCCESS)
		return ERROR_PFM_WRITEPAGE;

	free(pageData);
	return 0;
}

// Recursive search through the tree, returning the page ID of the leaf page that should contain the input key.
RC IndexManager::treeSearch(FileHandle &fileHandle, const Attribute attribute, const void * key, unsigned currentPageID, unsigned &returnPageID)
{
	void * pageData = malloc(PAGE_SIZE);

	// Gets the current page.
	if (fileHandle.readPage(currentPageID, pageData) != SUCCESS)
		return ERROR_PFM_READPAGE;

	// If it's a leaf page, we're done. Returns its id.
	if (isLeafPage(pageData))
	{
		returnPageID = currentPageID;
		free(pageData);
		return SUCCESS;
	}

	// Otherwise, we go one level below (towards the correct son page) and call the method again.
	unsigned sonPageID = getSonPageID(attribute, key, pageData);

	free(pageData);

	return treeSearch(fileHandle, attribute, key, sonPageID, returnPageID);
}

RC IndexManager::scan(FileHandle &fileHandle,
    const Attribute &attribute,
    const void      *lowKey,
    const void      *highKey,
    bool			lowKeyInclusive,
    bool        	highKeyInclusive,
    IX_ScanIterator &ix_ScanIterator)
{
	// Checks the file descriptor.
	if (fileHandle.getFileDescriptor() == NULL)
		return ERROR_PFM_FILEHANDLE;

	// Starts a recursive search on the tree.
	unsigned firstLeafPageID;
	RC result = treeSearch(fileHandle, attribute, lowKey, getRootPageID(fileHandle), firstLeafPageID);
	if (result != SUCCESS)
		return result;

	void * pageData = malloc(PAGE_SIZE);

	// Gets the first leaf page which could contain the required results.
	if (fileHandle.readPage(firstLeafPageID, pageData) != SUCCESS)
		return ERROR_PFM_READPAGE;

	// Start collecting results.

	// Initialize the vectors that will be returned through the IX_ScanIterator.
	vector<RID> rids;
	vector<int> keyVectorSizes;
	vector<void*> keyVector;

	// I start looking at the first leaf page that I got. I don't know yet if it will be enough.
	bool lookForNextPage = true;
	while(lookForNextPage)
	{
		// Cycles through all the records of the page.
		LeafPageHeader leafHeader = getLeafPageHeader(pageData);
		unsigned offset = sizeof(PageType) + sizeof(LeafPageHeader);
		bool includeCurrentRecord;
		for(unsigned i = 0; i < leafHeader.recordsNumber; i++)
		{
			includeCurrentRecord = false;

			// Gets the current key length.
			int currentKeyLength = getKeyLength(attribute, (char*) pageData + offset);

			// Compares the input keys with the current key, looking for records that meet the search bounds.
			if	(
					(
					(lowKeyInclusive && compareKeys(attribute, lowKey, (char*) pageData + offset) <= 0) ||
					(!lowKeyInclusive && compareKeys(attribute, lowKey, (char*) pageData + offset) < 0)
					)
				&&
					(
					(highKeyInclusive && compareKeys(attribute, (char*) pageData + offset, highKey) <= 0) ||
					(!highKeyInclusive && compareKeys(attribute, (char*) pageData + offset, highKey) < 0)
					)
				)
			{
				// If we are here, this entry has to be included in the results. Appends it to the output vector.
				includeCurrentRecord = true;

				RID returnRid;

				void * keyData;
				keyVectorSizes.push_back(currentKeyLength);

				keyData = malloc(currentKeyLength);
				memcpy(keyData, (char*) pageData + offset, currentKeyLength);
				keyVector.push_back(keyData);

				memcpy(&returnRid, (char*) pageData + offset + currentKeyLength, sizeof(RID));
				rids.push_back(returnRid);
			}

			// Offset increment.
			offset += currentKeyLength + sizeof(RID);
		}

		// If the last record of the current page is included in the results (and a next page exists), I must continue the search within the next page.
		if(includeCurrentRecord && leafHeader.nextPage != NULL_PAGE_ID)
		{
			lookForNextPage = true;

			if (fileHandle.readPage(leafHeader.nextPage, pageData) != SUCCESS)
				return ERROR_PFM_READPAGE;
		}
		else
			lookForNextPage = false;
	}

	// Return the results through the ScanIterator.
	RBFM_ScanIterator rbfm_ScanIterator;
	rbfm_ScanIterator.setVectors(rids, keyVectorSizes, keyVector);
	ix_ScanIterator = IX_ScanIterator(rbfm_ScanIterator);

	free(pageData);

	return 0;
}

void IX_PrintError (RC rc)
{
	switch (rc)
	{
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
