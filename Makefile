CC = g++
CXXFLAGS = -g -ljsoncpp -O3 -std=c++11 -Wall -Wextra -pedantic -pthread -lhdf5 -lmysqlclient
INCFLAGS = -I.  -D_GLIBCXX_USE_CXX11_ABI=0 -I$(CONDA_PREFIX)/include

CXXFLAGS += '-DHDF5'

all: config eventreader navel writer database decode link #huffman

tests: 
	$(CC) -o tests ReadConfig.o RawDataInput.o DATEEventHeader.o Digit.o EventReader.o HDF5Writer.o hdf5_functions.o database.o CopyEvents.o sensors.o huffman.o testing/*cc $(CXXFLAGS) $(INCFLAGS)

link:
	$(CC) -g -o decode decode.o ReadConfig.o RawDataInput.o DATEEventHeader.o Digit.o EventReader.o HDF5Writer.o hdf5_functions.o database.o CopyEvents.o sensors.o huffman.o $(CXXFLAGS) $(INCFLAGS)

decode:
	$(CC) -c decode.cc $(CXXFLAGS) $(INCFLAGS)

huffman:
	$(CC) -c decode_huffman.cc $(CXXFLAGS) $(INCFLAGS)
	$(CC) -g -o decode_huffman decode_huffman.o ReadConfig.o RawDataInput.o DATEEventHeader.o Digit.o EventReader.o HDF5Writer.o hdf5_functions.o database.o CopyEvents.o sensors.o huffman.o $(CXXFLAGS) $(INCFLAGS)

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
	@rm *.o decode

.PHONY: decode config clean navel eventreader writer database tests
