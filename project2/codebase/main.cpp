#include <fstream>
#include <iostream>
#include <cassert>

#include "test_util.h"
#include "FileManager.h"
#include "RecordBasedFile.h"
//#include "RelationManager.h"

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

//RelationManager *rm;

void rbfTest() {
  PagedFileManager *pfm = PagedFileManager::instance();
  RecordBasedFileManager *rbfm = RecordBasedFileManager::instance();

	rm->deleteTuples("foom");

  // write your own testing cases here
}

void TEST_RM_5(const string &tableName, const int nameLength, const string &name, const int age, const float height, const int salary)
{
    // Functions Tested
    // 0. Insert tuple;
    // 1. Read Tuple
    // 2. Delete Tuples **
    // 3. Read Tuple
    cout << "****In Test Case 5****" << endl;
    
    RID rid;
    int tupleSize = 0;
    void *tuple = malloc(100);
    void *returnedData = malloc(100);
    void *returnedData1 = malloc(100);
   
    // Test Insert Tuple 
    prepareTuple(nameLength, name, age, height, salary, tuple, &tupleSize);
    RC rc = rm->insertTuple(tableName, tuple, rid);
    assert(rc == success);

    // Test Read Tuple
    rc = rm->readTuple(tableName, rid, returnedData);
    assert(rc == success);
    printTuple(returnedData, tupleSize);

    cout << "Now Deleting..." << endl;

    // Test Delete Tuples
    rc = rm->deleteTuples(tableName);

    assert(rc == success);
    
    // Test Read Tuple
    memset((char*)returnedData1, 0, 100);
cout << "GOT HERE" << endl;
    rc = rm->readTuple(tableName, rid, returnedData1);
cout << "DIDN'T GET HERE" << endl;
    assert(rc != success);

    printTuple(returnedData1, tupleSize);
    
    if(memcmp(tuple, returnedData1, tupleSize) != 0)
    {
        cout << "****Test case 5 passed****" << endl << endl;
    }
    else
    {
        cout << "****Test case 5 failed****" << endl << endl;
    }
       
    free(tuple);
    free(returnedData);
    free(returnedData1);
    return;
}

int main() {
	cout << "test..." << endl;

	rm = new RelationManager();
	
	createTable("tbl_employee");
    // Delete Tuples
    TEST_RM_5("tbl_employee", 6, "Dillon", 29, 172.5, 7000);

	// other tests go here
	rbfTest();

	cout << "OK" << endl;
	
	exit(0);
}
