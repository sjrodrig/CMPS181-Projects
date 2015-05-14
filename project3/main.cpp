#include <fstream>
#include <iostream>
#include <cassert>

#include "RelationManager.h"
#include "IndexManager.h"

using namespace std;

RelationManager *rm;

void rbfTest() {
  PagedFileManager *pfm = PagedFileManager::instance();
  RecordBasedFileManager *rbfm = RecordBasedFileManager::instance();

  // write your own testing cases here
}


int main() {
	
	IndexManager *indexTester = IndexManager::instance();
	cout << "made index" << endl;
	indexTester->createFile("foo.tab");
	
		
	exit(0);
}
