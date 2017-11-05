#include "catch.hpp"
#include "RawDataInput.h"

TEST_CASE("Test flip words", "[flip_words]") {
	const unsigned int size = 12;
	unsigned short data[size] = {0x0123, 0x4567, 0x89ab, 0xcdef,
		0xfedc,	0xba98, 0x7654, 0x3210, 0xffff, 0xffff, 0xfafa, 0xfafa};

	unsigned short flip[size] = {0x4567, 0x0123, 0xcdef, 0x89ab,
		0xba98, 0xfedc, 0x3210, 0x7654, 0xffff, 0xffff, 0xfafa, 0xfafa};

	int16_t * flip_ptr = (int16_t*) flip;
	int16_t test_data[size];

	flipWords(size, (int16_t*)data, test_data);

	unsigned int size_check = size - 2; // FAFAFAFA won't be copied

	for(unsigned int i=0; i < size_check; i++){
		REQUIRE(test_data[i] == flip_ptr[i]);
	}
}

TEST_CASE("Test seq counter removal", "[seqcounter_removal]") {
	const unsigned int size = 40000;
	int dataframe_size = 3996;
	int16_t data[40000];
	int16_t test_data[40000];
	memset(data, 0, sizeof(int16_t) * size);
	memset(test_data, 1, sizeof(int16_t) * size);

	//Write sequence counters in the data
	for(unsigned int i=0; i < size/dataframe_size; i++){
		data[dataframe_size*i + 1] = i;
	}

	flipWords(size, data, test_data);

	//Sequence counters will be removed, so the actual data
	//copied will be smaller than size
	unsigned int size_check = size - size/dataframe_size * 2;

	for(unsigned int i=0; i<size_check; i++){
		REQUIRE(test_data[i] == 0);
	}
}

TEST_CASE("Test PMT elecID", "[pmt_elecid]") {
	//We are using FEC 2-3 for channels 0-15
	//and FECs 10-11 for 16-31
	for(int ch=0; ch<8; ch++){
		REQUIRE(computePmtElecID( 2, ch) == ch);
		REQUIRE(computePmtElecID( 3, ch) == ch+8);
		REQUIRE(computePmtElecID(10, ch) == ch+16);
		REQUIRE(computePmtElecID(11, ch) == ch+24);
	}
}

TEST_CASE("Test SiPM Channel Mask", "[sipm_chmask]") {
	const unsigned int nwords = 4;
	const unsigned int sensors_per_word = 16;
	const unsigned int nsensors = nwords * sensors_per_word;
	unsigned short data[nwords] = {0x0000, 0x0000, 0x0000, 0x0000};
	std::vector<int> channelMaskVec;

	unsigned int nfebs = 28;

	next::RawDataInput rdata = next::RawDataInput();

	// 4 words: 1000, 0000, 0000, 0000 to 0000 0000 0000 0001
	SECTION("Test sensors one by one" ) {
		for(unsigned int feb=0; feb<nfebs; feb++){
			for(unsigned int ch=0; ch<nsensors; ch++){
				int word = ch / sensors_per_word;
				int pos  = ch % sensors_per_word;
				data[word] = 0x8000 >> pos;
				//printf("%d-%d-%d data: 0x%04x, 0x%04x, 0x%04x, 0x%04x\n", ch, word, pos, data[0], data[1], data[2], data[3]);

				// Call function and test it
				int16_t * ptr = (int16_t*) data;
				rdata.sipmChannelMask(ptr, channelMaskVec, feb);
				REQUIRE(channelMaskVec.size() == 1);
				REQUIRE(channelMaskVec[0] == SipmIDtoPosition((feb+1)*1000 + ch));

				//Clean values
				data[word] = 0;
				channelMaskVec.clear();
			}
		}
	}

	// Check also all ffff ffff ffff ffff
	SECTION("Test all sensors" ) {
		//Set all to active
		for (unsigned int i=0; i<nwords; i++){
			data[i] = 0xFFFF;
		}

		// Call function and test it
		for(unsigned int feb=0; feb<nfebs; feb++){
			int16_t * ptr = (int16_t*) data;
			rdata.sipmChannelMask(ptr, channelMaskVec, feb);
			REQUIRE(channelMaskVec.size() == nsensors);
			for(unsigned int ch=0; ch<nsensors; ch++){
				REQUIRE(channelMaskVec[ch] == SipmIDtoPosition((feb+1)*1000 + ch));
			}

			//Clean values
			channelMaskVec.clear();
		}
	}

	// Check some random case: 0010   0300 7000 2030
	SECTION("Test chmask with on and off sensors" ) {
		//Set config
		data[0] = 0x0010;
		data[1] = 0x0200;
		data[2] = 0x6000;
		data[3] = 0x5030;

		const unsigned int nchannels = 8;
		int channels[nchannels] = {11, 22, 33, 34, 49, 51, 58, 59};

		// Call function and test it
		for(unsigned int feb=0; feb<nfebs; feb++){
			int16_t * ptr = (int16_t*) data;
			rdata.sipmChannelMask(ptr, channelMaskVec, feb);
			REQUIRE(channelMaskVec.size() == nchannels);
			for(unsigned int ch=0; ch<nchannels; ch++){
				REQUIRE(channelMaskVec[ch] == SipmIDtoPosition((feb+1)*1000 + channels[ch]));
			}

			//Clean values
			channelMaskVec.clear();
		}
	}
}

