#include "writer/hdf5_functions.h"

hid_t createTriggerConfType(){
  hid_t strtype = H5Tcopy(H5T_C_S1);
  H5Tset_size (strtype, STRLEN);

  //Create compound datatype for the table
  hsize_t memtype = H5Tcreate (H5T_COMPOUND, sizeof (triggerConf_t));
  H5Tinsert (memtype, "param" , HOFFSET (triggerConf_t, param), strtype);
  H5Tinsert (memtype, "value" , HOFFSET (triggerConf_t, value), H5T_NATIVE_INT);
  return memtype;
}

hid_t createEventType(){
	//Create compound datatype for the table
	hsize_t memtype = H5Tcreate (H5T_COMPOUND, sizeof (evt_t));
	H5Tinsert (memtype, "evt_number", HOFFSET (evt_t, evt_number), H5T_NATIVE_INT);
	H5Tinsert (memtype, "timestamp" , HOFFSET (evt_t, timestamp) , H5T_NATIVE_UINT64);
	return memtype;
}

hid_t createRunType(){
	hsize_t memtype = H5Tcreate (H5T_COMPOUND, sizeof (runinfo_t));
	H5Tinsert (memtype, "run_number", HOFFSET (runinfo_t, run_number), H5T_NATIVE_INT);
	return memtype;
}

hid_t createTriggerType(){
	hsize_t memtype = H5Tcreate (H5T_COMPOUND, sizeof (trigger_t));
	H5Tinsert (memtype, "trigger_type", HOFFSET (trigger_t, trigger_type), H5T_NATIVE_INT);
	return memtype;
}

hid_t createSensorType(){
	hsize_t memtype = H5Tcreate (H5T_COMPOUND, sizeof (sensor_t));
	H5Tinsert (memtype, "channel",  HOFFSET(sensor_t, channel) , H5T_NATIVE_INT);
	H5Tinsert (memtype, "sensorID", HOFFSET(sensor_t, sensorID), H5T_NATIVE_INT);
	return memtype;
}

hid_t createTable(hid_t group, std::string& table_name, hsize_t memtype){
	//Create 1D dataspace (evt number). First dimension is unlimited (initially 0)
	const hsize_t ndims = 1;
	hsize_t dims[ndims] = {0};
	hsize_t max_dims[ndims] = {H5S_UNLIMITED};
	hsize_t file_space = H5Screate_simple(ndims, dims, max_dims);

	// Create a dataset creation property list
	// The layout of the dataset have to be chunked when using unlimited dimensions
	hid_t plist = H5Pcreate(H5P_DATASET_CREATE);
	H5Pset_layout(plist, H5D_CHUNKED);
	hsize_t chunk_dims[ndims] = {32768};
	H5Pset_chunk(plist, ndims, chunk_dims);

	// Create dataset
	hid_t dataset = H5Dcreate(group, table_name.c_str(), memtype, file_space,
			H5P_DEFAULT, plist, H5P_DEFAULT);
	return dataset;
}

hid_t createGroup(hid_t file, std::string& groupName){
	//Create group
	hid_t wfgroup;
	wfgroup = H5Gcreate2(file, groupName.c_str(), H5P_DEFAULT, H5P_DEFAULT,
			H5P_DEFAULT);
	return wfgroup;
}


hid_t createWaveforms(hid_t group, std::string& dset_name, hsize_t nsensors,
		hsize_t nsamples){
	const hsize_t ndims = 3;
	hid_t file_space;

	//Create 3D dataspace (evt,sensor,data).
	//First dimension is unlimited (initially 0)
	hsize_t dims[ndims] = {0, nsensors, nsamples};
	hsize_t max_dims[ndims] = {H5S_UNLIMITED, nsensors, nsamples};
	file_space = H5Screate_simple(ndims, dims, max_dims);

	// Create a dataset creation property list
	// The layout of the dataset have to be chunked when using unlimited dimensions
	hid_t plist = H5Pcreate(H5P_DATASET_CREATE);
	H5Pset_layout(plist, H5D_CHUNKED);
	hsize_t chunk_dims[ndims] = {1, nsensors, nsamples};
	//if sipms
	if(nsensors > 50){
		chunk_dims[1] = 50;
	}else{ //pmts
		chunk_dims[1] = 1;
		if(nsamples > 32768){
			chunk_dims[2] = 32768;
		}
	}

	H5Pset_chunk(plist, ndims, chunk_dims);

	//Set compression
	H5Pset_deflate (plist, 4);

	//Create dataset
	hid_t dataset = H5Dcreate(group, dset_name.c_str(), H5T_NATIVE_SHORT,
			file_space, H5P_DEFAULT, plist, H5P_DEFAULT);
	return dataset;

}

