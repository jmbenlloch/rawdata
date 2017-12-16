#include "catch.hpp"

#ifndef SPDLOG_VERSION
#include "spdlog/spdlog.h"
#endif
#include "detail/EventReader.h"


TEST_CASE("Check Bit", "[check_bit]") {
	int mask = 1;

	for(int pos=0; pos<32; pos++){
		for(int i=0; i<32; i++){
			REQUIRE(CheckBit(mask, i) == (int)(i==pos));
		}
		mask = mask << 1;
	}
}

TEST_CASE( "Create event reader", "[eventreader]" ) {
	unsigned short pmt_header[21] = {
		0x0000, //Sequence counter H
		0x0000, //Sequence counter L
		0x00c0, //Format ID H
		0x0008, //Format ID L
		0x0f96, //Wordcount
		0x0009, //Event ID H
		0x0001, //Event ID L
		0x6590, //Event Conf0
		0x32c8, //Event Conf1
		0x33ff, //Event Conf2
		0x9d59, //Baseline
		0xd49d, //Baseline
		0x59d4, //Baseline
		0x9d69, //Baseline
		0xa59d, //Baseline
		0x69a5, //Baseline
		0x0048, //FEC ID
		0x56e2, //CT H
		0xa3d7, //CT L
		0x03e8, // FTH & CTms
		0x29c2, //FT L
	};

	int16_t * ptr = (int16_t*) pmt_header;
	int16_t * ptr_orig = (int16_t*) pmt_header;

	spdlog::drop("eventreader");
	next::EventReader* reader = new next::EventReader(0);

    SECTION("Test common header reader" ) {
		reader->ReadCommonHeader(ptr);
		REQUIRE(reader->SequenceCounter() == 0);
    	//Test Format ID
		REQUIRE(reader->FecType() == 0);
		REQUIRE(reader->ZeroSuppression() == 0);
		REQUIRE(reader->Baseline() == 1);
		REQUIRE(reader->GetDualModeBit() == 1);
		REQUIRE(reader->GetErrorBit() == 0);
		REQUIRE(reader->FWVersion() == 8);
    	//Test Word count
		REQUIRE(reader->WordCounter() == 0x0f96);
        //Test Event ID
		REQUIRE(reader->TriggerType() == 9);
		REQUIRE(reader->TriggerCounter() == 1);
		//Test Event Conf
		REQUIRE(reader->BufferSamples() == 52000);
		REQUIRE(reader->PreTriggerSamples() == 26000);
		//Test Baselines
		REQUIRE(reader->Pedestal(0) == 2517);
		REQUIRE(reader->Pedestal(1) == 2516);
		REQUIRE(reader->Pedestal(2) == 2517);
		REQUIRE(reader->Pedestal(3) == 2516);
		REQUIRE(reader->Pedestal(4) == 2518);
		REQUIRE(reader->Pedestal(5) == 2469);
		REQUIRE(reader->Pedestal(6) == 2518);
		REQUIRE(reader->Pedestal(7) == 2469);
		//Test FEC ID
		REQUIRE(reader->FecId() == 2);
		REQUIRE(reader->NumberOfChannels() == 8);
		REQUIRE(reader->DualModeMask() == 0x0033);
		REQUIRE(reader->ChannelMask() == 0x00FF);
		//Test CT and FTh
		REQUIRE(reader->TimeStamp() == 1492678303720);
		REQUIRE(reader->GetFTBit() == 0);
		//Test FTl
		REQUIRE(reader->TriggerFT() == 0x29c2);

		REQUIRE(ptr == ptr_orig + 21);
	}
}


TEST_CASE( "Test sequence counter", "[seq_counter]" ) {
	unsigned short data[2] = {
		0xFFFF, //Sequence counter H
		0xFFFF, //Sequence counter L
	};

	int16_t * ptr = (int16_t*) data;
	int16_t * ptr_orig = ptr;

	spdlog::drop("eventreader");
	next::EventReader* reader = new next::EventReader(0);

    SECTION("All active" ) {
		reader->readSeqCounter(ptr);
		REQUIRE(reader->SequenceCounter() == 0xFFFFFFFF);
		REQUIRE(ptr == ptr_orig + 2);
	}

	//spdlogs raise an exception between sections for some reason
	spdlog::drop("eventreader");
	data[0] = 0;
	data[1] = 0;

    SECTION("All inactive" ) {
		reader->readSeqCounter(ptr);
		REQUIRE(reader->SequenceCounter() == 0);
		REQUIRE(ptr == ptr_orig + 2);
	}
}

