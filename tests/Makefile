# tests/Makefile
#
# Copyright 2016 Ben Dooks <ben@fluff.org>

CXXFLAGS=-O2 -Wall -I../ -std=c++1z -D__TEST

all: triggers triggers2

triggers2: trigger.o test-trigger2.o
	g++ -o triggers2 trigger.o test-trigger2.o

triggers: trigger.o test-trigger.o
	g++ -o triggers trigger.o test-trigger.o

trigger.o:	../trigger.h
trigger.o:	../trigger.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<


