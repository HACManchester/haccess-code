# tests/Makefile
#
# Copyright 2016 Ben Dooks <ben@fluff.org>

# Find the arduino bits from here
ARDUINO=~/arduino-1.6.11/hardware/arduino/avr/cores/arduino

# We add our own Serial.h for Serial class support
CXXFLAGS=-O2 -Wall -I../ -I`pwd` -include Serial.h -include $(ARDUINO)/WString.h -std=c++1z -D__TEST -DString=string

all: triggers triggers2 testbench1

testbench1: testbench1.o trigger_config.o trigger.o timer.o millis.o fs.o IniFile.o WString.o avrclib.o
	g++ -lrt -o $@ $^

triggers2: trigger.o  timer.o test-trigger2.o millis.o
	g++ -lrt -o triggers2 $^

triggers: trigger.o timer.o test-trigger.o
	g++ -o triggers $^

# clean

.PHONY: clean
clean:
	-rm *.o

# code from upper level project

timer.o:	../timer.h
timer.o:	../timer.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

trigger.o:	../trigger.h
trigger.o:	../trigger.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

trigger_config.o:	../trigger_config.h
trigger_config.o:	../trigger_config.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

IniFile.o:	../IniFile.h
IniFile.o:	../IniFile.cpp
	$(CXX) $(CXXFLAGS) -I`pwd` -c -o $@ $<

# code we appropriate from the Arduino core
WString.o:	$(ARDUINO)/WString.h
WString.o:	$(ARDUINO)/WString.cpp
	$(CXX) $(CXXFLAGS) -include avr/clib.h -I$(ARDUINO) -c -o $@ $<
