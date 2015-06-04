#ifndef _rm_h_
#define _rm_h_

#include <string>
#include <vector>

#include "../rbf/rbfm.h"
#include "../ix/ix.h"

using namespace std;

#define TABLE_FILE_EXTENSION				".t"
#define TABLES_TABLE_NAME					"Tables"
#define TABLES_TABLE_ID						1

#define TABLES_COL_TABLE_ID					"table-id"
#define TABLES_COL_TABLE_NAME				"table-name"
#define TABLES_COL_TABLE_NAME_SIZE			40
#define TABLES_COL_FILE_NAME				"file-name"
#define TABLES_COL_FILE_NAME_SIZE			40

#define TABLES_RECORD_DATA_SIZE 			INT_SIZE + TABLES_COL_TABLE_NAME_SIZE + TABLES_COL_FILE_NAME_SIZE

#define COLUMNS_TABLE_NAME					"Columns"
#define COLUMNS_TABLE_ID					2

#define COLUMNS_COL_TABLE_ID				"table-id"
#define COLUMNS_COL_COLUMN_NAME				"column-name"
#define COLUMNS_COL_COLUMN_NAME_SIZE		40
#define COLUMNS_COL_COLUMN_TYPE				"column-type"
#define COLUMNS_COL_COLUMN_LENGTH			"column-length"

#define COLUMNS_RECORD_DATA_SIZE			INT_SIZE * 3 + COLUMNS_COL_COLUMN_NAME_SIZE
#define RM_EOF (-1)  // end of a scan operator

// RM_ScanIterator: Simple wrapper for RBFM_ScanIterator.
class RM_ScanIterator {
private:
  RBFM_ScanIterator rbfm_SI;

public:
	RM_ScanIterator() {}
	RM_ScanIterator(RBFM_ScanIterator &r) {this->rbfm_SI = r;}
	~RM_ScanIterator() {}

	// "data" follows the same format as RelationManager::insertTuple()
	int getNextTuple(RID &rid, void *data) {
		return rbfm_SI.getNextRecord(rid, data);
	}
	int close() {
		cout << "***" << endl;
		return rbfm_SI.close();
	}
};

typedef IX_ScanIterator RM_IndexScanIterator;

// Relation Manager
class RelationManager {
private:
	static RelationManager *_rm;
	static RecordBasedFileManager *_rbfm;
	static IndexManager *_ixm;
	// Defines.
	string t_tables;
	string t_columns;

	// Auxiliary methods.
	int getTableID(const string &tableName, int &tableID);

	static vector<Attribute> getTablesRecordDescriptor();
	static vector<Attribute> getColumnsRecordDescriptor();
	static void prepareTablesRecordData(int id, string tableName, void * outputData);
	static void prepareColumnsRecordData(int tableID, Attribute attr, void * outputData);

protected:
	RelationManager();
	~RelationManager();

public:
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

	// scan returns an iterator to allow the caller to go through the results one by one.
	int scan(const string &tableName,
      const string &conditionAttribute,
      const CompOp compOp,                  // comparision type such as "<" and "="
      const void *value,                    // used in the comparison
      const vector<string> &attributeNames, // a list of projected attributes
      RM_ScanIterator &rm_ScanIterator);

	int createIndex(const string &tableName, const string &attributeName);
	int destroyIndex(const string &tableName, const string &attributeName);
	// indexScan returns an iterator to allow the caller to go through qualified entries in index
	int indexScan(const string &tableName,
                        const string &attributeName,
                        const void *lowKey,
                        const void *highKey,
                        bool lowKeyInclusive,
                        bool highKeyInclusive,
                        RM_IndexScanIterator &rm_IndexScanIterator);


	// Extra credit
	int dropAttribute(const string &tableName, const string &attributeName);
	int addAttribute(const string &tableName, const Attribute &attr);
	int reorganizeTable(const string &tableName);
};

#endif
