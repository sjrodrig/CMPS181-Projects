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
	foo.length = 5;
	foo.type = TypeVarChar;
	nullVec.push_back(foo);

	Attribute bar;
	bar.name = "barq";
	bar.length = 127;
	bar.type = TypeVarChar;
	nullVec.push_back(bar);

	rbfTest();

	rm->createTable("foo", nullVec);
	// other tests go here

	const string tableName = "foo";
	vector<Attribute> attrs;

	rm->getAttributes(tableName, attrs);

	cout << "OK" << endl;
	
	exit(0);
}
