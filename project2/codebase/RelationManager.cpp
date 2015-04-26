#include "RelationManager.h"
#include <fstream>
#include <iostream>

#define flag0 "flag #0"
#define flag1 "flag #1"
#define flag2 "flag #2"
#define flag3 "flag #3"
#define flag4 "flag #4"
#define flag5 "flag #5"
#define flag6 "flag #6"
#define flag7 "flag #7"
#define flag8 "flag #8"
#define flag9 "flag #9"
#define flagA "flag #A"

using namespace std;

/**
 * constructor
 *
 * 
 */
RelationManager::RelationManager() {
	tables_table_name = "sys_tables.tab";
	columns_table_name = "sys_columns.tab";
	tableIDs = 0;

	TableID.name = "TableID";
	TableID.type = TypeInt;
	TableID.length = 4;

	ifstream checkStream(tables_table_name.c_str());

    if (checkStream.good()) {
        //Nothing TODO
    } else {
		//make "sys_tables.tab"
		sysTableHandler.createFile(tables_table_name.c_str());
		sysTableHandler.createFile(columns_table_name.c_str());
    }

    checkStream.close();
}

/**
 * @COMPLETED
 * This method creates a table called tableName with a vector of attributes (attrs).
 * 
 * returns 0 on success and 1 on failure
 *
 */
int
RelationManager::createTable(const string &tableName, const vector<Attribute> &attrs) {
	string tableFileName = user + tableName + ".tab";

	int go = sysTableHandler.createFile(tableFileName);
	//couldn't make the file...
	if(go != SUCCESS) { 
		cout << "Couldn't make file for table: \"" << tableName << "\"" << endl;
		return 1;
	}

	//A dummy RID to be returned
	RID dummyRID;

	//handle for the tables table
	FileHandle tableTable_handle;

	FILE * ttable = fopen(tables_table_name.c_str(), "a");
	tableTable_handle.setFileDescriptor(ttable);

	//the meta-data for the table
	vector<Attribute> table_meta_data;
	
	Attribute TableName;
	TableName.name = "TableName";
	TableName.type = TypeVarChar;
	TableName.length = tableName.size();

	Attribute FileName;
	FileName.name = "FileName";
	FileName.type = TypeVarChar;
	FileName.length = (tableName.size() + 8);

	//assemble the vector
	table_meta_data.push_back(TableID);
	table_meta_data.push_back(TableName);
	table_meta_data.push_back(FileName);

	//assemble the data
	int data_len = ((tableName.size() * 2) + 20);
	unsigned char* tableEntryData = new unsigned char[data_len];
	for(int init = 0; init < data_len; init++) { tableEntryData[init] = 127; }

	const char* tname = tableName.c_str();
	//cout << "tname is: " << tname << endl;

	tableIDs++;
	int new_table_id = tableIDs;
	int aa;

	for(aa = 3; aa > -1; aa--) {
		tableEntryData[aa] = (new_table_id % 256);
		new_table_id /= 256;
	}

	aa += 5;
	int meta = tableName.size();
	for(; aa < 8; aa++) {
		tableEntryData[aa] = (meta % 256);
		meta /= 256;
	}

	for(int bb = 0; tname[bb] != '\0'; aa++, bb++) {
		tableEntryData[aa] = tname[bb];
	}

	int cc = aa + 4;

	meta = (tableName.size() + 8);

	for(; aa < cc; aa++) {
		tableEntryData[aa] = (meta % 256);
		meta /= 256;
	}

	tableEntryData[aa] = 'u';
	aa++;
	tableEntryData[aa] = 's';
	aa++;
	tableEntryData[aa] = 'r';
	aa++;
	tableEntryData[aa] = '_';
	aa++;

	for(int bb = 0; tname[bb] != '\0'; aa++, bb++) {
		tableEntryData[aa] = tname[bb];
	}

	tableEntryData[aa] = '.';
	aa++;
	tableEntryData[aa] = 't';
	aa++;
	tableEntryData[aa] = 'a';
	aa++;
	tableEntryData[aa] = 'b';
	aa++;

	//allocate the size of the data
	void* _tableEntryData = tableEntryData;

	sysTableHandler.insertRecord(tableTable_handle, table_meta_data, _tableEntryData, dummyRID);

	//handle for the columns table
	FileHandle columnTable_handle;

	FILE * ctable = fopen(columns_table_name.c_str(), "a");
	columnTable_handle.setFileDescriptor(ctable);

	//for loop to add all the columns to the columns table
	for(unsigned int index = 0; index < attrs.size(); index++) {
		//cout << "looping: " << index << endl;

		string colName = attrs.at(index).name;
		//cout << "colName is: " << colName << endl;

		//set up the attributes for each column
		Attribute ColumnName;
		ColumnName.name = "ColumnName";
		ColumnName.type = TypeVarChar;
		ColumnName.length = colName.size();		//get the size of the name

		Attribute ColumnType;
		ColumnType.name = "ColumnType";
		ColumnType.type = TypeInt;
		ColumnType.length = 4;

		Attribute ColumnLength;
		ColumnLength.name = "ColumnLength";
		ColumnLength.type = TypeInt;
		ColumnLength.length = 4;

		//set up the attribute vector
		vector<Attribute> column_meta_data;
		column_meta_data.push_back(TableID);
		column_meta_data.push_back(ColumnName);
		column_meta_data.push_back(ColumnType);
		column_meta_data.push_back(ColumnLength);

		unsigned char* columnEntryData = new unsigned char[ColumnName.length+16];
		int ee;

		//enter the tableID
		for(ee = 0; ee < 4; ee++) {
			columnEntryData[ee] = tableEntryData[ee];
		}

		//enter the number preceeding the columnName
		int temp = colName.size();

		for(; ee < 8; ee++) {
			columnEntryData[ee] = (temp % 256);
			temp /= 256;
		}

		//enter the column's name
		const char* cName = colName.c_str();
		for(; cName[ee-8] != 0; ee++) {
			columnEntryData[ee] = cName[ee-8];
		}

		//enter the column's type
		int dd;
		int meta2 = attrs.at(index).type;
		for(dd = 0; dd < 4; dd++) {
			columnEntryData[ee+dd] = (meta2 % 256);
			meta2 /= 256;
		}

		//enter the column's length
		int meta3 = attrs.at(index).length;
		for(; dd < 8; dd++) {
			columnEntryData[ee+dd] = (meta3 % 256);
			meta3 /= 256;
		}

		void* _columnEntryData = columnEntryData;
		sysTableHandler.insertRecord(columnTable_handle, column_meta_data, columnEntryData, dummyRID);
	}
	return SUCCESS;
}

