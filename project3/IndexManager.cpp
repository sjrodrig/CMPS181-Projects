#include "IndexManager.h"
#include <iostream>
#include <string.h>
#include <stdexcept>
#include <stdio.h>
#include <algorithm>

/**
 * CMPS 181 - Project 3
 * Author: Paolo Di Febbo
 * Modifier: Benjy Strauss
 * File description: Implementing a B+ tree indexing system.
 *						(ref. p. 344-351 Ramakrishnan - Gehrke).
 */

using namespace std;

// Given a ChildEntry structure (<key, child page id>), writes it into the correct position within the non leaf page "pageData".
int
IndexManager::insertNonLeafRecord(const Attribute &attribute, ChildEntry &newChildEntry, void* pageData) {
	return -1;
}

// Given a record entry (<key, RID>), writes it into the correct position within the leaf page "pageData".
int
IndexManager::insertLeafRecord(const Attribute &attribute, const void *key, const RID &rid, void* pageData) {
	return -1;
}

// Recursive insert of the record <key, rid> into the (current) page "pageID".
// newChildEntry will store the return information of the "child" insert call.
// Following the exact implementation described in Ramakrishnan - Gehrke, p.349.
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
	return -1;
}

int
IndexManager::scan(FileHandle &fileHandle, const Attribute &attribute, const void *lowKey, const void *highKey, bool lowKeyInclusive, bool highKeyInclusive, IX_ScanIterator &ix_ScanIterator) {
	return -1;
}

// Given a non-leaf page and a key, finds the correct (direct) son page ID in which the key "fits".
unsigned
IndexManager::getSonPageID(const Attribute attribute, const void * key, void * pageData) {
	return -1;
}

int
IndexManager::createFile(const string &fileName) {
	return -1;
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