TEST_CASE( "Test Fec Type", "[fec_type]" ) {
	unsigned short data[2] = {
		0x000F, // Format ID H
		0x0000, // Format ID L
	};

	int16_t * ptr = (int16_t*) data;
	int16_t * ptr_orig = ptr;

	spdlog::drop("eventreader");
	next::EventReader* reader = new next::EventReader(0);

    SECTION("All active" ) {
		reader->readFormatID(ptr);
		REQUIRE(reader->FecType() == 0x0F);
		REQUIRE(ptr == ptr_orig + 2);
	}

	//spdlogs raise an exception between sections for some reason
	spdlog::drop("eventreader");
	data[0] = 0xFFF0;
	data[1] = 0xFFFF;

    SECTION("All inactive" ) {
		reader->readFormatID(ptr);
		REQUIRE(reader->FecType() == 0);
		REQUIRE(ptr == ptr_orig + 2);
	}
}


TEST_CASE( "Test Zero Suppresion", "[zs]" ) {
	unsigned short data[2] = {
		0x0010, // Format ID H
		0x0000, // Format ID L
	};

	int16_t * ptr = (int16_t*) data;
	int16_t * ptr_orig = ptr;

	spdlog::drop("eventreader");
	next::EventReader* reader = new next::EventReader(0);

    SECTION("All active" ) {
		reader->readFormatID(ptr);
		REQUIRE(reader->ZeroSuppression() == 1);
		REQUIRE(ptr == ptr_orig + 2);
	}

	//spdlogs raise an exception between sections for some reason
	spdlog::drop("eventreader");
	data[0] = 0xFFEF;
	data[1] = 0xFFFF;

    SECTION("All inactive" ) {
		reader->readFormatID(ptr);
		REQUIRE(reader->ZeroSuppression() == 0);
		REQUIRE(ptr == ptr_orig + 2);
	}
}

TEST_CASE( "Test baseline", "[baseline]" ) {
	unsigned short data[2] = {
		0x0040, // Format ID H
		0x0000, // Format ID L
	};

	int16_t * ptr = (int16_t*) data;
	int16_t * ptr_orig = ptr;

	spdlog::drop("eventreader");
	next::EventReader* reader = new next::EventReader(0);

    SECTION("All active" ) {
		reader->readFormatID(ptr);
		REQUIRE(reader->Baseline() == 1);
		REQUIRE(ptr == ptr_orig + 2);
	}

	//spdlogs raise an exception between sections for some reason
	spdlog::drop("eventreader");
	data[0] = 0xFFBF;
	data[1] = 0xFFFF;

    SECTION("All inactive" ) {
		reader->readFormatID(ptr);
		REQUIRE(reader->Baseline() == 0);
		REQUIRE(ptr == ptr_orig + 2);
	}
}

TEST_CASE( "Test dual mode bit", "[dualmodebit]" ) {
	unsigned short data[2] = {
		0x0080, // Format ID H
		0x0000, // Format ID L
	};

	int16_t * ptr = (int16_t*) data;
	int16_t * ptr_orig = ptr;

	spdlog::drop("eventreader");
	next::EventReader* reader = new next::EventReader(0);

    SECTION("All active" ) {
		reader->readFormatID(ptr);
		REQUIRE(reader->GetDualModeBit() == 1);
		REQUIRE(ptr == ptr_orig + 2);
	}

	//spdlogs raise an exception between sections for some reason
	spdlog::drop("eventreader");
	data[0] = 0xFF7F;
	data[1] = 0xFFFF;

    SECTION("All inactive" ) {
		reader->readFormatID(ptr);
		REQUIRE(reader->GetDualModeBit() == 0);
		REQUIRE(ptr == ptr_orig + 2);
	}
}