/**
 * @INCOMPLETE
 * This method deletes a table called tableName. 
 */
int
RelationManager::deleteTable(const string &tableName) {
	//a table ID which will be used for finding the columns to delete
	int delete_this_ID;

	//delete the file
	string tableFileName = user + tableName + ".tab";
	sysTableHandler.destroyFile(tableFileName);

	//the handle for the tables table to delete the table
	FileHandle tTable_handle;
	FILE * ttable = fopen(tables_table_name.c_str(), "a");
	tTable_handle.setFileDescriptor(ttable);

	//the handle for the columns table
	FileHandle cTable_handle;
	FILE * ctable = fopen(columns_table_name.c_str(), "a");
	cTable_handle.setFileDescriptor(ctable);

	//the attribute vector that will be used to erase the table
	vector<Attribute> tableEraser;

	Attribute TableName;
	TableName.name = "TableName";
	TableName.type = TypeVarChar;
	TableName.length = tableName.size();

	Attribute FileName;
	FileName.name = "FileName";
	FileName.type = TypeVarChar;
	FileName.length = (tableName.size() + 8);

	//assemble the vector
	tableEraser.push_back(TableID);
	tableEraser.push_back(TableName);
	tableEraser.push_back(FileName);

	/** @PSUEDO-CODE
	 * Going to have to get the (table's) RID someway (scan)?
	 * Read the table's ID
	 * Scan the columns for all columns with the table's ID
	 * delete those columns
	 * call the following method on the table
	 * call the following method on each column with the table's ID
	 * sysTableHandler.deleteRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid)
	 */

	return SUCCESS;
}

