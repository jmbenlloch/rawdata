#include "writer/PetaloWriter.h"
#include <sstream>
#include <cstring>
#include <stdlib.h>

#include<stdint.h>

namespace spd = spdlog;

next::PetaloWriter::PetaloWriter(ReadConfig * config) :
    _config(config)
{

	_log = spd::stdout_color_mt("writer");
	if(config->verbosity() > 0){
		_log->set_level(spd::level::debug);
	}

	_ievt = 0;
}

next::PetaloWriter::~PetaloWriter(){
}

void next::PetaloWriter::Open(std::string fileName){
	_log->debug("Opening output file {}", fileName);

	_file = H5Fcreate( fileName.c_str(), H5F_ACC_TRUNC,
			H5P_DEFAULT, H5P_DEFAULT );

	_isOpen=true;


	//Create triggerType table
	hsize_t memtype = createPetaloType();
	std::string table_name = std::string("data");
	_dataTable = createTable(_file, table_name, memtype);

	petalo_t data;
	data.evt_number  = 0xFFFFFFFF;
	data.tofpet_id   = 7;
	data.wordtype_id = 3;
	data.channel_id  = 0x3f;
	data.tac_id      = 3;
	data.tcoarse     = 0xffff;
	data.ecoarse     = 0x3FF;
	data.tfine       = 0x3FF;
	data.efine       = 0x3FF;
	writePetaloType(&data, _dataTable, memtype, _ievt);

	std::vector<petalo_t> tofpetData;
	data.evt_number  = 0x0;
	_ievt += 1;
	tofpetData.push_back(data);
	data.evt_number  = 0x1;
	_ievt += 1;
	tofpetData.push_back(data);
	data.evt_number  = 0x2;
	_ievt += 1;
	tofpetData.push_back(data);
	data.evt_number  = 0x3;
	_ievt += 1;
	tofpetData.push_back(data);

	Write(tofpetData);


	// int evt_number;
	// unsigned char tofpet_id;
	// unsigned char wordtype_id;
	// unsigned char channel_id;
	// unsigned char tac_id;
	// unsigned short int tcoarse;
	// unsigned short int ecoarse;
	// unsigned short int tfine;
	// unsigned short int efine;
    //

}

void next::PetaloWriter::Close(){
  _isOpen=false;

  _log->debug("Closing output file");
  H5Fclose(_file);
}

void next::PetaloWriter::Write(std::vector<petalo_t>& tofpetData){
	hsize_t memtype = createPetaloType();


	for(int i=0; i<tofpetData.size(); i++){
		unsigned int evt_id = tofpetData[i].evt_number;
		_log->debug("Writing event {} to HDF5 file", evt_id);
		writePetaloType(&(tofpetData[i]), _dataTable, memtype, evt_id);
	}
}