TEST_CASE( "Test error bit", "[errorbit]" ) {
	unsigned short data[2] = {
		0x4000, // Format ID H
		0x0000, // Format ID L
	};

	int16_t * ptr = (int16_t*) data;
	int16_t * ptr_orig = ptr;

	spdlog::drop("eventreader");
	next::EventReader* reader = new next::EventReader(0);

    SECTION("All active" ) {
		reader->readFormatID(ptr);
		REQUIRE(reader->GetErrorBit() == 1);
		REQUIRE(ptr == ptr_orig + 2);
	}

	//spdlogs raise an exception between sections for some reason
	spdlog::drop("eventreader");
	data[0] = 0xBFFF;
	data[1] = 0xFFFF;

    SECTION("All inactive" ) {
		reader->readFormatID(ptr);
		REQUIRE(reader->GetErrorBit() == 0);
		REQUIRE(ptr == ptr_orig + 2);
	}
}

TEST_CASE( "Test FW version", "[fwversion]" ) {
	unsigned short data[2] = {
		0x0000, // Format ID H
		0xFFFF, // Format ID L
	};

	int16_t * ptr = (int16_t*) data;
	int16_t * ptr_orig = ptr;

	spdlog::drop("eventreader");
	next::EventReader* reader = new next::EventReader(0);

    SECTION("All active" ) {
		reader->readFormatID(ptr);
		REQUIRE(reader->FWVersion() == 0xFFFF);
		REQUIRE(ptr == ptr_orig + 2);
	}

	//spdlogs raise an exception between sections for some reason
	spdlog::drop("eventreader");
	data[0] = 0xBFFF;
	data[1] = 0xFFFF;

    SECTION("All inactive" ) {
		reader->readFormatID(ptr);
		REQUIRE(reader->GetErrorBit() == 0);
		REQUIRE(ptr == ptr_orig + 2);
	}
}

TEST_CASE( "Test word count", "[wordcount]" ) {
	unsigned short data[1] = {
		0xFFFF, // wordcount
	};

	int16_t * ptr = (int16_t*) data;
	int16_t * ptr_orig = ptr;

	spdlog::drop("eventreader");
	next::EventReader* reader = new next::EventReader(0);

    SECTION("All active" ) {
		reader->readWordCount(ptr);
		REQUIRE(reader->WordCounter() == 0xFFFF);
		REQUIRE(ptr == ptr_orig + 1);
	}

	//spdlogs raise an exception between sections for some reason
	spdlog::drop("eventreader");
	data[0] = 0;

    SECTION("All inactive" ) {
		reader->readWordCount(ptr);
		REQUIRE(reader->WordCounter() == 0);
		REQUIRE(ptr == ptr_orig + 1);
	}
}

TEST_CASE( "Test Trigger type", "[trigger_type]" ) {
	unsigned short data[2] = {
		0x000F, // Event ID H
		0x0000, // Event ID L
	};

	int16_t * ptr = (int16_t*) data;
	int16_t * ptr_orig = ptr;

	spdlog::drop("eventreader");
	next::EventReader* reader = new next::EventReader(0);

    SECTION("All active" ) {
		reader->readEventID(ptr);
		REQUIRE(reader->TriggerType() == 0xF);
		REQUIRE(ptr == ptr_orig + 2);
	}

	//spdlogs raise an exception between sections for some reason
	spdlog::drop("eventreader");
	data[0] = 0xFFF0;
	data[1] = 0xFFFF;

    SECTION("All inactive" ) {
		reader->readEventID(ptr);
		REQUIRE(reader->TriggerType() == 0);
		REQUIRE(ptr == ptr_orig + 2);
	}
}

