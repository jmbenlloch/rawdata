#ifndef _HUFFMAN
#define _HUFFMAN
#endif

#include <iostream>

#ifndef _READCONFIG
#include "config/ReadConfig.h"
#endif

#ifndef SPDLOG_VERSION
#include "spdlog/spdlog.h"
#endif

#ifndef _EVENTREADER
#include "detail/EventReader.h"
#endif

struct Huffman {
	int value;
	Huffman * next[2];
};

//int parse_huffman_line(std::string &line, Huffman * huffman);
//void print_huffman(Huffman * huffman, int code);
void print_huffman(std::shared_ptr<spdlog::logger> log, Huffman * huffman, int code);
void printf_huffman(Huffman * huffman, int code);
int parse_huffman_line(int value, char * code, Huffman * huffman);
int decode_compressed_value(int previous_value, int data, int * start_bit, Huffman * huffman);
short int decode_huffman(Huffman * huffman, int code, int position, int * result);

