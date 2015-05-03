#include <fstream>
#include <iostream>
#include <cassert>

//#include "test_util.h"
#include "FileManager.h"
#include "RecordBasedFile.h"
#include "RelationManager.h"

using namespace std;

RelationManager *rm;

void rbfTest() {
  PagedFileManager *pfm = PagedFileManager::instance();
  RecordBasedFileManager *rbfm = RecordBasedFileManager::instance();

	rm->deleteTuples("foom");

  // write your own testing cases here
}


int main() {
	cout << "test..." << endl;

	rm = new RelationManager();
	vector<Attribute> nullVec;

	Attribute foo;
	foo.name = "foo";
	foo.length = 6;
	foo.type = TypeVarChar;
	nullVec.push_back(foo);

	Attribute bar;
	bar.name = "bar";
	bar.length = INT_SIZE;
	bar.type = TypeInt;
	nullVec.push_back(bar);

	RID dummyID;
	dummyID.pageNum = 0;
	dummyID.slotNum = 0;

	const string tableName = "foom";

	unsigned char* meta = new unsigned char[14];
	meta[0] = 6; //<--this is what is auto-determining the size
	meta[1] = 0;
	meta[2] = 0;
	meta[3] = 0;	
	meta[4] = 't';
	meta[5] = 'e';
	meta[6] = 's';
	meta[7] = 't';
	meta[8] = 0;
	meta[9] = 0;
	meta[10] = 0xDE;
	meta[11] = 0xAD;
	meta[12] = 0xD0;
	meta[13] = 0x0D;

	unsigned char* _meta = new unsigned char[14];

	rm->createTable(tableName, nullVec);

	rm->insertTuple(tableName, meta, dummyID);

	//rm->createTable("bar", nullVec);
	rm->readTuple(tableName, dummyID, _meta);

	// other tests go here
	rbfTest();

	vector<Attribute> attrs;

	rm->deleteTable(tableName);

	cout << "OK" << endl;
	
	exit(0);
}