//Only one waveform for the external PMT (event, waveform)
hid_t createWaveform(hid_t group, std::string& dset_name, hsize_t nsamples){
	const hsize_t ndims = 2;
	hid_t file_space;

	//Create 3D dataspace (evt,sensor,data).
	//First dimension is unlimited (initially 0)
	hsize_t dims[ndims] = {0, nsamples};
	hsize_t max_dims[ndims] = {H5S_UNLIMITED, nsamples};
	file_space = H5Screate_simple(ndims, dims, max_dims);

	// Create a dataset creation property list
	// The layout of the dataset have to be chunked when using unlimited dimensions
	hid_t plist = H5Pcreate(H5P_DATASET_CREATE);
	H5Pset_layout(plist, H5D_CHUNKED);
	hsize_t chunk_dims[ndims] = {1, 32768};
	if(nsamples < 32768){
		chunk_dims[1] = nsamples;
	}

	H5Pset_chunk(plist, ndims, chunk_dims);

	//Set compression
	H5Pset_deflate (plist, 4);

	//Create dataset
	hid_t dataset = H5Dcreate(group, dset_name.c_str(), H5T_NATIVE_SHORT,
			file_space, H5P_DEFAULT, plist, H5P_DEFAULT);
	return dataset;

}


hid_t createBaselines(hid_t group, std::string& dset_name, hsize_t nsamples){
	const hsize_t ndims = 2;
	hid_t file_space;

	//Create 3D dataspace (evt,sensor,data).
	//First dimension is unlimited (initially 0)
	hsize_t dims[ndims] = {0, nsamples};
	hsize_t max_dims[ndims] = {H5S_UNLIMITED, nsamples};
	file_space = H5Screate_simple(ndims, dims, max_dims);

	// Create a dataset creation property list
	// The layout of the dataset have to be chunked when using unlimited dimensions
	hid_t plist = H5Pcreate(H5P_DATASET_CREATE);
	H5Pset_layout(plist, H5D_CHUNKED);
	hsize_t chunk_dims[ndims] = {1, 32768};
	if(nsamples < 32768){
		chunk_dims[1] = nsamples;
	}

	H5Pset_chunk(plist, ndims, chunk_dims);

	//Set compression
	H5Pset_deflate (plist, 4);

	//Create dataset
	hid_t dataset = H5Dcreate(group, dset_name.c_str(), H5T_NATIVE_INT,
			file_space, H5P_DEFAULT, plist, H5P_DEFAULT);
	return dataset;

}


void WriteWaveforms(short int * data, hid_t dataset, hsize_t nsensors,
	   	hsize_t nsamples, hsize_t evt){
	hid_t memspace, file_space;
	//Create memspace for one PMT row
	const hsize_t ndims = 3;
	hsize_t dims[ndims] = {1, nsensors, nsamples};
	memspace = H5Screate_simple(ndims, dims, NULL);

	//Extend PMT dataset
	dims[0] = evt+1;
	dims[1] = nsensors;
	dims[2] = nsamples;
	H5Dset_extent(dataset, dims);

	//Write PMT waveforms
	file_space = H5Dget_space(dataset);
	hsize_t start[3] = {evt, 0, 0};
	hsize_t count[3] = {1, nsensors, nsamples};
	H5Sselect_hyperslab(file_space, H5S_SELECT_SET, start, NULL, count, NULL);
	H5Dwrite(dataset, H5T_NATIVE_SHORT, memspace, file_space, H5P_DEFAULT, data);
	H5Sclose(file_space);
	H5Sclose(memspace);
}

void WriteWaveform(short int * data, hid_t dataset, hsize_t nsamples, hsize_t evt){
	hid_t memspace, file_space;
	//Create memspace for one PMT row
	const hsize_t ndims = 2;
	hsize_t dims[ndims] = {1, nsamples};
	memspace = H5Screate_simple(ndims, dims, NULL);

	//Extend PMT dataset
	dims[0] = evt+1;
	dims[1] = nsamples;
	H5Dset_extent(dataset, dims);

	//Write PMT waveforms
	file_space = H5Dget_space(dataset);
	hsize_t start[2] = {evt, 0};
	hsize_t count[2] = {1, nsamples};
	H5Sselect_hyperslab(file_space, H5S_SELECT_SET, start, NULL, count, NULL);
	H5Dwrite(dataset, H5T_NATIVE_SHORT, memspace, file_space, H5P_DEFAULT, data);
	H5Sclose(file_space);
	H5Sclose(memspace);
}

