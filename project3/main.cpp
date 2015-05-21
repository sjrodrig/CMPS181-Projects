#include <fstream>
#include <iostream>
#include <cassert>

#include "RelationManager.h"
#include "IndexManager.h"

using namespace std;

Tools util;
RelationManager *rm;
IndexManager *indexTester;

/**
 * NOTE:	ALL DATA WAS 12-BYTES FURTHER ALONG THE PAGE THAN EXPECTED.
 * 			This is here because of the "PAGED_FILE__" text on the first page
 *	>>> THIS IS FIXED!
 */

void rbfTest() {
  PagedFileManager *pfm = PagedFileManager::instance();
  RecordBasedFileManager *rbfm = RecordBasedFileManager::instance();

  // write your own testing cases here
}

int main() {
	indexTester = IndexManager::instance();
	cout << "made index" << endl;
	indexTester->createFile("foo.tab");

	Attribute testInt;
	testInt.type = TypeInt;
	testInt.length = 4;
	testInt.name = "testInt";

	ChildEntry TestEntry;
	TestEntry.childPageNumber = 2;
	int* key = new int[1];
	key[0] = 5;
	TestEntry.key = key;

	char* rawPageData = new char[PAGE_SIZE * 2];
	unsigned char* pageData = new unsigned char[PAGE_SIZE * 2];
	unsigned char* useData = new unsigned char[PAGE_SIZE];

	ifstream infile;
	infile.open("foo.tab");
	infile.read(rawPageData, PAGE_SIZE * 2);

	for(int index = 0; index < PAGE_SIZE*2; index++) {
		pageData[index] = rawPageData[index];
	}

	util.fprintNBytes("check.dump", pageData, 8192);

	for(int index = 0; index < PAGE_SIZE; index++) {
		useData[index] = rawPageData[index+PAGE_SIZE];
	}
	indexTester->testInsertNonLeafRecord(testInt, TestEntry, useData);


		
	exit(0);
}
