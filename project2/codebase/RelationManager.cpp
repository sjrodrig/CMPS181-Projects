#include "RelationManager.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <bitset>

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

/** Progress:
 * Constructor() 		Complete + Tested
 * Destructior() 		Complete
 * createTable() 		Complete + Tested
 * deleteTable() 		Complete + Lightly Tested
 * getAttributes() 		Complete + Tested
 * insertTuple() 		Complete
 * deleteTuples() 		Complete
 * deleteTuple() 		Complete
 * updateTuple() 		Complete
 * readTuple() 			Complete
 * readAttribute() 		Complete
 * reorganizePage() 	>>Unstarted
 * scan()				Complete
 */

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

	//Table Attributes
	Attribute TableID;
	TableID.name = "TableID";
	TableID.type = TypeInt;
	TableID.length = 4;

	Attribute TableName;
	TableName.name = "TableName";
	TableName.type = TypeVarChar;
	TableName.length = NAME_LEN;

	Attribute FileName;
	FileName.name = "FileName";
	FileName.type = TypeVarChar;
	FileName.length = (NAME_LEN + 8);

	//Column Attributes
	Attribute ColumnName;
	ColumnName.name = "ColumnName";
	ColumnName.type = TypeVarChar;
	ColumnName.length = NAME_LEN;		//get the size of the name

	Attribute ColumnType;
	ColumnType.name = "ColumnType";
	ColumnType.type = TypeInt;
	ColumnType.length = 4;

	Attribute ColumnLength;
	ColumnLength.name = "ColumnLength";
	ColumnLength.type = TypeInt;
	ColumnLength.length = 4;

	tabAttrs.push_back(TableID);
	tabAttrs.push_back(TableName);
	tabAttrs.push_back(FileName);

	colAttrs.push_back(TableID);
	colAttrs.push_back(ColumnName);
	colAttrs.push_back(ColumnType);
	colAttrs.push_back(ColumnLength);

	//table names
	tabNames.push_back("TableID");
	tabNames.push_back("TableName");
	tabNames.push_back("FileName");

	colNames.push_back("TableID");
	colNames.push_back("ColumnName");
	colNames.push_back("ColumnType");
	colNames.push_back("ColumnLength");

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

