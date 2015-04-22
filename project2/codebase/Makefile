#Benjy's Magic Makefile

#Author: Benjamin Strauss
#Author: ???
#CMPS_181 makefile

GCC	= g++ -g -O0 -Wall -Wextra -std=gnu++11

all:	db181

db181: FileManager.o RecordBasedFile.o RelationManager.o main.o
	${GCC} -o db181 FileManager.o RecordBasedFile.o RelationManager.o main.o

main.o: main.cpp
	${GCC} -c main.cpp

#tools.o: tools.cpp
#	${GCC} -c tools.cpp

FileManager.o: FileManager.cpp
	${GCC} -c FileManager.cpp

RecordBasedFile.o: RecordBasedFile.cpp
	${GCC} -c RecordBasedFile.cpp

RelationManager.o: RelationManager.cpp
	${GCC} -c RelationManager.cpp

#delete the executable
clear:
	rm -f tools.o
	rm -f RecordBasedFile.o
	rm -f RelationManager.o
	rm -f FileManager.o
	rm -f main.o
	rm -f db181

#recompile from scratch
remake:
	make purge
	make all
	clear

purge:
	make clear
	rm -f sys_columns.tab
	rm -f sys_tables.tab
	clear








