#ifndef _PETALOWRITER
#define _PETALOWRITER
#endif

#include <vector>
#include <iostream>
//#include <hdf5/serial/hdf5.h>
#include <hdf5.h>

#include "navel/Digit.hh"
#include "writer/hdf5_functions.h"

#include "database/database.h"
#include "database/sensors.h"

#ifndef _READCONFIG
#include "config/ReadConfig.h"
#endif

#ifndef SPDLOG_VERSION
#include "spdlog/spdlog.h"
#endif

#define MAX_PMTs 24
#define MAX_SIPMs 1792

namespace next{

  class PetaloWriter {

  private:

    //! HDF5 file
    size_t _file;

	bool _isOpen;

	//! First event
	bool _firstEvent;

	//Datasets
	size_t _dataTable;

    //! counter for writen events
    size_t _ievt;
    size_t _row;

	ReadConfig * _config;

	std::shared_ptr<spdlog::logger> _log;

  public:

    //! constructor
    PetaloWriter(ReadConfig * config);

    //! destructor
    virtual ~PetaloWriter();

    //! write event
    void Write(std::vector<petalo_t>&);

    //! open file
    void Open(std::string filename);

    //! close file
    void Close();
  };
}