//destructor
RelationManager::~RelationManager() {
	//Nothing TODO
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

	FILE * ttable = fopen(tables_table_name.c_str(), "r+");
	tableTable_handle.setFileDescriptor(ttable);

	//assemble the data
	int data_len = ((NAME_LEN * 2) + 20);
	//cout << "data_len is: " << data_len << endl;
	unsigned char* tableEntryData = new unsigned char[data_len];
	for(int init = 0; init < data_len; init++) { tableEntryData[init] = 127; }

	const char* tname = tableName.c_str();
	//cout << "tname is: " << tname << endl;

	tableIDs++;
	int new_table_id = tableIDs;
	int aa;

	//This is an integer--set it's value
	for(aa = 0; aa < 4; aa++) {
		unsigned char meta = (new_table_id % 256);
		tableEntryData[aa] = meta;
		new_table_id /= 256;
	}

	int meta = NAME_LEN;
	//cout << "meta: " << meta << endl; 
	for(; aa < 8; aa++) {
		tableEntryData[aa] = (meta % 256);
		meta /= 256;
	}

	for(int bb = 0; tname[bb] != '\0'; aa++, bb++) {
		tableEntryData[aa] = tname[bb];
	}

	for(; aa < 48; aa++) {
		tableEntryData[aa] = 0;
	}
	//put the index in the right place
	aa = 48;

	int cc = aa + 4;

	meta = (NAME_LEN + 8);
	//cout << "meta: " << meta << endl; 

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

	for(; aa < data_len; aa++) {
		tableEntryData[aa] = 0;
	}

	if(DEBUG) {
		cout << "inserting data:" << endl;
		printRawData(tableEntryData, data_len);
	}
	int retVal;
	retVal = sysTableHandler.insertRecord(tableTable_handle, tabAttrs, tableEntryData, dummyRID);
	if( retVal != SUCCESS ) {
		cout << "failing with: " << retVal << endl;
		return retVal;
	}

	//handle for the columns table
	FileHandle columnTable_handle;

	FILE * ctable = fopen(columns_table_name.c_str(), "r+");
	columnTable_handle.setFileDescriptor(ctable);

	//for loop to add all the columns to the columns table
	for(unsigned int index = 0; index < attrs.size(); index++) {
		//cout << "looping: " << index << endl;

		string colName = attrs.at(index).name;
		//cout << "colName is: " << colName << endl;

		unsigned char* columnEntryData = new unsigned char[NAME_LEN+16];
		int ee;

		//enter the tableID
		for(ee = 0; ee < 4; ee++) {
			columnEntryData[ee] = tableEntryData[ee];
		}

		//enter the number preceeding the columnName
		int temp = NAME_LEN;

		for(; ee < 8; ee++) {
			columnEntryData[ee] = (temp % 256);
			temp /= 256;
		}

		//enter the column's name
		const char* cName = colName.c_str();
		for(; cName[ee-8] != 0; ee++) {
			columnEntryData[ee] = cName[ee-8];
		}

		//fill everything else with zeros
		for(; ee < 48; ee++) {
			columnEntryData[ee] = 0;
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
		sysTableHandler.insertRecord(columnTable_handle, colAttrs, columnEntryData, dummyRID);
	}

	//cout << "ins-ret: " << retVal << endl;
	return retVal;
}

/**
 * @COMPLETE
 * This method deletes a table called tableName. 
 */
int
RelationManager::deleteTable(const string &tableName) {
	//a table ID which will be used for finding the columns to delete
	int retVal;
	RID tableRID, columnRID;
	RBFM_ScanIterator tableRecordIter, colRecordIter;
	int tableID = 0;
	int control = 0;

	//delete the file
	string tableFileName = user + tableName + ".tab";
	sysTableHandler.destroyFile(tableFileName);

	//the handle for the tables table to delete the table
	FileHandle tTable_handle;
	FILE * ttable = fopen(tables_table_name.c_str(), "r+");
	tTable_handle.setFileDescriptor(ttable);

	//the handle for the columns table
	FileHandle cTable_handle;
	FILE * ctable = fopen(columns_table_name.c_str(), "r+");
	cTable_handle.setFileDescriptor(ctable);

	//the attribute vectors that will be used to erase the table and it's columns
	const vector<string> table_attrNames = tabNames;
	const vector<Attribute> table_recordDescriptor = tabAttrs;
	const vector<string> column_attrNames = colNames;
	const vector<Attribute> column_recordDescriptor = colAttrs;

	//get the table's RID + data
	retVal = sysTableHandler.scan(tTable_handle, table_recordDescriptor, "TableName", EQ_OP, &tableName, table_attrNames, tableRecordIter);

	if( retVal != SUCCESS ) {
		cout << "(RelationManager::deleteTable) Couldn't find table " << tableName << " in table's table." << endl;
		cout << "Failing with: " << retVal << endl;
		return retVal;
	}

	unsigned char* metaPage = new unsigned char[PAGE_SIZE];					//page
	unsigned char* tableRecord = new unsigned char[(NAME_LEN*2)+20];		//raw data
	unsigned char* columnRecord = new unsigned char[NAME_LEN+16];

	tableRecordIter.getNextRecord(tableRID, metaPage);

	sysTableHandler.readRecord(tTable_handle, table_recordDescriptor, tableRID, tableRecord);
	//delete the table from the table of tables
	sysTableHandler.deleteRecord(tTable_handle, table_recordDescriptor, tableRID);

	memcpy(&tableID, tableRecord, 4);

	//get the columns's RID + data
	retVal = sysTableHandler.scan(cTable_handle, column_recordDescriptor, "TableName", EQ_OP, &tableName, column_attrNames, colRecordIter);

	if( retVal != SUCCESS ) {
		//cout << "(RelationManager::deleteTable) Couldn't find table " << tableName << " in table's table." << endl;
		cout << "Failing with: " << retVal << endl;
		return retVal;
	}

	//delete the table's columns from the table of columns
	control = colRecordIter.getNextRecord(columnRID, metaPage);

	while(control == 0) {
		sysTableHandler.readRecord(cTable_handle, column_recordDescriptor, columnRID, columnRecord);
//printRawData(columnRecord, 56);
		sysTableHandler.deleteRecord(cTable_handle, column_recordDescriptor, columnRID);
		control = colRecordIter.getNextRecord(columnRID, metaPage);
	}

	return retVal;
}

/**
 * COMPLETE
 * This method gets the attributes (attrs) of a table called tableName. 
 */
int
RelationManager::getAttributes(const string &tableName, vector<Attribute> &attrs) {
	int tableID = 0;
	int retVal = -1;

	//make the filehandle and set it to the columns table
	FileHandle tab_handle;
	FILE * ttable = fopen(tables_table_name.c_str(), "r+");
	tab_handle.setFileDescriptor(ttable);

	const vector<Attribute> _table_recordDescriptor = tabAttrs;
	const vector<string> _table_attributeNames = tabNames;

	//the iterator
	RBFM_ScanIterator tableRecordIter;

	//now scan
	retVal = sysTableHandler.scan(tab_handle, _table_recordDescriptor, "TableName", EQ_OP, &tableName, _table_attributeNames, tableRecordIter);

	//check if scan was successful
	if( retVal != SUCCESS ) {
		cout << "(RelationManager::getAttributes) Couldn't find table " << tableName << " in table's table." << endl;
		cout << "Failing with: " << retVal << endl;
		return retVal;
	}

	RID metaRID;															//a return value
	unsigned char* data_value = new unsigned char[PAGE_SIZE];				//the page
	tableRecordIter.getNextRecord(metaRID, data_value);

	unsigned char* dataTuple = new unsigned char[(NAME_LEN * 2) + 20];
	retVal = sysTableHandler.readRecord(tab_handle,  _table_recordDescriptor, metaRID, dataTuple);

	if( retVal != SUCCESS ) {
		cout << "(RelationManager::getAttributes) Couldn't find table " << tableName << " in table's table." << endl;
		cout << "Failing with: " << retVal << endl;
		return retVal;
	}
	//copy the table's id.
	memcpy(&tableID, dataTuple, 4);

	//make the filehandle and set it to the columns table
	FileHandle col_handle;
	FILE * ctable = fopen(columns_table_name.c_str(), "r+");
	col_handle.setFileDescriptor(ctable);

	//the vectors:
	const vector<Attribute> _col_recordDescriptor = colAttrs;
	const vector<string> _col_attributeNames = colNames;

	//the iterator
	RBFM_ScanIterator colRecordIter;

	void* t_ID = malloc(4);
	//copy the table's id.
	memcpy(t_ID, &tableID, 4);

	retVal = sysTableHandler.scan(col_handle, _col_recordDescriptor, "TableID", EQ_OP, t_ID, _col_attributeNames, colRecordIter);

	//if something goes wrong, alert the user
	if( retVal != SUCCESS ) {
		cout << "(RelationManager::getAttributes) Couldn't find columns for " << tableName << " in columns's table." << endl;
	}

	//vector<Attribute> &attrs
	//[TAB_ID-4][NAME-VAR][TYPE-4][LEN-4]
	int control = 0;

	while(control == 0) {
//cout << "Reading Column Data" << endl;
		RID metaRID2;
		unsigned char* dataPage = new unsigned char[PAGE_SIZE];
		unsigned char* data = new unsigned char[NAME_LEN+20];
		control = colRecordIter.getNextRecord(metaRID2, data);

		sysTableHandler.readRecord(col_handle, _col_recordDescriptor, metaRID2, data);

//cout << "reading col data:" << endl;
//printRawData(data,56);

		if(data == NULL) { 
			cout << "ERROR: NULL DATA!" << endl;
		}

		Attribute pushMe;

		//extract the name
		char* attrName = new char[NAME_LEN];
		for(int aa = 8; aa < NAME_LEN+8; aa++) {
			attrName[aa-8] = data[aa];
		}

		//compact and set the name
		for(int aa = 0; attrName[aa] != '\0'; aa++) {
			pushMe.name += attrName[aa];
		}
		cout << "name is: " << pushMe.name << endl;

		//extract the type
		unsigned char* typeData = new unsigned char[4];
		for(int aa = (NAME_LEN + 8); aa < (NAME_LEN + 12); aa++) { typeData[aa-(NAME_LEN+8)] = data[aa]; }
		
		int power = 1;
		int accu = 0;
		for(int bb = 0; bb < 4; bb++) {
			accu += typeData[bb] * power;
			power *= 256;
		}

		if(accu == 0) {
			pushMe.type = TypeInt;
		} else if (accu == 1) {
			pushMe.type = TypeReal;
		} else if (accu == 2) {
			pushMe.type = TypeVarChar;
		} else {
			cout << "ERROR READING ATTRIBUTE TYPE { " << pushMe.type << " }" << endl;
		}

		//extract the length
		unsigned char* lenData = new unsigned char[4];
		for(int aa = (NAME_LEN + 12); aa < (NAME_LEN + 16); aa++) { lenData[aa-(NAME_LEN+12)] = data[aa]; }

		int _power = 1;
		int _accu = 0;
		for(int bb = 0; bb < 4; bb++) {
			_accu += lenData[bb] * _power;
			_power *= 256;
		}

		pushMe.length = _accu;
		cout << "length is: " << pushMe.length << endl;

		attrs.push_back(pushMe);
	}

	return retVal;
}

/**
 * This method inserts a tuple into a table called tableName.
 * You can assume that the input is always correct and free of error.
 * That is, you do not need to check if the input tuple has the right number of attributes and if the attribute types match. 
 */
int
RelationManager::insertTuple(const string &tableName, const void *data, RID &rid) {
	string tableFileName = user + tableName + ".tab";
	//the return value
	int retVal = -1;

	FileHandle insertHandle;
	FILE * tableFile = fopen(tableFileName.c_str(), "r+");
	insertHandle.setFileDescriptor(tableFile);

	vector<Attribute> insertVector;
	getAttributes(tableName, insertVector);
	const vector<Attribute> _insertVector = insertVector;

	retVal = sysTableHandler.insertRecord(insertHandle, _insertVector, data, rid);

	return retVal;
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
	FILE * _clearMe = fopen(tableFileName.c_str(), "r+");
	clearMe.setFileDescriptor(_clearMe);

	retVal = sysTableHandler.deleteRecords(clearMe);

	return retVal;
}

/**
 * COMPLETE
 * 
 * This method deletes a tuple with a given rid. 
 */
int
RelationManager::deleteTuple(const string &tableName, const RID &rid) {
	string tableFileName = user + tableName + ".tab";
	//the return value
	int retVal = -1;

	FileHandle clearMe;
	FILE * _clearMe = fopen(tableFileName.c_str(), "r+");
	clearMe.setFileDescriptor(_clearMe);

	//the Record Descriptor
	vector<Attribute> descriptor;

	//assign the table's attributes to "descriptor"
	getAttributes(tableFileName, descriptor);

	retVal = sysTableHandler.deleteRecord(clearMe, descriptor, rid);

	return retVal;
}

/**
 * Simple delete + insert
 *
 * This method updates a tuple identified by a given rid. Note: if the tuple grows (i.e., the size of tuple increases) and there is no space in the page to store the tuple (after the update), then, the tuple is migrated to a new page with enough free space. Since you will implement an index structure (e.g., B-tree) in project 3, you can assume that tuples are identified by their rids and when they migrate, they leave a tombstone behind pointing to the new location of the tuple.
 */
int
RelationManager::updateTuple(const string &tableName, const void *data, const RID &rid) {
	int retVal;
	retVal = deleteTuple(tableName, rid);

	if(retVal != SUCCESS) { //delete failed
		cout << "Delete Error" << endl;
		return retVal;
	}

	const void* insert_this = data;
	RID ins;
	ins.slotNum = rid.slotNum;
	ins.pageNum = rid.pageNum;

	retVal = insertTuple(tableName, insert_this, ins);

	return retVal;
}

//This method reads a tuple identified by a given rid. 
int
RelationManager::readTuple(const string &tableName, const RID &rid, void *data) {
	int retVal;

	//the handle for the tables table to delete the table
	FileHandle handle;
	string s0 = user + tableName + ".tab";
	FILE * tableFile = fopen(s0.c_str(), "r+");
	handle.setFileDescriptor(tableFile);

	vector<Attribute> tableAttrs;
	getAttributes(tableName, tableAttrs);
	const vector<Attribute> _tableAttrs = tableAttrs;

	retVal = sysTableHandler.readRecord(handle, _tableAttrs, rid, data);
	return retVal;
}

//This method reads a specific attribute of a tuple identified by a given rid. 
int
RelationManager::readAttribute(const string &tableName, const RID &rid, const string &attributeName, void *data) {
	// Open table file
	FileHandle fileHandle;
	string filename = user + tableName + ".tab";
	FILE *table_file = fopen(filename.c_str(), "r+");
	if (table_file == NULL) return -1;
	fileHandle.setFileDescriptor(table_file);

	// Fetch table attributes
	vector<Attribute> recordDescriptor;
	if (getAttributes(tableName, recordDescriptor) != SUCCESS){
		return -2;
	}

	// Read the attribute in the table
	if (sysTableHandler.readAttribute(fileHandle, recordDescriptor, rid, attributeName, data) != SUCCESS){
		return -3;
	}
	return SUCCESS;
}

//This method scans a table called tableName. That is, it sequentially reads all the entries in the table. This method returns an iterator called rm_ScanIterator to allow the caller to go through the records in the table one by one. A scan has a filter condition associated with it, e.g., it consists of a list of attributes to project out as well as a predicate on an attribute (“Sal > 40000”). Note: the RBFM_ScanIterator should not cache the entire scan result in memory. In fact, you need to be looking at one (or a few) page(s) of data at a time, ever. In this project, let the OS do the memory-management work for you. 
int
RelationManager::scan(const string &tableName, const string &conditionAttribute, const CompOp compOp, const void *value, const vector<string> &attributeNames, RM_ScanIterator &rm_ScanIterator) {
	// Open table file
	FileHandle fileHandle;
	string filename = user + tableName + ".tab";
	FILE *table_file = fopen(filename.c_str(), "r+");
	if (table_file == NULL) { return -1; }
	fileHandle.setFileDescriptor(table_file);

	// Fetch table attributes
	vector<Attribute> recordDescriptor;
	if (getAttributes(tableName, recordDescriptor) != SUCCESS) {
		return -2;
	}

	// Initialize iterator for record scan
	RBFM_ScanIterator record_iterator = RBFM_ScanIterator();
	if (sysTableHandler.scan(fileHandle, recordDescriptor, conditionAttribute, compOp, value, attributeNames, record_iterator) != SUCCESS){
		return -3;
	}

	// Set the result iterator & return
	rm_ScanIterator = RM_ScanIterator(record_iterator); 
	return SUCCESS;
}

//This method reorganizes the tuples in a page. That is, it pushes the free space towards the end of the page. Note: In this method you are NOT allowed to change the rids, since they might be used by other external index structures, and it's too expensive to modify those structures for each such a function call. It's OK to keep those deleted tuples and their slots. 
int
RelationManager::reorganizePage(const string &tableName, const unsigned pageNumber) {
	return -1;
}

void
RelationManager::printRawData(unsigned char* data, int len) {
	for(int i = 0; i < len; i++) {
		cout << "[";
		if (i < 10) { cout << "0"; }
		cout << i << "] ";
		unsigned char meta = data[i];
		cout << (bitset<8>) meta;

		/*if(data[i] > 32 && data[i] < 127) {
			char c = data[i];
			cout << c;
		} else if(data[i] == 0) {
			cout << "NULL";
		} else {
			int foo = data[i];
			cout << "'" << foo << "'";
		}
		cout << "} ";*/
		if((i+1) % 4 == 0) { cout << endl; }
	}
	cout << endl;
}

void
RelationManager::printRawToFile(unsigned char* data, int len, bool all) {
	ofstream outfile;
	outfile.open("dumpfile.dump");

	for(int i = 0; i < len; i++) {
		unsigned char meta = data[i];
		if(all || meta != 0) {
			outfile << "[";
			if (i < 10) { outfile << "0"; }
			if (i < 100) { outfile << "0"; }
			if (i < 1000) { outfile << "0"; }
			outfile << i << "] ";
			unsigned char meta = data[i];
			outfile << (bitset<8>) meta;
		}
		if(all && ((i+1) % 4 == 0)) { outfile << endl; }
		if(!all && meta != 0) { outfile << endl; }
	}
	outfile << endl;
}