TEST_CASE( "Test Trigger counter", "[trigger_counter]" ) {
	unsigned short data[2] = {
		0xFFF0, // Event ID H
		0xFFFF, // Event ID L
	};

	int16_t * ptr = (int16_t*) data;
	int16_t * ptr_orig = ptr;

	spdlog::drop("eventreader");
	next::EventReader* reader = new next::EventReader(0);

    SECTION("All active" ) {
		reader->readEventID(ptr);
		REQUIRE(reader->TriggerCounter() == 0xFFFFFFF);
		REQUIRE(ptr == ptr_orig + 2);
	}

	//spdlogs raise an exception between sections for some reason
	spdlog::drop("eventreader");
	data[0] = 0x000F;
	data[1] = 0x0000;

    SECTION("All inactive" ) {
		reader->readEventID(ptr);
		REQUIRE(reader->TriggerCounter() == 0);
		REQUIRE(ptr == ptr_orig + 2);
	}
}

TEST_CASE( "Test Buffer samples", "[buffer_samples]" ) {
	unsigned short data[3] = {
		0xFFFF, // Event Conf 0
		0x0000, // Event Conf 1
		0x0000, // Event Conf 2
	};

	int16_t * ptr = (int16_t*) data;
	int16_t * ptr_orig = ptr;

	spdlog::drop("eventreader");
	next::EventReader* reader = new next::EventReader(0);

    SECTION("All active") {
		reader->readEventConf(ptr);
		REQUIRE(reader->BufferSamples() == 2 * 0xFFFF);
		REQUIRE(ptr == ptr_orig + 3);
	}

	//spdlogs raise an exception between sections for some reason
	spdlog::drop("eventreader");
	data[0] = 0x0000;
	data[1] = 0xFFFF;
	data[2] = 0xFFFF;

    SECTION("All inactive" ) {
		reader->readEventConf(ptr);
		REQUIRE(reader->BufferSamples() == 0);
		REQUIRE(ptr == ptr_orig + 3);
	}
}

TEST_CASE( "Test PreTrigger samples", "[pretrigger_samples]" ) {
	unsigned short data[3] = {
		0x0000, // Event Conf 0
		0xFFFF, // Event Conf 1
		0x0000, // Event Conf 2
	};

	int16_t * ptr = (int16_t*) data;
	int16_t * ptr_orig = ptr;

	spdlog::drop("eventreader");
	next::EventReader* reader = new next::EventReader(0);

    SECTION("All active") {
		reader->readEventConf(ptr);
		REQUIRE(reader->PreTriggerSamples() == 2 * 0xFFFF);
		REQUIRE(ptr == ptr_orig + 3);
	}

	//spdlogs raise an exception between sections for some reason
	spdlog::drop("eventreader");
	data[0] = 0xFFFF;
	data[1] = 0x0000;
	data[2] = 0xFFFF;

    SECTION("All inactive" ) {
		reader->readEventConf(ptr);
		REQUIRE(reader->PreTriggerSamples() == 0);
		REQUIRE(ptr == ptr_orig + 3);
	}
}

TEST_CASE( "Test Channel mask", "[channelmask]" ) {
	unsigned short data[3] = {
		0x0000, // Event Conf 0
		0x0000, // Event Conf 1
		0xFFFF, // Event Conf 2
	};

	int16_t * ptr = (int16_t*) data;
	int16_t * ptr_orig = ptr;

	spdlog::drop("eventreader");
	next::EventReader* reader = new next::EventReader(0);

    SECTION("All active") {
		reader->readEventConf(ptr);
		REQUIRE(reader->ChannelMask() == 0xFFFF);
		REQUIRE(ptr == ptr_orig + 3);
	}

	//spdlogs raise an exception between sections for some reason
	spdlog::drop("eventreader");
	data[0] = 0xFFFF;
	data[1] = 0xFFFF;
	data[2] = 0x0000;

    SECTION("All inactive" ) {
		reader->readEventConf(ptr);
		REQUIRE(reader->ChannelMask() == 0);
		REQUIRE(ptr == ptr_orig + 3);
	}
}

TEST_CASE( "Test FEC ID", "[fecid]" ) {
	unsigned short data[1] = {
		0xFFE0, // FEC ID
	};

	int16_t * ptr = (int16_t*) data;
	int16_t * ptr_orig = ptr;

	spdlog::drop("eventreader");
	next::EventReader* reader = new next::EventReader(0);

    SECTION("All active" ) {
		reader->readFecID(ptr);
		REQUIRE(reader->FecId() == 0x07FF);
		REQUIRE(ptr == ptr_orig + 1);
	}

	//spdlogs raise an exception between sections for some reason
	spdlog::drop("eventreader");
	data[0] = 0x001F;

    SECTION("All inactive" ) {
		reader->readFecID(ptr);
		REQUIRE(reader->FecId() == 0);
		REQUIRE(ptr == ptr_orig + 1);
	}
}

