CC = g++
CXXFLAGS = -g -ljsoncpp -O3 -std=c++11 -Wall -Wextra -pedantic -pthread -lhdf5 -lmysqlclient
INCFLAGS = -I.  -D_GLIBCXX_USE_CXX11_ABI=0 -I$(CONDA_PREFIX)/include

CXXFLAGS += '-DHDF5'

all: config eventreader navel writer database main link

tests: 
	$(CC) -o tests ReadConfig.o RawDataInput.o DATEEventHeader.o Digit.o Trigger.o EventReader.o HDF5Writer.o hdf5_functions.o database.o CopyEvents.o sensors.o testing/*cc $(CXXFLAGS) $(INCFLAGS)

link:
	$(CC) -g -o main main.o ReadConfig.o RawDataInput.o DATEEventHeader.o Digit.o Trigger.o EventReader.o HDF5Writer.o hdf5_functions.o database.o CopyEvents.o sensors.o $(CXXFLAGS) $(INCFLAGS)

main:
	$(CC) -c main.cc $(CXXFLAGS) $(INCFLAGS)

config:
	$(CC) -c config/ReadConfig.cc $(CXXFLAGS) $(INCFLAGS)

eventreader:
	$(CC) -c detail/EventReader.cc $(CXXFLAGS) $(INCFLAGS)
	$(CC) -c RawDataInput.cc $(CXXFLAGS) $(INCFLAGS)
	
navel:
	$(CC) -c navel/*cc $(CXXFLAGS) $(INCFLAGS)

writer:
	$(CC) -c writer/*cc $(CXXFLAGS) $(INCFLAGS)

database:
	$(CC) -c database/*cc $(CXXFLAGS) $(INCFLAGS)

clean:
	@rm *.o main

.PHONY: main config clean navel eventreader writer database tests
