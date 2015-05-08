/**
 * RelationManager.cpp
 * CMPS181 - Spring 2015
 * Project 2
 *
 * Benjamin (Benjy) Strauss
 * Paul-Valentin Mini (pcamille)
 */


#include "RelationManager.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <bitset>

#define f0 "flag #0"
#define f1 "flag #1"
#define f2 "flag #2"
#define f3 "flag #3"
#define f4 "flag #4"
#define f5 "flag #5"
#define f6 "flag #6"
#define f7 "flag #7"
#define f8 "flag #8"
#define f9 "flag #9"
#define fA "flag #A"

using namespace std;


/**
 * Relation Manager Instance Manager
 */
RelationManager* RelationManager::_rm_manager = 0;

RelationManager* RelationManager::instance() {
	if (!_rm_manager) { _rm_manager = new RelationManager(); }
	return _rm_manager;
}

/**
 * Constructor
 */
RelationManager::RelationManager() {
	_rm_manager = NULL;
	tables_table_name = "sys_tables.tab";
	columns_table_name = "sys_columns.tab";

	ifstream ins;
	ins.open("valid_ids.txt");
	if(ins.good()) {
		ins >> tableIDs;
		//cout << "tableID: " << tableIDs << endl;
		ins.close();
	} else {
		tableIDs = 0;
	}

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

/**
 * Destructor
 * _IO_flush_all_lockp (do_lock=do_lock@entry=0) at genops.c:841
 */
RelationManager::~RelationManager() {
	delete _rm_manager;
}

/**
 * @COMPLETED
 * This method creates a table called tableName with a vector of attributes (attrs).
 * 
 * Returns 0 on success and 1 on failure
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

	// A dummy RID to be returned
	RID dummyRID;

	// Handle for the tables table
	FileHandle tableTable_handle;

	FILE * ttable = fopen(tables_table_name.c_str(), "r+");
	tableTable_handle.setFileDescriptor(ttable);

	// Assemble the data
	int data_len = ((NAME_LEN * 2) + 20);
	unsigned char* tableEntryData = new unsigned char[data_len];
	for(int init = 0; init < data_len; init++) { tableEntryData[init] = 127; }

	const char* tname = tableName.c_str();

	tableIDs++;
	int new_table_id = tableIDs;
	//cout << "table's id is: " << new_table_id << endl;

	//write the table's ID to a file so no duplicates
	ofstream out;
	out.open("valid_ids.txt");
	out << tableIDs;
	out.close();

	int aa;

	// This is an integer--set it's value
	for(aa = 0; aa < 4; aa++) {
		unsigned char meta = (new_table_id % 256);
		tableEntryData[aa] = meta;
		new_table_id /= 256;
	}

	int meta = NAME_LEN; 
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
	
	// Put the index in the right place
	aa = 48;
	int cc = aa + 4;
	meta = (NAME_LEN + 8);

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

	// Handle for the columns table
	FileHandle columnTable_handle;

	FILE * ctable = fopen(columns_table_name.c_str(), "r+");
	columnTable_handle.setFileDescriptor(ctable);

	// For loop to add all the columns to the columns table
	for(unsigned int index = 0; index < attrs.size(); index++) {
		string colName = attrs.at(index).name;
		unsigned char* columnEntryData = new unsigned char[NAME_LEN+16];
		int ee;

		// Enter the tableID
		for(ee = 0; ee < 4; ee++) {
			columnEntryData[ee] = tableEntryData[ee];
		}

		// Enter the number preceeding the columnName
		int temp = NAME_LEN;
		for(; ee < 8; ee++) {
			columnEntryData[ee] = (temp % 256);
			temp /= 256;
		}

		// Enter the column's name
		const char* cName = colName.c_str();
		for(; cName[ee-8] != 0; ee++) {
			columnEntryData[ee] = cName[ee-8];
		}

		// Fill everything else with zeros
		for(; ee < 48; ee++) {
			columnEntryData[ee] = 0;
		}

		// Enter the column's type
		int dd;
		int meta2 = attrs.at(index).type;
		for(dd = 0; dd < 4; dd++) {
			columnEntryData[ee+dd] = (meta2 % 256);
			meta2 /= 256;
		}

		// Enter the column's length
		int meta3 = attrs.at(index).length;
		for(; dd < 8; dd++) {
			columnEntryData[ee+dd] = (meta3 % 256);
			meta3 /= 256;
		}

		sysTableHandler.insertRecord(columnTable_handle, colAttrs, columnEntryData, dummyRID);
	}
	return retVal;
}

/**
 * @COMPLETE
 * This method deletes a table called tableName. 
 */
int
RelationManager::deleteTable(const string &tableName) {
	// Table ID which will be used for finding the columns to delete
	int retVal;
	RID tableRID, columnRID;
	RBFM_ScanIterator tableRecordIter, colRecordIter;
	int tableID = 0;
	int control = 0;

	// Delete the file
	string tableFileName = user + tableName + ".tab";
	sysTableHandler.destroyFile(tableFileName);

	// The handle for the tables table to delete the table
	FileHandle tTable_handle;
	FILE * ttable = fopen(tables_table_name.c_str(), "r+");
	tTable_handle.setFileDescriptor(ttable);

	// The handle for the columns table
	FileHandle cTable_handle;
	FILE * ctable = fopen(columns_table_name.c_str(), "r+");
	cTable_handle.setFileDescriptor(ctable);

	// The attribute vectors that will be used to erase the table and it's columns
	const vector<string> table_attrNames = tabNames;
	const vector<Attribute> table_recordDescriptor = tabAttrs;
	const vector<string> column_attrNames = colNames;
	const vector<Attribute> column_recordDescriptor = colAttrs;

	// Get the table's RID + data
	retVal = sysTableHandler.scan(tTable_handle, table_recordDescriptor, "TableName", EQ_OP, &tableName, table_attrNames, tableRecordIter);

	if( retVal != SUCCESS ) {
		cout << "(RelationManager::deleteTable) Couldn't find table " << tableName << " in table's table." << endl;
		return retVal;
	}

	unsigned char* metaPage = new unsigned char[PAGE_SIZE];					//page
	unsigned char* tableRecord = new unsigned char[(NAME_LEN*2)+20];		//raw data
	unsigned char* columnRecord = new unsigned char[NAME_LEN+16];

	tableRecordIter.getNextRecord(tableRID, metaPage);
	sysTableHandler.readRecord(tTable_handle, table_recordDescriptor, tableRID, tableRecord);
	
	// Delete the table from the table of tables
	sysTableHandler.deleteRecord(tTable_handle, table_recordDescriptor, tableRID);
	memcpy(&tableID, tableRecord, 4);

	// Get the columns's RID + data
	retVal = sysTableHandler.scan(cTable_handle, column_recordDescriptor, "TableName", EQ_OP, &tableName, column_attrNames, colRecordIter);

	if( retVal != SUCCESS ) {
		cout << "(RelationManager::deleteTable) Couldn't find table " << tableName << " in table's table." << endl;
		return retVal;
	}

	// Delete the table's columns from the table of columns
	control = colRecordIter.getNextRecord(columnRID, metaPage);

	while(control == 0) {
		sysTableHandler.readRecord(cTable_handle, column_recordDescriptor, columnRID, columnRecord);
		sysTableHandler.deleteRecord(cTable_handle, column_recordDescriptor, columnRID);
		control = colRecordIter.getNextRecord(columnRID, metaPage);
	}

	delete metaPage;
	delete tableRecord;
	delete columnRecord;
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

	if(CORE_DEBUG) { cout << f0 << endl; }

	// Make the filehandle and set it to the columns table
	FileHandle tab_handle;
	FILE * ttable = fopen(tables_table_name.c_str(), "r+");
	tab_handle.setFileDescriptor(ttable);
	if(CORE_DEBUG) { cout << "ttable is: " << ttable << endl; }

	if(ttable == 0) {
		cout << "Couldn't open file--trying again." << endl;
		ttable = fopen(tables_table_name.c_str(), "r+");
		tab_handle.setFileDescriptor(ttable);
	}
	if(ttable == 0) {
		cout << "ERROR: Can't open file!" << endl;
		return retVal;
	}

	if(CORE_DEBUG) { cout << f1 << endl; }
	const vector<Attribute> _table_recordDescriptor = tabAttrs;
	const vector<string> _table_attributeNames = tabNames;

	// The iterator
	RBFM_ScanIterator tableRecordIter;
	if(CORE_DEBUG) { cout << f2 << endl; }
	
	// Now scan
	retVal = sysTableHandler.scan(tab_handle, _table_recordDescriptor, "TableName", EQ_OP, &tableName, _table_attributeNames, tableRecordIter);
	if(CORE_DEBUG) { cout << f3 << endl; }

	// Check if scan was successful
	if( retVal != SUCCESS ) {
		cout << "(RelationManager::getAttributes) Couldn't find table " << tableName << " in table's table." << endl;
		return retVal;
	}

	RID metaRID;															//a return value
	unsigned char* data_value = new unsigned char[PAGE_SIZE];				//the page
	tableRecordIter.getNextRecord(metaRID, data_value);

	unsigned char* dataTuple = new unsigned char[(NAME_LEN * 2) + 20];
	retVal = sysTableHandler.readRecord(tab_handle,  _table_recordDescriptor, metaRID, dataTuple);

	if( retVal != SUCCESS ) {
		cout << "(RelationManager::getAttributes) Couldn't find table " << tableName << " in table's table." << endl;
		return retVal;
	}

	// Copy the table's id.
	memcpy(&tableID, dataTuple, 4);

	// Make the filehandle and set it to the columns table
	FileHandle col_handle;
	FILE * ctable = fopen(columns_table_name.c_str(), "r+");
	col_handle.setFileDescriptor(ctable);

	// The vectors
	const vector<Attribute> _col_recordDescriptor = colAttrs;
	const vector<string> _col_attributeNames = colNames;

	// The iterator
	RBFM_ScanIterator colRecordIter;

	// Copy the table's id.
	void* t_ID = malloc(4);
	unsigned* debugID = new unsigned[1];
	memcpy(t_ID, &tableID, 4);
	memcpy(debugID, &tableID, 4);

	if(CORE_DEBUG) { cout << "table's ID: " << *debugID << endl; }

	retVal = sysTableHandler.scan(col_handle, _col_recordDescriptor, "TableID", EQ_OP, t_ID, _col_attributeNames, colRecordIter);

	// If something goes wrong, alert the user
	if( retVal != SUCCESS ) {
		cout << "(RelationManager::getAttributes) Couldn't find columns for " << tableName << " in columns's table." << endl;
	}

	int control = 0;
	RID metaRID2;
	unsigned char* dataPage = new unsigned char[PAGE_SIZE];
	unsigned char* data = new unsigned char[NAME_LEN+20];
	control = colRecordIter.getNextRecord(metaRID2, dataPage);

	for(unsigned debugVal = 0; control == 0; debugVal++) {
		sysTableHandler.readRecord(col_handle, _col_recordDescriptor, metaRID2, data);
		if(CORE_DEBUG) { cout << "debugVal is: " << debugVal << endl; }

		if(CORE_DEBUG) {
			cout << "reading col data:" << endl;
			printRawToFile(data, PAGE_SIZE, true);
		}
		
		/*if(debugVal % 2 == 0) {
			cout << "reading col data:" << endl;
			printRawData(data, 60);
		}*/

		if(data == NULL) { 
			cout << "ERROR: NULL DATA!" << endl;
		}

		int colID;
		//test if the attribute is ours:
		memcpy(&colID, data, 4);
		if(colID != tableID) {
			control = colRecordIter.getNextRecord(metaRID2, dataPage);
			continue;
		}

		Attribute pushMe;

		// Extract the name
		char* attrName = new char[NAME_LEN];
		for(int aa = 8; aa < NAME_LEN+8; aa++) {
			attrName[aa-8] = data[aa];
		}

		// Compact and set the name
		for(int aa = 0; attrName[aa] != '\0'; aa++) {
			pushMe.name += attrName[aa];
		}

		// Extract the type
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
			cout << "ERROR READING ATTRIBUTE TYPE { " << accu << " }" << endl;
		}

		// Extract the length
		unsigned char* lenData = new unsigned char[4];
		for(int aa = (NAME_LEN + 12); aa < (NAME_LEN + 16); aa++) { lenData[aa-(NAME_LEN+12)] = data[aa]; }

		int _power = 1;
		int _accu = 0;
		for(int bb = 0; bb < 4; bb++) {
			_accu += lenData[bb] * _power;
			_power *= 256;
		}

		//cout << "_accu is: " << _accu << endl;
		pushMe.length = _accu;

		// Free the allocated pointers
		delete attrName;
		delete typeData;
		delete lenData;

		attrs.push_back(pushMe);
		control = colRecordIter.getNextRecord(metaRID2, dataPage);
	}

	// Now delete everything that isn't needed:
	delete data_value;
	delete dataTuple;
	delete dataPage;
	delete data;
	free(t_ID);

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
	int retVal = -1;

	if (CORE_DEBUG) {
		if(data != NULL) {
			cout << "Inserting Data:" << endl;
			unsigned char* ucdata = NULL;
			ucdata = (unsigned char*) data;
			printRawData(ucdata,22);
			cout << flush;
		}
	}

	FileHandle insertHandle;
	FILE * tableFile = fopen(tableFileName.c_str(), "r+");
	insertHandle.setFileDescriptor(tableFile);

	if(tableFile == NULL) {
		cout << "ERROR: Couldn't open table file!" << endl;
		return -1;
	}

	vector<Attribute> insertVector;
	int testRetVal = getAttributes(tableName, insertVector);

	if (testRetVal != SUCCESS){
		free(tableFile);
		return -1;
	}

	const vector<Attribute> _insertVector = insertVector;

	if (CORE_DEBUG) {
		cout << "++++" << endl;
		for(unsigned i = 0; i < _insertVector.size(); i++) { 
			cout << "attr name: " << _insertVector.at(i).name << endl;
			cout << "attr type: " << _insertVector.at(i).type << endl;
			cout << "attr size: " << _insertVector.at(i).length << endl;
			cout << endl;
		}
		//cout << "data size is: " << sizeof(data) << endl;
	}

	retVal = sysTableHandler.insertRecord(insertHandle, _insertVector, data, rid);

	return retVal;
}

