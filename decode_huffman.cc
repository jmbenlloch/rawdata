#include <iostream>
#include <fstream>
#include "config/ReadConfig.h"
#include "writer/CopyEvents.h"
#include "RawDataInput.h"

#ifndef _HDF5WRITER
#include "writer/HDF5Writer.h"
#endif

#ifndef SPDLOG_VERSION
#include "spdlog/spdlog.h"
#endif

namespace spd = spdlog;

struct Huffman {
	int value;
	Huffman * next[2];
};


int read_data(unsigned char * data, int position){
	int tmp = 0;

	tmp = tmp | data[position    ] << 24;
	tmp = tmp | data[position + 1] << 16;
	tmp = tmp | data[position + 2] <<  8;
	tmp = tmp | data[position + 3];
	// printf("position: %d, tmp: 0x%08x\n", position, tmp);

	return tmp;
}

void print_huffman(Huffman * huffman, int code){
	for(int i=0; i<2; i++){
		if(huffman->next[i]){
			int new_code = code * 10 + i;
			print_huffman(huffman->next[i], new_code);
		}
	}

	if(! (huffman->next[0] || huffman->next[1])){
		printf("%d -> %d\n", code, huffman->value);
	}
}

short int decode_huffman(Huffman * huffman, int code, int position, int * result){
	int bit = CheckBit(code, position);
//	printf("pos: %d, bit: %d\n", position, bit);

	int final_pos = position;

	if(! (huffman->next[0] || huffman->next[1])){
		*result = huffman->value;
	}else{
		final_pos = decode_huffman(huffman->next[bit], code, position-1, result);
	}
	// printf("code: %d, pos: %d, result: %d\n", code, position, *result);
	// printf("final_pos: %d\n", final_pos);

	return final_pos;
}

int parse_huffman_line(std::string &line, Huffman * huffman){
	// copy line to cstring to use strtok function
	// (input has to be non-const)
	char * cstr = new char [line.length()+1];
	std::strcpy (cstr, line.c_str());

	int result;

	char *token;
	token = strtok (cstr," ,\t");
	result = std::stoi(token);
//	printf("result: %d\n", result);

	token = strtok (NULL, " ,\t");
//	printf("code: %s\n", token);

	Huffman * current_node = huffman;

	int bitcount = 0;
	unsigned char bit;
	while (token[bitcount]){
		bit = token[bitcount] - 0x30; // ascii to int
//		printf("token[%d]: %c\n", bitcount, token[bitcount]);
		bitcount += 1;

		if(current_node->next[bit]){
//			printf("bit already set\n");
			current_node = current_node->next[bit];
		}else{
//			printf("bit not set\n");
			Huffman * new_node = (Huffman*) malloc(sizeof(Huffman));
			new_node->next[0] = NULL;
			new_node->next[1] = NULL;

			current_node->next[bit] = new_node;
			current_node = new_node;
		}
	}

//	printf("bitcount: %d, %d\n", bitcount, bit);
	current_node->value = result;
//	printf("huffman-value: %d\n", current_node->value);

	delete[] cstr;

	return result;
}


void read_huffman_file(std::string filein, Huffman * huffman){
	std::ifstream inFile;
	std::string line;
	inFile.open(filein);

	if (inFile.is_open()){
		while(getline(inFile, line)) {
			//printf("\n\nline: %s\n", line.c_str());
			parse_huffman_line(line, huffman);
		}
	}
}


int read_data_file(const std::string &filein, unsigned char **data){
	std::ifstream inFile;
	std::string line;
	inFile.open(filein);

	int size = 0;
	int position = 0;
	int bitcount = 7;
	unsigned char tmp = 0;

	if (inFile.is_open()){
		while(getline(inFile, line)) {
			//			std::cout << line << std::endl;
			size = line.size();
			std::cout << "size: " << size << std::endl;
			*data = (unsigned char*) malloc(size);
			memset(*data, 0, size);

			for(int i=0; i<size; i++){
				//				std::cout << line[i] << std::endl;

				unsigned char value = line[i] - 0x30; // convert ASCII to int
				//				printf("line[%d]: %c, %d\n", i, line[i], value);

				tmp = tmp | (value << bitcount);
				bitcount -= 1;
				//				printf("tmp: 0x%04x\n", tmp);

				if (bitcount < 0){
					(*data)[position] = tmp;

					// printf("val: 0x%02x\n", tmp);
					position += 1;
					bitcount  = 7;
					tmp       = 0;
				}
			}
		}
		inFile.close();
	}
	return size;
}


// current_bit will be modified to set the new position
int decode_compressed_value(int previous_value, int data, int * start_bit, Huffman * huffman){
	// Check data type (0 uncompressed, 1 huffman)
	int current_bit = *start_bit;
	int type = CheckBit(data, current_bit);
	current_bit -= 1;

	int wfvalue;
	if(type == 0){
		wfvalue = (data >> (current_bit - 11)) & 0x0FFF;
		current_bit -= 12;
	}else{
		current_bit = decode_huffman(huffman, data, current_bit, &wfvalue);
		wfvalue = previous_value + wfvalue;
	}
	*start_bit = current_bit;

	return wfvalue;
}


int main(int argc, char* argv[]){
	auto console = spd::stdout_color_mt("console");
	console->info("RawDataInput started");

	if (argc < 2){
        console->error("Missing argument: <configfile>");
		std::cout << "Usage: rawdatareader <configfile>" << std::endl;
		return 1;
	}

	std::string filename = std::string(argv[1]);
	ReadConfig config = ReadConfig(filename);

	if(config.verbosity() > 0){
		console->info("Huffman tree: {}", config.huffman_tree());
		console->info("npmts: {}", config.npmts());
	}

	// Load waveforms data from txt file
	unsigned char * data;
	int size = read_data_file(config.file_in(), &data);

	// Create Huffman tree
	Huffman huffman;
	huffman.next[0] = NULL;
	huffman.next[1] = NULL;

	read_huffman_file(config.huffman_tree(), &huffman);

	if(config.verbosity() > 0){
		console->info("Huffman tree:");
		print_huffman(&huffman, 1);
	}

	// Start processing data
	int itmp = 0;

	int position_dec = 0;
	int current_bit = 31 - config.offset();

	if(config.verbosity() > 1){
		for(int i=0; i<32; i++){
			printf("data[%d]: %02x", i, data[i*4  ]);
			printf("%02x",              data[i*4+1]);
			printf("%02x",              data[i*4+2]);
			printf("%02x\n",            data[i*4+3]);
		}
	 }

	int npmts = config.npmts();
	int * values = (int *) malloc(sizeof(int) * npmts);

    std::ofstream out(config.file_out());

	//for(int i=0; i<128; i++){
	for(int i=0; i<12*400000; i++){
		// printf("\n\n new value, i: %d\n", i);

		// second half of the word -> load next one
		if(current_bit < 16){
			// move as many 8-bit words as needed
			int nwords = (31 - current_bit) / 8;
			current_bit += nwords * 8;
			// printf("nwords: %d\n", nwords);
			// printf("current_bits: %d\n", current_bit);
			position_dec += nwords;

		}

		itmp = read_data(data, position_dec);
		int wfvalue = decode_compressed_value(values[i % npmts], itmp, &current_bit, &huffman);
		values[i % npmts] = wfvalue;

		out << values[i % npmts] << "\t";
		if((i+1) % npmts == 0){
			out << std::endl;
		}
	}

    out.close();

	free(data);
	free(values);

	return 0;
}

