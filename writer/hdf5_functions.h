#ifndef _HDF5FUNCTIONS
#define _HDF5FUNCTIONS
#endif

//#include <hdf5/serial/hdf5.h>
#include <hdf5.h>
#include <iostream>

#define STRLEN 20

typedef struct{
	int channel;
	int sensorID;
} sensor_t;

typedef struct{
	int run_number;
} runinfo_t;

typedef struct{
	int trigger_type;
} trigger_t;

typedef struct{
	int evt_number;
	uint64_t timestamp;
} evt_t;

typedef struct{
	char param[STRLEN];
	int value;
} triggerConf_t;


hid_t createGroup(hid_t file, std::string& group);

void WriteWaveforms(short int * data, hid_t dataset, hsize_t nsensors, hsize_t nsamples, hsize_t evt);
hid_t createWaveforms(hid_t group, std::string& dataset, hsize_t nsensors, hsize_t nsamples);

void WriteWaveform(short int * data, hid_t dataset, hsize_t nsamples, hsize_t evt);
hid_t createWaveform(hid_t group, std::string& dataset, hsize_t nsamples);

hid_t createTable(hid_t group, std::string& table_name, hsize_t memtype);
hid_t createEventType();
hid_t createRunType();
hid_t createTriggerConfType();
hid_t createSensorType();
hid_t createTriggerType();

void writeEvent(evt_t * evtData, hid_t dataset, hid_t memtype, hsize_t evt_number);
void writeRun(runinfo_t * runData, hid_t dataset, hid_t memtype, hsize_t evt_number);
void writeSensor(sensor_t * sensorData, hid_t dataset, hid_t memtype, hsize_t sensor_number);
void writeTriggerType(trigger_t * trigger, hid_t dataset, hid_t memtype, hsize_t evt_number);
void writeTriggerConf(triggerConf_t * triggerData, hid_t dataset, hid_t memtype, hsize_t index);
