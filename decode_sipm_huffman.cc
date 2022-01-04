#include <iostream>
#include "config/ReadConfig.h"
#include "writer/CopyEvents.h"
#include "RawDataInput.h"
#include <fstream>
#include <cassert>

#ifndef _HDF5WRITER
#include "writer/HDF5Writer.h"
#endif

#ifndef SPDLOG_VERSION
#include "spdlog/spdlog.h"
#endif

namespace spd = spdlog;


// TODO: Filter 12 bits after reading charge


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


void decodeChargeIndiaPmtCompressed(int16_t* &ptr,
		std::map<int, int> &last_values,
	   	int * current_bit, Huffman * huffman,
	   	std::vector<int> &channelMaskVec, int time){
	int data = 0;

	for(int chan=0; chan<channelMaskVec.size(); chan++){
		// It is important to keep datatypes, memory allocation changes with them
		int16_t * charge_ptr = (int16_t *) &data;

		if(*current_bit < 16){
			ptr++;
			// printf("increment: 0x%04x\n", *ptr);
			*current_bit += 16;
		}

		memcpy(charge_ptr+1, ptr  , 2);
		memcpy(charge_ptr  , ptr+1, 2);

		// printf("charge_ptr: 0x%08x\n", *((int*)charge_ptr));


		// Get previous value
//		auto dgt = digits.begin() + positions[channelMaskVec[chan]];
		int previous = 0;
		int elec_id = channelMaskVec[chan];
		if (last_values.find(elec_id) != last_values.end()) {
			previous = last_values[elec_id];
		}
//		if (time){
//			previous = dgt->waveform()[time-1];
//		}


//		printf("word: 0x%04x\t, ElecID is %d\t Time is %d\t", data, channelMaskVec[chan], time);
		int wfvalue = decode_compressed_value(previous, data, current_bit, huffman);
		wfvalue = wfvalue & 0x0FFF;

		printf("ElecID is %d\t Time is %d\t Charge is 0x%04x\n", channelMaskVec[chan], time, wfvalue);
		last_values[elec_id] = wfvalue;

		//Save data in Digits
//		dgt->waveform()[time] = wfvalue;
	}

	// printf("\ncurrent_bit: %d\n", *current_bit);
	// printf("ptr: 0x%04x\n", *ptr);

	if (*current_bit >= 16){
		ptr++;
	}
	else{
		ptr+=2;
	}
}


unsigned char decode_ascii_char(char c){
	unsigned char result = 0;
//	printf("char: %d, 0x%01x\n", c,c);
	if ((c >= 0x30) && (c <= 0x39)){
		result = c - 0x30;
//		printf("1- %d\n", result);
	}
	if ((c >= 0x41) && (c <= 0x46)){
		result = c - 0x37;
//		printf("2- %d\n", result);
	}
	if ((c >= 0x61) && (c <= 0x66)){
		result = c - 0x57;
//		printf("3- %d\n", result);
	}

//	printf("0x%01x\n", result);
	return result;
}

int16_t decode_ascii_word(std::string& word){
	int16_t result = 0;

    assert(word.size() == 5);

	for (int i=0; i<4; i++){
		result = (result << 4) | decode_ascii_char(word[i]);
		// printf("0x%04x- 0x%01x\n", result, decode_ascii_char(word[i]));
	}
	return result;
}


//There are 4 16-bit words with the channel mask for SiPMs
//MSB ch63, LSB ch0
//Data came after from 0 to 63
int sipmChannelMask(int16_t * &ptr, std::vector<int> &channelMaskVec, int febId){
	int TotalNumberOfSiPMs = 0;
	int temp;

	int ElecID, sipmNumber;

	channelMaskVec.clear();
	for(int l=4; l>0; l--){
		temp = (*ptr) & 0x0FFFF;
		for (int t=0; t < 16; t++){
			int bit = CheckBit(temp, 15-t);

			ElecID = (febId+1)*1000 + l*16 - t - 1;

			if(bit>0){
				channelMaskVec.push_back(ElecID);
				TotalNumberOfSiPMs++;
			}
		}
		ptr++;
	}
	std::sort(channelMaskVec.begin(), channelMaskVec.end());
	return TotalNumberOfSiPMs;
}