/**
 * COMPLETE
 *
 * Test 5 segfaults
 * segfault is NOT in this method.
 *
 * This method deletes all tuples in a table called tableName.
 * This command should result in an empty table.
 */ 
int
RelationManager::deleteTuples(const string &tableName) {
	string tableFileName = user + tableName + ".tab";
	int retVal = -1;

	FileHandle clearMe;
	clearMe.fname = tableFileName;
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
	int retVal = -1;

	FileHandle clearMe;
	FILE * _clearMe = fopen(tableFileName.c_str(), "r+");
	clearMe.setFileDescriptor(_clearMe);

	//the Record Descriptor
	vector<Attribute> descriptor;

	//assign the table's attributes to "descriptor"
	if (getAttributes(tableFileName, descriptor) != SUCCESS){
		return -1;
	}

	retVal = sysTableHandler.deleteRecord(clearMe, descriptor, rid);

	return retVal;
}

/**
 * Simple delete + insert
 *
 * This method updates a tuple identified by a given rid. Note: if the tuple grows 
 * (i.e., the size of tuple increases) and there is no space in the page to store the tuple 
 * (after the update), then, the tuple is migrated to a new page with enough free space. 
 * Since you will implement an index structure (e.g., B-tree) in project 3, you can assume 
 * that tuples are identified by their rids and when they migrate, they leave a tombstone 
 * behind pointing to the new location of the tuple.
 */
