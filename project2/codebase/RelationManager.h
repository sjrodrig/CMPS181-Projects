#include "RecordBasedFile.h"

/**
 * @modifyer: Benjamin (Benjy) Strauss
 *
 * 
 */

#define user "usr_"
#define system "sys_"

class RM_ScanIterator {
public:
  RM_ScanIterator() {};
  RM_ScanIterator(RBFM_ScanIterator &r) {this->rbfm_SI = r;};
  ~RM_ScanIterator() {};

  // "data" follows the same format as RelationManager::insertTuple()
  RC setVectors(vector<RID> rids, vector<void*> dataVector) {
  	return rbfm_SI.setVectors(rids, dataVector);
  };
  RC getNextTuple(RID &rid, void *data) {
	return rbfm_SI.getNextRecord(rid, data);
  };
  RC close() {
	return rbfm_SI.close();
  };

private:
  RBFM_ScanIterator rbfm_SI;
};

class RelationManager {
private:
	RecordBasedFileManager sysTableHandler;
	string tables_table_name;
	string columns_table_name;
	int tableIDs;
	//every table has this attribute exactly the same
	Attribute TableID;

protected:
public:
	RelationManager();
	~RelationManager();
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
}; 