TEST_CASE( "Test FEC ID and chmask", "[fecid-chmask]" ) {
	unsigned short data[1] = {
		0xFFE0, // FEC ID
	};

	int16_t * ptr = (int16_t*) data;
	int16_t * ptr_orig = ptr;

	spdlog::drop("eventreader");
	next::EventReader* reader = new next::EventReader(0);
	reader->SetDualModeBit(0);
	reader->SetChannelMask(0xFFFF);
    SECTION("nodual_fec_odd" ) {
		reader->readFecID(ptr);
		REQUIRE(reader->FecId() == 0x07FF);
		REQUIRE(reader->ChannelMask() == 0xFFFF);
		REQUIRE(reader->DualModeMask() == 0);
		REQUIRE(ptr == ptr_orig + 1);
	}

	data[0] = 0;
	reader->SetDualModeBit(0);
	reader->SetChannelMask(0xFFFF);
    SECTION("nodual_fec_even" ) {
		reader->readFecID(ptr);
		REQUIRE(reader->FecId() == 0);
		REQUIRE(reader->ChannelMask() == 0xFFFF);
		REQUIRE(reader->DualModeMask() == 0);
		REQUIRE(ptr == ptr_orig + 1);
	}

	data[0] = 0;
	reader->SetDualModeBit(1);
	reader->SetChannelMask(0x00FF);
    SECTION("dual_dmmask_fec_even" ) {
		reader->readFecID(ptr);
		REQUIRE(reader->FecId() == 0);
		REQUIRE(reader->ChannelMask() == 0x00FF);
		REQUIRE(reader->DualModeMask() == 0);
		REQUIRE(ptr == ptr_orig + 1);
	}

	data[0] = 0xFFE0;
	reader->SetDualModeBit(1);
	reader->SetChannelMask(0x00FF);
    SECTION("dual_chmask_fec_odd" ) {
		reader->readFecID(ptr);
		REQUIRE(reader->FecId() == 0x07FF);
		REQUIRE(reader->ChannelMask() == 0);
		REQUIRE(reader->DualModeMask() == 0x00FF);
		REQUIRE(ptr == ptr_orig + 1);
	}

	data[0] = 0;
	reader->SetDualModeBit(1);
	reader->SetChannelMask(0xFF00);
    SECTION("dual_dmmask_fec_even" ) {
		reader->readFecID(ptr);
		REQUIRE(reader->FecId() == 0);
		REQUIRE(reader->ChannelMask() == 0);
		REQUIRE(reader->DualModeMask() == 0x00FF);
		REQUIRE(ptr == ptr_orig + 1);
	}

	data[0] = 0xFFE0;
	reader->SetDualModeBit(1);
	reader->SetChannelMask(0xFF00);
    SECTION("dual_chmask_fec_odd" ) {
		reader->readFecID(ptr);
		REQUIRE(reader->FecId() == 0x07FF);
		REQUIRE(reader->ChannelMask() == 0x00FF);
		REQUIRE(reader->DualModeMask() == 0);
		REQUIRE(ptr == ptr_orig + 1);
	}
}

TEST_CASE( "Test Number of channels", "[nchannels]" ) {
	unsigned short data[1] = {
		0x001F, // FEC ID
	};

	int16_t * ptr = (int16_t*) data;
	int16_t * ptr_orig = ptr;

	spdlog::drop("eventreader");
	next::EventReader* reader = new next::EventReader(0);

    SECTION("All active" ) {
		reader->readFecID(ptr);
		REQUIRE(reader->NumberOfChannels() == 0x1F);
		REQUIRE(ptr == ptr_orig + 1);
	}

	//spdlogs raise an exception between sections for some reason
	spdlog::drop("eventreader");
	data[0] = 0xFFE0;

    SECTION("All inactive" ) {
		reader->readFecID(ptr);
		REQUIRE(reader->NumberOfChannels() == 0);
		REQUIRE(ptr == ptr_orig + 1);
	}
}