void decodeCharge(int16_t* &ptr, std::vector<int> &channelMaskVec, int time){
	//Raw Mode
	int Charge = 0;
	int16_t * payloadCharge_ptr = ptr;

	//We have 64 SiPM per FEB
	for(int chan=0; chan<channelMaskVec.size(); chan++){
		// It is important to keep datatypes, memory allocation changes with them
		int16_t * charge_ptr = (int16_t *) &Charge;

		memcpy(charge_ptr+1, payloadCharge_ptr, 2);
		memcpy(charge_ptr, payloadCharge_ptr+1, 2);

		switch(chan % 4){
			case 0:
				Charge = Charge >> 20;
				Charge = Charge & 0x0fff;
				break;
			case 1:
				Charge = Charge >> 8;
				Charge = Charge & 0x0fff;
				break;
			case 2:
				Charge = Charge >> 12;
				Charge = Charge & 0x0fff;
				break;
			case 3:
				Charge = Charge & 0x0fff;
				break;
		}
		if((chan % 4)==1){
			payloadCharge_ptr+=1;
		}
		if((chan % 4)==3){
			payloadCharge_ptr+=2;
		}

		printf("ElecID is %d\t Time is %d\t Charge is 0x%04x\n", channelMaskVec[chan], time, Charge);

		// Channel 3 does not add new words
		// 3 words (0123,4567,89AB) give 4 charges (012,345,678,9AB)
		// (012)(3 45)(67 8)(9AB)
		if((chan % 4) != 3){
			ptr++;
		}
	}
}


void process_evt(int16_t * buffer, int size, bool compressed, Huffman * huffman){
	int16_t *payload_ptr = buffer;
	int count = 0;

	//Map febid -> channelmask
	std::map<int, std::vector<int> > feb_chmask;

	//Map elec_id -> last_value
	std::map<int, int> last_values;

	while (count < size){
		if (count == size-1){
			printf("stop condition");
			break;
		}

		printf("\n\n\n");

//		for (int i=0; i<40; i++){
//		    printf("[%d]: 0x%04x\n", payload_ptr-buffer+i, *(payload_ptr+i));
//		}

		int FEBId = ((*payload_ptr) & 0x0FC00) >> 10;
		feb_chmask.emplace(FEBId, std::vector<int>());
		payload_ptr++;

		int ft = (*payload_ptr) & 0x0FFFF;
		payload_ptr++;

		int n_channels = sipmChannelMask(payload_ptr, feb_chmask[FEBId], FEBId);
		printf("n_channels: %d\n", n_channels);
		for (int i=0; i<feb_chmask[FEBId].size(); i++){
			std::cout << "ch: " << feb_chmask[FEBId][i] << std::endl;
		}

		if (compressed){
			// std::cout << "process compressed data" << std::endl;
			int current_bit = 31;
			decodeChargeIndiaPmtCompressed(payload_ptr, last_values, &current_bit, huffman, feb_chmask[FEBId], ft);
		}
		else{
			decodeCharge(payload_ptr, feb_chmask[FEBId], ft);
		}

		count = payload_ptr - buffer;
		// printf("count: %d, size: %d\n", count, size);
	}
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

	bool compressed = config.compressed();
	std::cout << "Compressed data: " << compressed << std::endl;

	// Event buffer and position counter
	int16_t * evt_data = (int16_t *) malloc(500000);
	memset(evt_data, 0, 500000);
	int count = 0;


	// Read Huffman tree from DB
	Huffman huffman_;
	huffman_.next[0] = NULL;
	huffman_.next[1] = NULL;
	getHuffmanFromDB(&config, &huffman_, 8000);
	print_huffman(&huffman_, 1);


	// Read input file
	std::string line;
	std::ifstream filein(config.file_in());
	if (filein.is_open()){
		while (getline(filein,line)){
//			std::cout << line << "\t" << line.size();

			int16_t data = decode_ascii_word(line);
//			printf("\t 0x%04x\n", data);
			evt_data[count] = data;
			count++;

			if (line.compare(0,4,"    ") == 0){
				std::cout << "New event" << '\n';
				process_evt(evt_data, count, compressed, &huffman_);
				memset(evt_data, 0, 500000);
				count = 0;
			}
		}
		filein.close();
	}
	else std::cout << "Unable to open file";

	return 0;
}

