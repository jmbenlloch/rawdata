#include "database/huffman.h"

//int parse_huffman_line(std::string &line, Huffman * huffman){
int parse_huffman_line(int value, char * code, Huffman * huffman){
	Huffman * current_node = huffman;
	// printf("code: %s, value: %d\n", code, value);

	int bitcount = 0;
	unsigned char bit;
	while (code[bitcount]){
		bit = code[bitcount] - 0x30; // ascii to int
		// printf("code[%d]: %c\n", bitcount, code[bitcount]);
		bitcount += 1;

		if(current_node->next[bit]){
			// printf("bit already set\n");
			current_node = current_node->next[bit];
		}else{
			// printf("bit not set\n");
			Huffman * new_node = (Huffman*) malloc(sizeof(Huffman));
			new_node->next[0] = NULL;
			new_node->next[1] = NULL;

			current_node->next[bit] = new_node;
			current_node = new_node;
		}
	}

	// printf("bitcount: %d, %d\n", bitcount, bit);
	current_node->value = value;
	// printf("huffman-value: %d\n", current_node->value);

	return value;
}


void printf_huffman(Huffman * huffman, int code){
	// printf("code: %d\n", code);
	// printf("next[0]: 0x%x\n", huffman->next[0]);
	// printf("next[1]: 0x%x\n", huffman->next[1]);
	for(int i=0; i<2; i++){
		if(huffman->next[i]){
			int new_code = code * 10 + i;
			printf_huffman(huffman->next[i], new_code);
		}
	}

	if(! (huffman->next[0] || huffman->next[1])){
		printf("%d -> %d\n", code, huffman->value);
	}
}

void print_huffman(std::shared_ptr<spdlog::logger> log, Huffman * huffman, int code){
	for(int i=0; i<2; i++){
		if(huffman->next[i]){
			int new_code = code * 10 + i;
			print_huffman(log, huffman->next[i], new_code);
		}
	}

	if(! (huffman->next[0] || huffman->next[1])){
		log->debug("{}, {}", code, huffman->value);
	}
}

// start_bit will be modified to set the new position
int decode_compressed_value(int previous_value, int data, int control_code, int * start_bit, Huffman * huffman){
	// Check data type (0 uncompressed, 1 huffman)
	int current_bit = *start_bit;

	int wfvalue;

	// printf("call decode_huffman: data 0x%08x, current_bit: %d\n", data, current_bit);
	current_bit = decode_huffman(huffman, data, current_bit, &wfvalue);
	// printf("value: 0x%04x\n", wfvalue);

	if(wfvalue == control_code){
		wfvalue = (data >> (current_bit - 11)) & 0x0FFF;
		current_bit -= 12;
		// printf("12-bit wfvalue: %d\n", wfvalue);
	}else{
		wfvalue = previous_value + wfvalue;
	}
	*start_bit = current_bit;
	// printf("curren bit: %d\n", current_bit);

	return wfvalue;
}

short int decode_huffman(Huffman * huffman, int code, int position, int * result){
	int bit = CheckBit(code, position);
	// printf("pos: %d, bit: %d\n", position, bit);

	int final_pos = position;

	// printf("next[0]: 0x%x\n", huffman->next[0]);
	// printf("next[1]: 0x%x\n", huffman->next[1]);

	if(! (huffman->next[0] || huffman->next[1])){
		*result = huffman->value;
	}else{
		final_pos = decode_huffman(huffman->next[bit], code, position-1, result);
	}
	// printf("code: 0x%04x, pos: %d, result: %d\n", code, position, *result);
	// printf("final_pos: %d\n", final_pos);

	return final_pos;
}