//This method gets the attributes (attrs) of a table called tableName. 
int
RelationManager::getAttributes(const string &tableName, vector<Attribute> &attrs) {
	return -1;
}

//This method inserts a tuple into a table called tableName. You can assume that the input is always correct and free of error. That is, you do not need to check if the input tuple has the right number of attributes and if the attribute types match. 
int
RelationManager::insertTuple(const string &tableName, const void *data, RID &rid) {
	return -1;
}

/**
 * COMPLETE BUT UNTESTED!!!
 *
 * This method deletes all tuples in a table called tableName.
 * This command should result in an empty table.
 */ 
int
RelationManager::deleteTuples(const string &tableName) {
	string tableFileName = user + tableName + ".tab";
	//the return value
	int retVal = -1;

	FileHandle clearMe;
	FILE * _clearMe = fopen(tableFileName.c_str(), "a");
	clearMe.setFileDescriptor(_clearMe);

	retVal = sysTableHandler.deleteRecords(clearMe);

	return retVal;
}

/**
 * UNDER CONSTRUCTION!!!
 * Do NOT call this method!!!
 * 
 * descriptor does not have any attributes; it needs some way of getting the appropriate values.
 * 
 * 
 * 
 * This method deletes a tuple with a given rid. 
 */
int
RelationManager::deleteTuple(const string &tableName, const RID &rid) {
	string tableFileName = user + tableName + ".tab";
	//the return value
	int retVal = -1;

	FileHandle clearMe;
	FILE * _clearMe = fopen(tableFileName.c_str(), "a");
	clearMe.setFileDescriptor(_clearMe);

	//the Record Descriptor
	const vector<Attribute> descriptor;

	/**
	 * Need code here to assign attributes to "descriptor"
	 * 
	 */

	retVal = sysTableHandler.deleteRecord(clearMe, descriptor, rid);

	return retVal;
}

//This method updates a tuple identified by a given rid. Note: if the tuple grows (i.e., the size of tuple increases) and there is no space in the page to store the tuple (after the update), then, the tuple is migrated to a new page with enough free space. Since you will implement an index structure (e.g., B-tree) in project 3, you can assume that tuples are identified by their rids and when they migrate, they leave a tombstone behind pointing to the new location of the tuple.
int
RelationManager::updateTuple(const string &tableName, const void *data, const RID &rid) {
	return -1;
}

//This method reads a tuple identified by a given rid. 
int
RelationManager::readTuple(const string &tableName, const RID &rid, void *data) {
	return -1;
}

//This method reads a specific attribute of a tuple identified by a given rid. 
int
RelationManager::readAttribute(const string &tableName, const RID &rid, const string &attributeName, void *data) {
	return -1;
}

//This method reorganizes the tuples in a page. That is, it pushes the free space towards the end of the page. Note: In this method you are NOT allowed to change the rids, since they might be used by other external index structures, and it's too expensive to modify those structures for each such a function call. It's OK to keep those deleted tuples and their slots. 
int
RelationManager::reorganizePage(const string &tableName, const unsigned pageNumber) {
	return -1;
}

//This method scans a table called tableName. That is, it sequentially reads all the entries in the table. This method returns an iterator called rm_ScanIterator to allow the caller to go through the records in the table one by one. A scan has a filter condition associated with it, e.g., it consists of a list of attributes to project out as well as a predicate on an attribute (“Sal > 40000”). Note: the RBFM_ScanIterator should not cache the entire scan result in memory. In fact, you need to be looking at one (or a few) page(s) of data at a time, ever. In this project, let the OS do the memory-management work for you. 
int
RelationManager::scan(const string &tableName, const string &conditionAttribute, const CompOp compOp, const void *value, const vector<string> &attributeNames, RM_ScanIterator &rm_ScanIterator) {
	return -1;
}

/*********************************************************************************
 *										Iterator								 *
 *********************************************************************************/

//This method is used to get the next tuple from the scanned table. It returns RM_EOF when all tuples are scanned. 
int
RM_ScanIterator::getNextTuple(RID &rid, void *data) {
	return -1;
}

//This method is used to close the iterator. 
int
RM_ScanIterator::close() {
	return -1;
}


