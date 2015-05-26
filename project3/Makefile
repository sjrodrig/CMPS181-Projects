#Benjy's Magic Makefile

#Author: Benjamin Strauss
#Author: Paul Mini
#CMPS_181 makefile

GCC	= g++ -g -O0 -Wall -Wextra -std=gnu++11

all:	db181

db181: Tools.o FileManager.o RecordBasedFile.o RelationManager.o IndexManager.o main.o
	${GCC} -o db181 Tools.o FileManager.o RecordBasedFile.o RelationManager.o IndexManager.o main.o

one: Tools.o FileManager.o RecordBasedFile.o RelationManager.o IndexManager.o ixtest1.o
	${GCC} -o db181 Tools.o FileManager.o RecordBasedFile.o RelationManager.o IndexManager.o ixtest1.o

two: Tools.o FileManager.o RecordBasedFile.o RelationManager.o IndexManager.o ixtest2.o
	${GCC} -o db181 Tools.o FileManager.o RecordBasedFile.o RelationManager.o IndexManager.o ixtest2.o

ixtest1.o: ixtest1.cpp
	${GCC} -c ixtest1.cpp	

ixtest2.o: ixtest2.cpp
	${GCC} -c ixtest2.cpp

main.o: main.cpp
	${GCC} -c main.cpp

Tools.o: Tools.cpp
	${GCC} -c Tools.cpp

FileManager.o: FileManager.cpp
	${GCC} -c FileManager.cpp

RecordBasedFile.o: RecordBasedFile.cpp
	${GCC} -c RecordBasedFile.cpp

RelationManager.o: RelationManager.cpp
	${GCC} -c RelationManager.cpp

IndexManager.o: IndexManager.cpp
	${GCC} -c IndexManager.cpp

#delete the executable
clear:
	rm -f *.o
	rm -f db181

#recompile from scratch
remake:
	make purge
	make all
	clear

purge:
	make clear
	rm -f valid_ids.txt
	rm -f rids_file
	rm -f sizes_file
	rm -f *.tab
	rm -f *.dump
	clear

remake1:
	make purge
	rm -f Age_idx
	make one
	clear

remake2:
	make purge
	make two
	clear




