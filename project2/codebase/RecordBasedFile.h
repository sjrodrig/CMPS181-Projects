#ifndef _RecordBasedFile_h_
#define _RecordBasedFile_h_
#include <string>
#include <vector>
#include "FileManager.h"

#define INT_SIZE 4
#define REAL_SIZE 4
#define VARCHAR_LENGTH_SIZE 4

/**
 * @modifyer: Benjamin (Benjy) Strauss
 *
 * 
 */

typedef unsigned AttrLength;
using namespace std;

// Record ID
typedef struct {
	unsigned pageNum;
	unsigned slotNum;

	bool operator != (R r) {
		return (pageNum != r.pageNum) || (slotNum != r.slotNum);
	}
} RID;

typedef struct {
  unsigned freeSpaceOffset;
  unsigned recordEntriesNumber;
} SlotDirectoryHeader;

enum RecordEntryType {Alive, Dead, Tombstone};

typedef struct SDRE
{
  enum RecordEntryType recordEntryType;
  // A slot directory record entry can either contain:
  // 1. Length and offset of the actual record, or
  // 2. RID where the record was moved, in case of tombstone
  union {
	  struct {unsigned length; unsigned offset;};
	  RID tombStoneRID;
  };
} SlotDirectoryRecordEntry;

// Attribute
typedef enum { TypeInt = 0, TypeReal, TypeVarChar } AttrType;

typedef struct {
    string   name;     // attribute name
    AttrType type;     // attribute type
    unsigned length; // attribute length
} Attribute;

// Comparison Operator (NOT needed for part 1 of the project)
typedef enum { 
	EQ_OP = 0,  // =
	LT_OP,      // <
	GT_OP,      // >
 	LE_OP,      // <=
	GE_OP,      // >=
	NE_OP,      // !=
	NO_OP       // no condition
} CompOp;

/*********************************************************************************
 * The scan iterator is NOT required to be implemented for part 1 of the project *
 *********************************************************************************/

# define RBFM_EOF (-1)  // end of a scan operator
/* RBFM_ScanIterator is an iteratr to go through records
 * The way to use it is like the following:
 *  RBFM_ScanIterator rbfmScanIterator;
 *  rbfm.open(..., rbfmScanIterator);
 *  while (rbfmScanIterator(rid, data) != RBFM_EOF) {
 *    process the data;
 *  }
 *  rbfmScanIterator.close();*/

class RBFM_ScanIterator {
public:
	RBFM_ScanIterator() {};
	~RBFM_ScanIterator() {};

	// "data" follows the same format as RecordBasedFileManager::insertRecord()
	int getNextRecord(RID &rid, void *data) { return RBFM_EOF; };
	int close() { return -1; };
};

class RecordBasedFileManager {
private:
	static RecordBasedFileManager *_rbf_manager;
	PagedFileManager *_pf_manager;

	void newRecordBasedPage(void * page);
	SlotDirectoryHeader getSlotDirectoryHeader(void * page);
	void setSlotDirectoryHeader(void * page, SlotDirectoryHeader slotHeader);
	SlotDirectoryRecordEntry getSlotDirectoryRecordEntry(void * page, unsigned recordEntryNumber);
	void setSlotDirectoryRecordEntry(void * page, unsigned recordEntryNumber, SlotDirectoryRecordEntry recordEntry);

	unsigned getPageFreeSpaceSize(void * page);
	unsigned getRecordSize(const vector<Attribute> &recordDescriptor, const void *data);
protected:
	RecordBasedFileManager();
	~RecordBasedFileManager();
public:
	static RecordBasedFileManager* instance();
	int createFile(const string &fileName);
	int destroyFile(const string &fileName);
	int openFile(const string &fileName, FileHandle &fileHandle);
	int closeFile(FileHandle &fileHandle);

	/**  Format of the data passed into the function is the following:
	 *  1) data is a concatenation of values of the attributes
	 *  2) For int and real: use 4 bytes to store the value;
	 *     For varchar: use 4 bytes to store the length of characters, then store the actual characters.
	 *  !!!The same format is used for updateRecord(), the returned data of readRecord(), and readAttribute()
	 */
	int insertRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, RID &rid);
	int readRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, void *data);
	// This method will be mainly used for debugging/testing
	int printRecord(const vector<Attribute> &recordDescriptor, const void *data);
	/*************************************************************
	* IMPORTANT, PLEASE READ: All methods below this comment are *
	*  NOT required to be implemented for part 1 of the project  *
	**************************************************************/
	int deleteRecords(FileHandle &fileHandle);
	int deleteRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid);
	// Assume the rid does not change after update
	int updateRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, const RID &rid);
	int readAttribute(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, const string attributeName, void *data);
	int reorganizePage(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const unsigned pageNumber);
	// scan returns an iterator to allow the caller to go through the results one by one. 
	int scan(FileHandle &fileHandle,
		const vector<Attribute> &recordDescriptor,
		const string &conditionAttribute,
		const CompOp compOp,                  // comparision type such as "<" and "="
		const void *value,                    // used in the comparison
		const vector<string> &attributeNames, // a list of projected attributes
		RBFM_ScanIterator &rbfm_ScanIterator);

// Extra credit for part 2 of the project, please ignore for part 1 of the project
	int reorganizeFile(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor);
};

#endif