TEST_CASE( "Test baselines", "[baselines]" ) {
	const int nsensors = 8;
	const int nwords = 6;
	unsigned short data[nsensors][nwords] = {
		{0xFFF0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
		{0x000F, 0xFF00, 0x0000, 0x0000, 0x0000, 0x0000},
		{0x0000, 0x00FF, 0xF000, 0x0000, 0x0000, 0x0000},
		{0x0000, 0x0000, 0x0FFF, 0x0000, 0x0000, 0x0000},
		{0x0000, 0x0000, 0x0000, 0xFFF0, 0x0000, 0x0000},
		{0x0000, 0x0000, 0x0000, 0x000F, 0xFF00, 0x0000},
		{0x0000, 0x0000, 0x0000, 0x000F, 0x00FF, 0xF000},
		{0x0000, 0x0000, 0x0000, 0x000F, 0x0000, 0x0FFF}
	};

	int16_t * ptr[nsensors];
	int16_t * ptr_orig[nsensors];
	for(int i=0; i<nsensors; i++){
		ptr[i] = (int16_t *) data[i];
		ptr_orig[i] = (int16_t *) data[i];
	}

	spdlog::drop("eventreader");
	next::EventReader* reader = new next::EventReader(0);

    SECTION("All active") {
		for (int i=0; i<nsensors; i++){
			reader->readHotelBaselines(ptr[i]);
			REQUIRE(reader->Pedestal(i) == 0x0FFF);
			REQUIRE(ptr[i] == ptr_orig[i] + nwords);
		}
	}

	//spdlogs raise an exception between sections for some reason
	spdlog::drop("eventreader");
	for (int i=0; i<nsensors; i++){
		for (int j=0; j<nwords; j++){
			data[i][j] = ~data[i][j];
		}
	}

    SECTION("All inactive" ) {
		for (int i=0; i<nsensors; i++){
			reader->readHotelBaselines(ptr[i]);
			REQUIRE(reader->Pedestal(i) == 0);
			REQUIRE(ptr[i] == ptr_orig[i] + nwords);
		}
	}
}

TEST_CASE( "Test CT", "[ct]" ) {
	unsigned short data[3] = {
		0xFFFF, // CT H
		0xFFFF, // CT L
		0x03FF, // FTH & CTms
	};

	int16_t * ptr = (int16_t*) data;
	int16_t * ptr_orig = ptr;

	spdlog::drop("eventreader");
	next::EventReader* reader = new next::EventReader(0);

    SECTION("All active") {
		reader->readCTandFTh(ptr);
		REQUIRE(reader->TimeStamp() == 0x3FFFFFFFFFF);
		REQUIRE(ptr == ptr_orig + 3);
	}

	//spdlogs raise an exception between sections for some reason
	spdlog::drop("eventreader");
	data[0] = 0x0000;
	data[1] = 0x0000;
	data[2] = 0xFC00;

    SECTION("All inactive" ) {
		reader->readCTandFTh(ptr);
		REQUIRE(reader->TimeStamp() == 0);
		REQUIRE(ptr == ptr_orig + 3);
	}
}

TEST_CASE( "Test FT L", "[ft_l]" ) {
	unsigned short data[1] = {
		0xFFFF, // FT L
	};

	int16_t * ptr = (int16_t*) data;
	int16_t * ptr_orig = ptr;

	spdlog::drop("eventreader");
	next::EventReader* reader = new next::EventReader(0);

    SECTION("All active" ) {
		reader->readFTl(ptr);
		REQUIRE(reader->TriggerFT() == 0xFFFF);
		REQUIRE(ptr == ptr_orig + 1);
	}

	//spdlogs raise an exception between sections for some reason
	spdlog::drop("eventreader");
	data[0] = 0;

    SECTION("All inactive" ) {
		reader->readFTl(ptr);
		REQUIRE(reader->TriggerFT() == 0);
		REQUIRE(ptr == ptr_orig + 1);
	}
}
