#ifndef _HDF5WRITER
#define _HDF5WRITER
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

  class HDF5Writer {

  private:

    //! HDF5 file
    size_t _file[2];

	//! Second file
    bool _splitTrg;
	std::map<int, int> _triggerCodeToFile;

	bool _isOpen;

	//! First event
	bool _firstEvent[2];

	bool _nodb;
	std::vector<int> _pmt_elecids;
	std::vector<int> _blr_elecids;
	std::vector<int> _sipm_elecids;

	bool _hasPmts;
	bool _hasBlrs;
	bool _hasSipms;

	//Datasets
	size_t _pmtrd[2];
	size_t _triggerd[2];
	size_t _extpmtrd[2];
	size_t _eventsTable[2];
	size_t _triggerTable[2];
	size_t _pmtblr[2];
	size_t _sipmrd[2];
	size_t _memtypeEvt;
	size_t _pmtBaseline[2];
	size_t _blrBaseline[2];

    //! counter for writen events
    size_t _ievt[2];

	ReadConfig * _config;

	next::Sensors _sensors;

	std::shared_ptr<spdlog::logger> _log;

  public:

    //! constructor
    HDF5Writer(ReadConfig * config);

    //! destructor
    virtual ~HDF5Writer();

    //! write event
    void Write(DigitCollection & pmts, DigitCollection& blrs, DigitCollection& extPmt, DigitCollection& sipms, std::vector<std::pair<std::string, int> > triggerInfo, std::vector<int> triggerChans, int triggerType, std::uint64_t timestamp, unsigned int evt_number, size_t run_number);

	void StorePmtWaveforms(std::vector<next::Digit*> sensors, hsize_t nsensors, hsize_t datasize, hsize_t dataset, int dset_idx);
	void StorePmtBaselines(std::vector<next::Digit*> sensors, hsize_t nsensors, hsize_t dataset, int dset_idx);
	void StoreSipmWaveforms(std::vector<next::Digit*> sensors, hsize_t nsensors, hsize_t datasize, hsize_t dataset, int dset_idx);
	void StoreTriggerChannels(std::vector<next::Digit*> sensors, std::vector<int> triggers, hsize_t nsensors, hsize_t datasize, hsize_t dataset, int dset_idx);

	void sortPmts(std::vector<next::Digit*> &sorted_sensors, DigitCollection &sensors);
	void sortPmtsNoDB(std::vector<next::Digit*> &sorted_sensors, DigitCollection &sensors);
	void sortSipmsNoDB(std::vector<next::Digit*> &sorted_sensors, std::vector<next::Digit*> &sensors);
	void sortSipms(std::vector<next::Digit*> &sorted_sensors, DigitCollection &sensors);
	void save_elecids(std::vector<int>* elecids, std::vector<next::Digit*> &sorted_sensors);
	void select_active_sensors(std::vector<next::Digit*> * active_sensors, DigitCollection& sensors);
	void saveTriggerInfo(std::vector<std::pair<std::string, int> > triggerInfo, hid_t trigger_group);
	void saveTriggerType(hid_t table, int triggerType, int dset_idx);

	hid_t CreateRunInfoGroup(hsize_t file, size_t run_number);

    //! open file
    void Open(std::string filename, std::string filename2);

    //! close file
    void Close();

    //! write dst info into root file
    void WriteRunInfo();
    void WriteRunInfo(size_t file);
  };
}
