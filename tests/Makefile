# tests/Makefile
#
# Copyright 2016 Ben Dooks <ben@fluff.org>

CXXFLAGS=-O2 -Wall -I../ -std=c++1z -D__TEST

all: triggers triggers2 testbench1

testbench1: trigger.o timer.o millis.o fs.o IniFile.o
	g++ -lrt -o $@ $^

triggers2: trigger.o  timer.o test-trigger2.o millis.o
	g++ -lrt -o triggers2 $^

triggers: trigger.o timer.o test-trigger.o
	g++ -o triggers $^

# code from upper level project

timer.o:	../timer.h
timer.o:	../timer.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

trigger.o:	../trigger.h
trigger.o:	../trigger.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

IniFile.o:	../IniFile.h
IniFile.o:	../IniFile.cpp
	$(CXX) $(CXXFLAGS) -I`pwd` -c -o $@ $<


