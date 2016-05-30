# tests/Makefile
#
# Copyright 2016 Ben Dooks <ben@fluff.org>

CXXFLAGS=-O2 -Wall -I../ -std=c++1z -D__TEST

all: triggers triggers2 testbench1

testbench1: trigger.o timer.o
	g++ -lrt -o $@ $^

triggers2: trigger.o  timer.o test-trigger2.o
	g++ -lrt -o triggers2 $^

triggers: trigger.o timer.o test-trigger.o
	g++ -o triggers $^

timer.o:	../timer.h
timer.o:	../timer.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

trigger.o:	../trigger.h
trigger.o:	../trigger.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<