int
RelationManager::updateTuple(const string &tableName, const void *data, const RID &rid) {
	int retVal = -1;
	string tableFileName = user + tableName + ".tab";

	FileHandle fixMe;
	FILE * _fixMe = fopen(tableFileName.c_str(), "r+");
	fixMe.setFileDescriptor(_fixMe);

	vector<Attribute> tableAttrs;
	if (getAttributes(tableName, tableAttrs) != SUCCESS){
		return retVal;
	}
	const vector<Attribute> recordDescriptor = tableAttrs;
	retVal = sysTableHandler.updateRecord(fixMe, tableAttrs, data, rid);

	return retVal;
}

//This method reads a tuple identified by a given rid.
/**
 * -1 = File I/O error
 */ 
int
RelationManager::readTuple(const string &tableName, const RID &rid, void *data) {
	int retVal = -1;

	// The handle for the tables table to delete the table
	FileHandle handle;
	string s0 = user + tableName + ".tab";
	FILE * tableFile = fopen(s0.c_str(), "r+");
	if(tableFile == NULL) {
		cout << "Couldn't open file--trying again." << endl;
		tableFile = fopen(s0.c_str(), "r+");
	}
	if(tableFile == 0) {
		cout << "ERROR: Can't open file!" << endl;
		return retVal;
	}

	handle.setFileDescriptor(tableFile);

	vector<Attribute> tableAttrs;
	if (getAttributes(tableName, tableAttrs) != SUCCESS){
		return -2;
	}

	const vector<Attribute> _tableAttrs = tableAttrs;

	if (CORE_DEBUG) {
		for(unsigned i = 0; i < _tableAttrs.size(); i++) { cout << "attr is: " << _tableAttrs.at(i).name << endl; }
		cout << "data size is: " << sizeof(data) << endl;
	}

	retVal = sysTableHandler.readRecord(handle, _tableAttrs, rid, data);

	if(testFileForEmptiness(s0) == SUCCESS) {
		return 1;
	}
	cout << "returning: " << retVal << endl;

	return retVal;
}

