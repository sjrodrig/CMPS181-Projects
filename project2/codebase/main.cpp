#include <fstream>
#include <iostream>
#include <cassert>

#include "FileManager.h"
#include "RecordBasedFile.h"
#include "RelationManager.h"

using namespace std;

RelationManager *rm;

void rbfTest() {
  PagedFileManager *pfm = PagedFileManager::instance();
  RecordBasedFileManager *rbfm = RecordBasedFileManager::instance();

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

	RID dummyID;
	dummyID.pageNum = 0;
	dummyID.slotNum = 0;

	/*Attribute bar;
	bar.name = "barq";
	bar.length = 127;
	bar.type = TypeVarChar;
	nullVec.push_back(bar);*/

	rbfTest();

	unsigned char* meta = new unsigned char[10];
	meta[0] = 6; //<--this is what is auto-determining the size
	meta[1] = 0;
	meta[2] = 0;
	meta[3] = 0;	
	meta[4] = 't';
	meta[5] = 'e';
	meta[6] = 's';
	meta[7] = 't';
	meta[8] = '*';
	meta[9] = 0;

	rm->createTable("foo", nullVec);

	rm->insertTuple("foo", meta, dummyID);

	rm->createTable("bar", nullVec);


	// other tests go here

	const string tableName = "foo";
	vector<Attribute> attrs;

	rm->deleteTable(tableName);

	cout << "OK" << endl;
	
	exit(0);
}