void WriteBaselines(int * data, hid_t dataset, hsize_t nsamples, hsize_t evt){
	hid_t memspace, file_space;
	//Create memspace for one PMT row
	const hsize_t ndims = 2;
	hsize_t dims[ndims] = {1, nsamples};
	memspace = H5Screate_simple(ndims, dims, NULL);

	//Extend PMT dataset
	dims[0] = evt+1;
	dims[1] = nsamples;
	H5Dset_extent(dataset, dims);

	//Write PMT waveforms
	file_space = H5Dget_space(dataset);
	hsize_t start[2] = {evt, 0};
	hsize_t count[2] = {1, nsamples};
	H5Sselect_hyperslab(file_space, H5S_SELECT_SET, start, NULL, count, NULL);
	H5Dwrite(dataset, H5T_NATIVE_INT, memspace, file_space, H5P_DEFAULT, data);
	H5Sclose(file_space);
	H5Sclose(memspace);
}




void writeEvent(evt_t * evtData, hid_t dataset, hid_t memtype, hsize_t evt_number){
	hid_t memspace, file_space;
	hsize_t dims[1] = {1};
	memspace = H5Screate_simple(1, dims, NULL);

	//Extend PMT dataset
	dims[0] = evt_number+1;
	H5Dset_extent(dataset, dims);

	//Write PMT waveforms
	file_space = H5Dget_space(dataset);
	hsize_t start[1] = {evt_number};
	hsize_t count[1] = {1};
	H5Sselect_hyperslab(file_space, H5S_SELECT_SET, start, NULL, count, NULL);
	H5Dwrite(dataset, memtype, memspace, file_space, H5P_DEFAULT, evtData);
	H5Sclose(file_space);
	H5Sclose(memspace);
}


void writeRun(runinfo_t * runData, hid_t dataset, hid_t memtype, hsize_t evt_number){
	hid_t memspace, file_space;
	hsize_t dims[1] = {1};
	memspace = H5Screate_simple(1, dims, NULL);

	//Extend PMT dataset
	dims[0] = evt_number+1;
	H5Dset_extent(dataset, dims);

	file_space = H5Dget_space(dataset);
	hsize_t start[1] = {evt_number};
	hsize_t count[1] = {1};
	H5Sselect_hyperslab(file_space, H5S_SELECT_SET, start, NULL, count, NULL);
	H5Dwrite(dataset, memtype, memspace, file_space, H5P_DEFAULT, runData);
	H5Sclose(file_space);
	H5Sclose(memspace);
}

void writeTriggerType(trigger_t * trigger, hid_t dataset, hid_t memtype, hsize_t evt_number){
	hid_t memspace, file_space;
	hsize_t dims[1] = {1};
	memspace = H5Screate_simple(1, dims, NULL);

	//Extend PMT dataset
	dims[0] = evt_number+1;
	H5Dset_extent(dataset, dims);

	file_space = H5Dget_space(dataset);
	hsize_t start[1] = {evt_number};
	hsize_t count[1] = {1};
	H5Sselect_hyperslab(file_space, H5S_SELECT_SET, start, NULL, count, NULL);
	H5Dwrite(dataset, memtype, memspace, file_space, H5P_DEFAULT, trigger);
	H5Sclose(file_space);
	H5Sclose(memspace);
}

void writeTriggerConf(triggerConf_t * triggerData, hid_t dataset, hid_t memtype, hsize_t index){
	hid_t memspace, file_space;
	hsize_t dims[1] = {1};
	memspace = H5Screate_simple(1, dims, NULL);

	//Extend PMT dataset
	dims[0] = index+1;
	H5Dset_extent(dataset, dims);

	file_space = H5Dget_space(dataset);
	hsize_t start[1] = {index};
	hsize_t count[1] = {1};
	H5Sselect_hyperslab(file_space, H5S_SELECT_SET, start, NULL, count, NULL);
	H5Dwrite(dataset, memtype, memspace, file_space, H5P_DEFAULT, triggerData);
	H5Sclose(file_space);
	H5Sclose(memspace);
}

void writeSensor(sensor_t * sensorData, hid_t dataset, hid_t memtype, hsize_t sensor_number){
	hid_t memspace, file_space;
	hsize_t dims[1] = {1};
	memspace = H5Screate_simple(1, dims, NULL);

	//Extend PMT dataset
	dims[0] = sensor_number+1;
	H5Dset_extent(dataset, dims);

	file_space = H5Dget_space(dataset);
	hsize_t start[1] = {sensor_number};
	hsize_t count[1] = {1};
	H5Sselect_hyperslab(file_space, H5S_SELECT_SET, start, NULL, count, NULL);
	H5Dwrite(dataset, memtype, memspace, file_space, H5P_DEFAULT, sensorData);
	H5Sclose(file_space);
	H5Sclose(memspace);
}