TEST_CASE("Build SiPM data", "[sipm_data]") {
	const unsigned int size = 12;
	unsigned short data1[size] = {0x0000, 0x1111, 0x2222, 0x3333,
		0x4444,	0x5555, 0x6666, 0x7777, 0x8888, 0x9999, 0xaaaa, 0xbbbb};
	unsigned short data2[size] = {0x0123, 0x4567, 0x89ab, 0xcdef,
		0xfedc,	0xba98, 0x7654, 0x3210, 0x1234, 0x5678, 0x9abc, 0xdef0};

	int16_t * data1_ptr = (int16_t*) data1;
	int16_t * data2_ptr = (int16_t*) data2;
	int16_t test_data[2*size];

	buildSipmData(size, test_data, data1_ptr, data2_ptr);

	for(unsigned int i=0; i < size; i++){
		REQUIRE(test_data[2*i]   == data1_ptr[i]);
		REQUIRE(test_data[2*i+1] == data2_ptr[i]);
	}
}

TEST_CASE("Selected event", "[selected_event]") {
	eventHeaderStruct event;

	//Event types currently goes from 1 to 14
	for(unsigned int i=0; i<16; i++){
		event.eventType = i;
		bool condition = (i == PHYSICS_EVENT) || (i == CALIBRATION_EVENT);
		REQUIRE(isEventSelected(event) == condition);
	}
}

