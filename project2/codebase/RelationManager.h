#ifndef _RelationManager_h_
#define _RelationManager_h_
#include "RecordBasedFile.h"

/**
 * RelationManager.h
 * CMPS181 - Spring 2015
 * Project 2
 *
 * Benjamin (Benjy) Strauss
 * Paul-Valentin Mini (pcamille)
 */


#define user "usr_"
#define system "sys_"
#define NAME_LEN 40
#define RM_EOF -1

class RM_ScanIterator {
private:
  RBFM_ScanIterator rbfm_SI;

public:
	RM_ScanIterator() {}
	RM_ScanIterator(RBFM_ScanIterator &r) { this->rbfm_SI = r; }
	~RM_ScanIterator() {}

	// "Data" follows the same format as RelationManager::insertTuple()
	int setVectors(vector<RID> rids, vector<void*> dataVector) {
  		return rbfm_SI.setVectors(rids, dataVector);
	}
	int getNextTuple(RID &rid, void *data) {
		return rbfm_SI.getNextRecord(rid, data);
	}
	int close() { return rbfm_SI.close(); }
};

class RelationManager {
private:
	static RelationManager *_rm_manager;
	PagedFileManager* myPFM;
	RecordBasedFileManager sysTableHandler;
	string tables_table_name;
	string columns_table_name;

	// Attributes and their names for the system tables
	vector<Attribute> tabAttrs;
	vector<Attribute> colAttrs;
	vector<string> tabNames;
	vector<string> colNames;

	int tableIDs;
	int testFileForEmptiness(string data);
	void printRawData(unsigned char* data, int len);
	void printRawToFile(unsigned char* data, int len, bool all);

protected:
public:
	RelationManager();
	~RelationManager();
	static RelationManager* instance();
	int createTable(const string &tableName, const vector<Attribute> &attrs);
	int deleteTable(const string &tableName);
	int getAttributes(const string &tableName, vector<Attribute> &attrs);
	int insertTuple(const string &tableName, const void *data, RID &rid);
	int deleteTuples(const string &tableName);
	int deleteTuple(const string &tableName, const RID &rid);
	// Assume the rid does not change after update
	int updateTuple(const string &tableName, const void *data, const RID &rid);
	int readTuple(const string &tableName, const RID &rid, void *data);
	int readAttribute(const string &tableName, const RID &rid, const string &attributeName, void *data);
	int reorganizePage(const string &tableName, const unsigned pageNumber);

	// Scan returns an iterator to allow the caller to go through the results one by one. 
	int scan(const string &tableName,
		const string &conditionAttribute,
		const CompOp compOp,                  // comparision type such as "<" and "="
		const void *value,                    // used in the comparison
		const vector<string> &attributeNames, // a list of projected attributes
		RM_ScanIterator &rm_ScanIterator);
}; 
#endif
