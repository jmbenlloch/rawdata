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
	printf("position: %d, tmp: 0x%08x\n", position, tmp);

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
	int final_pos = position;

	if(! (huffman->next[0] || huffman->next[1])){
		*result = huffman->value;
	}else{
		final_pos = decode_huffman(huffman->next[bit], code, position+1, result);
	}

	return position;
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

	std::cout << "file in: "      << config.file_in() << std::endl;
	std::cout << "file out: "      << config.file_out() << std::endl;
	std::cout << "huffman tree: " << config.huffman_tree() << std::endl;
	std::cout << "npmts: "        << config.npmts() << std::endl;

	std::ifstream inFile;
	std::string line;
	inFile.open(config.file_in());

	unsigned char * data;
	int size = 0;
	int position = 0;
	int bitcount = 7;
	unsigned char tmp = 0;

	if (inFile.is_open()){
		while(getline(inFile, line)) {
//			std::cout << line << std::endl;
			size = line.size();
			std::cout << "size: " << size << std::endl;
			data = (unsigned char*) malloc(size);
			memset(data, 0, size);

			for(int i=0; i<size; i++){
//				std::cout << line[i] << std::endl;

				unsigned char value = line[i] - 0x30; // convert ASCII to int
//				printf("line[%d]: %c, %d\n", i, line[i], value);

				tmp = tmp | (value << bitcount);
				bitcount -= 1;
//				printf("tmp: 0x%04x\n", tmp);

				if (bitcount < 0){
					data[position] = tmp;

					printf("val: 0x%02x\n", tmp);
					position += 1;
					bitcount  = 7;
					tmp       = 0;
				}

				if (i > 512){
					break;
				}
			}
		}
		inFile.close();
	}


	Huffman huffman;
	huffman.next[0] = NULL;
	huffman.next[1] = NULL;

	// read huffman tree
	inFile.open(config.huffman_tree());
	if (inFile.is_open()){
		while(getline(inFile, line)) {
//			printf("\n\nline: %s\n", line.c_str());
			parse_huffman_line(line, &huffman);
		}
	}

	printf("\n\n\nHuffman tree:\n");
	print_huffman(&huffman, 1);


	// Start processing data
	int itmp = 0;
	unsigned char * payload_ptr = data;

	int position_dec = 0;
	int current_bit = 31 - config.offset();
	printf("current_bit: %d\n", current_bit);

	for(int i=0; i<32; i++){
		printf("data[%d]: %02x", i, data[i*4  ]);
		printf("%02x",              data[i*4+1]);
		printf("%02x",              data[i*4+2]);
		printf("%02x\n",            data[i*4+3]);
	}

	for(int i=0; i<32; i++){
		printf("\n\n new value, i: %d\n", i);
		int nbits;

		// second half of the word -> load next one
		if(current_bit < 16){
			// move as many 8-bit words as needed
			int nwords = (31 - current_bit) / 8;
			current_bit += nwords * 8;
			printf("nwords: %d\n", nwords);
			printf("current_bits: %d\n", current_bit);
			position_dec += nwords;

		}

		itmp = read_data(payload_ptr, position_dec);

		int type = CheckBit(itmp, current_bit);
		current_bit -= 1;
		printf("current_bit: %d\n", current_bit);

		printf("type: %d\n", type);

		int wfvalue;
		if(type == 0){
			printf("current_bit: %d\n", current_bit);
			wfvalue = (itmp >> (current_bit - 11)) & 0x0FFF;
			nbits = 12;
		}else{
			nbits = decode_huffman(&huffman, itmp, current_bit, &wfvalue);
		}

		current_bit -= nbits;

		printf("Value: 0x%04x\n", wfvalue);

		itmp = 0;
		
	}

	return 0;
}