//TODO: test with 64 sensors one by one
TEST_CASE("Decode SiPM charge", "[sipm_charge]") {
	//Max sensors: 64
	//max words: 48 (64*12/16)
	//fill the end with 111...11

	// Test all sensors from 1 to 64, reading 0xFFF
	// Set initial data to zero
	const unsigned int max_sensors = 64;
	const unsigned int charge_size = 12;
	const unsigned int word_size = 16;
	const unsigned int max_words = max_sensors * charge_size / word_size;

	unsigned short data[max_words];
	memset(data, 0, sizeof(int16_t) * max_words);

	next::RawDataInput rdata = next::RawDataInput();

	//Create DigitCollection, create Digits and positions vectors
	next::DigitCollection digits;
	int positions[max_sensors];
	std::vector<int> channelMaskVec;

	SECTION("Test different amount of sensors (1 to 64)" ) {
		for(unsigned int nsensors=1; nsensors<=max_sensors; nsensors++){
			// Round up x/y: (x + y - 1) / y;
			unsigned int nwords = (nsensors*charge_size + word_size - 1) / word_size;
			//		printf("sensors: %d -> words: %d\n", nsensors, nwords);

			// Fill with 0xFFFF all the corresponding words
			for(unsigned word=0; word<nwords; word++){
				data[word] = 0xFFFF;
			}

			// Fill remaining part with 0 (in real data will be 1's,
			// but to detect errors now here, is better to put 0's)
			unsigned int remain = nwords * word_size - nsensors*charge_size;
			//	printf("remain: %d\n", remain);
			for(unsigned int bit=0; bit<remain; bit++){
				//data[nwords-1] = (data[nwords-1] & 0xFFFE) << 1;
				data[nwords-1] = data[nwords-1] << 1;
				//			printf("data[%d] = 0x%04x\n", nwords-1,data[nwords-1]);
			}

			//Create digits
			for(unsigned s=0; s<nsensors; s++){
				digits.emplace_back(s, next::digitType::RAW, next::chanType::PMT);
				positions[s] = s;
				channelMaskVec.push_back(s);
			}

			int16_t * ptr = (int16_t*) data;
			int time = 0;
			rdata.decodeCharge(ptr, digits, channelMaskVec, positions, time);

			for(unsigned word=0; word<nwords; word++){
				//			printf("data[%d] = 0x%04x\n", word, data[word]);
			}

			//Check there is one charge in each Digit and it's value is 0xFFF
			for(unsigned s=0; s<nsensors; s++){
				//			printf("s: %d\n", s);
				REQUIRE(digits[s].waveform().size() == 1);
				REQUIRE(digits[s].waveform()[0] == 0xFFF);
			}

			// Clean up
			memset(data, 0, sizeof(int16_t) * max_words);
			digits.clear();
			channelMaskVec.clear();
		}
	}

	SECTION("Test sensors one by one with 64 sensors") {
		/************************************************
		 *             Case0 | Case1 | Case2 | Case3
         * data[i  ] = FFF0  | 000F  | 0000  | 0000
         * data[i+1] = 0000  | FF00  | 00FF  | 0000
         * data[i+2] = 0000  | 0000  | F000  | 0FFF
         *
         * Start position (i):  sensor/4*3
         *  0,  1,  2,  3 -> 0
         *  4,  5,  6,  7 -> 3
         *  8,  9, 10, 11 -> 6
         * 12, 13, 14, 15 -> 9
		 **********************************************/
		for(unsigned int sensor=0; sensor<max_sensors; sensor++){
			unsigned int word = (sensor*charge_size + word_size - 1) / word_size;

			int start = sensor/4*3;
			switch(sensor % 4){
				case 0:
					data[start  ] = 0xFFF0;
					data[start+1] = 0x0000;
					data[start+2] = 0x0000;
					break;
				case 1:
					data[start  ] = 0x000F;
					data[start+1] = 0xFF00;
					data[start+2] = 0x0000;
					break;
				case 2:
					data[start  ] = 0x0000;
					data[start+1] = 0x00FF;
					data[start+2] = 0xF000;
					break;
				case 3:
					data[start  ] = 0x0000;
					data[start+1] = 0x0000;
					data[start+2] = 0x0FFF;
					break;
			}

			//for(unsigned i=0; i<48; i++){
			//	printf("data[%d] = 0x%04x\n", i, data[i]);
			//}

			//Prepare digits
			for(unsigned s=0; s<max_sensors; s++){
				digits.emplace_back(s, next::digitType::RAW, next::chanType::PMT);
				positions[s] = s;
				channelMaskVec.push_back(s);
			}

			//Decode data
			int16_t * ptr = (int16_t*) data;
			int time = 0;
			rdata.decodeCharge(ptr, digits, channelMaskVec, positions, time);

			// Test result
			REQUIRE(digits.size() == max_sensors);
			for(unsigned s=0; s<max_sensors; s++){
				REQUIRE(digits[s].chID() == s);
				REQUIRE(digits[s].waveform().size() == 1);
				//Only one sensor is 0xFFF, the rest of them are 0x000
				int value = (s == sensor) ? 0xFFF : 0x000;
				REQUIRE(digits[s].waveform()[0] == value);
			}

			// Clean up
			memset(data, 0, sizeof(int16_t) * max_words);
			digits.clear();
			channelMaskVec.clear();
		}
	}
}
