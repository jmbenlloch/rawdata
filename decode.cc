#include <iostream>
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

	if(!config.copyEvts()){
		next::HDF5Writer writer = next::HDF5Writer(&config);
		writer.Open(config.file_out());

		next::RawDataInput rdata = next::RawDataInput(&config, &writer);
		rdata.readFile(config.file_in());
		//rdata.readNext();
		bool hasNext = true;
		while (hasNext){
			hasNext = rdata.readNext();
		}

		//CLose open files with rawdatainput
		writer.WriteRunInfo();
		writer.Close();
	}else{
		next::CopyEvents copyEvts = next::CopyEvents(&config);
		copyEvts.readFile(config.file_in(), config.file_out());
		bool hasNext = true;
		while (hasNext){
			hasNext = copyEvts.readNext();
		}

	}

	return 0;
}