//This method reads a specific attribute of a tuple identified by a given rid. 
int
RelationManager::readAttribute(const string &tableName, const RID &rid, const string &attributeName, void *data) {
	// Open table file
	FileHandle fileHandle;
	string filename = user + tableName + ".tab";
	FILE *table_file = fopen(filename.c_str(), "r+");
	if (table_file == NULL) { return -1; }
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
	FILE *table_file = fopen(tableName.c_str(), "r+");
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

/**
 * 0 = empty
 * 1 != empty
 */
int
RelationManager::testFileForEmptiness(string tableName) {
	//test if the tuple is valid:
	ifstream tester;
	tester.open(tableName.c_str());
	char* delMarker = new char[23];
	tester.read(delMarker, 23);

	cout << "opened: " << tableName.c_str() << endl;
	cout << "delMarker is: ";
	for(int i = 0; i < 23; i++) {
		if(delMarker[i] == 0) { cout << "^@"; }
		else { cout << delMarker[i]; }
	}
	cout << endl << flush;

	if(delMarker[12] != 0) { return 1; }
	if(delMarker[13] != 0) { return 1; }
	if(delMarker[14] != 0) { return 1; }
	if(delMarker[15] != 0) { return 1; }
	if(deleteProcedure == 2) { return SUCCESS; }
	if(delMarker[16] != 'D') { return 1; }
	if(delMarker[17] != 'E') { return 1; }
	if(delMarker[18] != 'L') { return 1; }
	if(delMarker[19] != 'E') { return 1; }
	if(delMarker[20] != 'T') { return 1; }
	if(delMarker[21] != 'E') { return 1; }
	if(delMarker[22] != 'D') { return 1; }

	return SUCCESS;
}

void
RelationManager::printRawData(unsigned char* data, int len) {
	for(int i = 0; i < len; i++) {
		cout << "[";
		if (i < 10) { cout << "0"; }
		cout << i << "] ";
		unsigned char meta = data[i];
		cout << (bitset<8>) meta;

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
