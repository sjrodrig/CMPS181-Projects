#Benjy's Magic Makefile

#Author: Benjamin Strauss
#Author: Paul Mini
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








