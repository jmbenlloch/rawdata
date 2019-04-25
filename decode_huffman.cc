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

int parse_huffman_line(std::string &line, Huffman * huffman){
	// copy line to cstring to use strtok function
	// (input has to be non-const)
	char * cstr = new char [line.length()+1];
	std::strcpy (cstr, line.c_str());

	int result;

	char *token;
	token = strtok (cstr," ,\t");
	result = std::stoi(token);
	printf("result: %d\n", result);

	token = strtok (NULL, " ,\t");
	printf("code: %s\n", token);

	Huffman * current_node = huffman;

	int bitcount = 0;
	unsigned char bit;
	while (token[bitcount]){
		bit = token[bitcount] - 0x30; // ascii to int
		printf("token[%d]: %c\n", bitcount, token[bitcount]);
		bitcount += 1;

		if(current_node->next[bit]){
			printf("bit already set\n");
			current_node = current_node->next[bit];
		}else{
			printf("bit not set\n");
			Huffman * new_node = (Huffman*) malloc(sizeof(Huffman));
			new_node->next[0] = NULL;
			new_node->next[1] = NULL;

			current_node->next[bit] = new_node;
			current_node = new_node;
		}
	}

	printf("bitcount: %d, %d\n", bitcount, bit);
	current_node->value = result;
	printf("huffman-value: %d\n", current_node->value);

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
	int position = 0;
	int bitcount = 7;
	unsigned char tmp = 0;

	if (inFile.is_open()){
		while(getline(inFile, line)) {
//			std::cout << line << std::endl;
			int size = line.size();
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
					printf("val: 0x%02x\n", tmp);
					position += 1;
					bitcount  = 7;
					tmp       = 0;
				}


				if (i > 32){
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
			printf("\n\nline: %s\n", line.c_str());

			parse_huffman_line(line, &huffman);
		}
	}

	printf("\n\n\nHuffman tree:\n");
	print_huffman(&huffman, 1);

	return 0;
}

