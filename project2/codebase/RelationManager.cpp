#include "RelationManager.h"
#include <fstream>

using namespace std;

//constructor
RelationManager::RelationManager() {
	
	ifstream checkStream("sys_tables.tab");

    if (checkStream.good()) {
        //Nothing TODO
    } else {
		//make "sys_tables.tab"
		ofstream maker1("sys_tables.tab");
		ofstream maker2("sys_columns.tab");
		
		maker1.close();
		maker2.close();
    }

    checkStream.close();
	
}

//This method creates a table called tableName with a vector of attributes (attrs).
int
RelationManager::createTable(const string &tableName, const vector<Attribute> &attrs) {

}

//This method deletes a table called tableName. 
int
RelationManager::deleteTable(const string &tableName) {

}

//This method gets the attributes (attrs) of a table called tableName. 
int
RelationManager::getAttributes(const string &tableName, vector<Attribute> &attrs) {

}

//This method inserts a tuple into a table called tableName. You can assume that the input is always correct and free of error. That is, you do not need to check if the input tuple has the right number of attributes and if the attribute types match. 
int
RelationManager::insertTuple(const string &tableName, const void *data, RID &rid) {

}

//This method deletes all tuples in a table called tableName. This command should result in an empty table. 
int
RelationManager::deleteTuples(const string &tableName) {

}

//This method deletes a tuple with a given rid. 
int
RelationManager::deleteTuple(const string &tableName, const RID &rid) {

}

//This method updates a tuple identified by a given rid. Note: if the tuple grows (i.e., the size of tuple increases) and there is no space in the page to store the tuple (after the update), then, the tuple is migrated to a new page with enough free space. Since you will implement an index structure (e.g., B-tree) in project 3, you can assume that tuples are identified by their rids and when they migrate, they leave a tombstone behind pointing to the new location of the tuple.
int
RelationManager::updateTuple(const string &tableName, const void *data, const RID &rid) {

}

//This method reads a tuple identified by a given rid. 
int
RelationManager::readTuple(const string &tableName, const RID &rid, void *data) {

}

//This method reads a specific attribute of a tuple identified by a given rid. 
int
RelationManager::readAttribute(const string &tableName, const RID &rid, const string &attributeName, void *data) {

}

//This method reorganizes the tuples in a page. That is, it pushes the free space towards the end of the page. Note: In this method you are NOT allowed to change the rids, since they might be used by other external index structures, and it's too expensive to modify those structures for each such a function call. It's OK to keep those deleted tuples and their slots. 
int
RelationManager::reorganizePage(const string &tableName, const unsigned pageNumber) {

}

//This method scans a table called tableName. That is, it sequentially reads all the entries in the table. This method returns an iterator called rm_ScanIterator to allow the caller to go through the records in the table one by one. A scan has a filter condition associated with it, e.g., it consists of a list of attributes to project out as well as a predicate on an attribute (“Sal > 40000”). Note: the RBFM_ScanIterator should not cache the entire scan result in memory. In fact, you need to be looking at one (or a few) page(s) of data at a time, ever. In this project, let the OS do the memory-management work for you. 
int
RelationManager::scan(const string &tableName, const string &conditionAttribute, const CompOp compOp, const void *value, const vector<string> &attributeNames, RM_ScanIterator &rm_ScanIterator) {

}

/*********************************************************************************
 *										Iterator								 *
 *********************************************************************************/

//This method is used to get the next tuple from the scanned table. It returns RM_EOF when all tuples are scanned. 
int
RM_ScanIterator::getNextTuple(RID &rid, void *data) {

}

//This method is used to close the iterator. 
int
RM_ScanIterator::close() {

}


