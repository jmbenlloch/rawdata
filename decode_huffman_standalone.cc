#include <iostream>
#include <cstring>
#include <string>
#include <fstream>

struct Huffman {
	int value;
	Huffman * next[2];
};

int CheckBit(int mask, int pos){
	return (mask & (1 << pos)) != 0;
}

int read_data(unsigned char * data, int position){
	int tmp = 0;

	tmp = tmp | data[position    ] << 24;
	tmp = tmp | data[position + 1] << 16;
	tmp = tmp | data[position + 2] <<  8;
	tmp = tmp | data[position + 3];

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
		final_pos = decode_huffman(huffman->next[bit], code, position-1, result);
	}

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

	token = strtok (NULL, " ,\t");

	Huffman * current_node = huffman;

	int bitcount = 0;
	unsigned char bit;
	while (token[bitcount]){
		bit = token[bitcount] - 0x30; // ascii to int
		bitcount += 1;

		if(current_node->next[bit]){
			current_node = current_node->next[bit];
		}else{
			Huffman * new_node = (Huffman*) malloc(sizeof(Huffman));
			new_node->next[0] = NULL;
			new_node->next[1] = NULL;

			current_node->next[bit] = new_node;
			current_node = new_node;
		}
	}

	current_node->value = result;

	delete[] cstr;

	return result;
}


void read_huffman_file(std::string filein, Huffman * huffman){
	std::ifstream inFile;
	std::string line;
	inFile.open(filein);

	if (inFile.is_open()){
		while(getline(inFile, line)) {
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
			size = line.size();
			std::cout << "size: " << size << std::endl;
			*data = (unsigned char*) malloc(size);
			memset(*data, 0, size);

			for(int i=0; i<size; i++){
				unsigned char value = line[i] - 0x30; // convert ASCII to int

				tmp = tmp | (value << bitcount);
				bitcount -= 1;

				if (bitcount < 0){
					(*data)[position] = tmp;
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

	if (argc < 6){
		std::cout << "Usage: rawdatareader <filein> <tree_file> <fileout> <npmts> <offset>" << std::endl;
		return 1;
	}

	std::string filein    = std::string(argv[1]);
	std::string tree_file = std::string(argv[2]);
	std::string fileout   = std::string(argv[3]);
	int npmts             = std::stoi  (argv[4]);
	int offset            = std::stoi  (argv[5]);

	std::cout << "Huffman tree: " << tree_file << std::endl;
	std::cout << "Input   file: " << filein    << std::endl;
	std::cout << "Output  file: " << fileout   << std::endl;
	std::cout << "PMTs: " << npmts << std::endl;

	// Load waveforms data from txt file
	unsigned char * data;
	int size = read_data_file(filein, &data);

	// Create Huffman tree
	Huffman huffman;
	huffman.next[0] = NULL;
	huffman.next[1] = NULL;

	read_huffman_file(tree_file, &huffman);

	std::cout << "Huffman tree:" << std::endl;
	print_huffman(&huffman, 1);

	// Start processing data
	int payload = 0;

	int position_dec = 0;
	int current_bit = 31 - offset;

	int * values = (int *) malloc(sizeof(int) * npmts);

    std::ofstream out(fileout);

	int i=0;
	while(position_dec < (size/8)){
		// second half of the word -> load next one
		if(current_bit < 16){
			// move as many 8-bit words as needed
			int nwords = (31 - current_bit) / 8;
			current_bit += nwords * 8;
			position_dec += nwords;

		}

		payload = read_data(data, position_dec);

		int wfvalue = decode_compressed_value(values[i % npmts], payload, &current_bit, &huffman);
		values[i % npmts] = wfvalue;

		out << values[i % npmts] << "\t";
		if((i+1) % npmts == 0){
			out << std::endl;
		}
		i += 1;
	}

    out.close();

	free(data);
	free(values);

	return 0;
}

