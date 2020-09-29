#include <iostream>
#include "config/ReadConfig.h"
//#include "writer/CopyEvents.h"
#include "RawDataPetalo.h"

#ifndef _PETALOWRITER
#include "writer/PetaloWriter.h"
#endif

#ifndef SPDLOG_VERSION
#include "spdlog/spdlog.h"
#endif

namespace spd = spdlog;

int main(int argc, char* argv[]){
	auto console = spd::stdout_color_mt("console");
	console->info("RawDataPetalo started");

	if (argc < 2){
        console->error("Missing argument: <configfile>");
		std::cout << "Usage: rawdatareader <configfile>" << std::endl;
		return 1;
	}

	std::string filename = std::string(argv[1]);
	ReadConfig config = ReadConfig(filename);

	next::PetaloWriter writer = next::PetaloWriter(&config);
	writer.Open(config.file_out());

	next::RawDataPetalo rdata = next::RawDataPetalo(&config, &writer);
	rdata.readFile(config.file_in());
	rdata.readNext();
	bool hasNext = true;
	while (hasNext){
	     hasNext = rdata.readNext();
	}

	//CLose open files with rawdatainput
	writer.Close();

	if(!rdata.errors()){
		console->info("RawDataPetalo finished");
	}else{
		console->info("RawDataPetalo encountered errors");
	}

	return 0;
}

