#include "database/sensors.h"
#include <iostream>


next::Sensors::Sensors(){
}

//Return -1 if not found
int next::Sensors::elecToSensor(int elecID){
	if (_elecToSensor.find(elecID) == _elecToSensor.end()){
		return -1;
	}
	return _elecToSensor[elecID];
}

//Return -1 if not found
int next::Sensors::sensorToElec(int sensorID){
	if (_sensorToElec.find(sensorID) == _sensorToElec.end()){
		return -1;
	}
	return _sensorToElec[sensorID];
}

int next::Sensors::sipmIDtoPositionDB(int sensorID){
	if (_sipmIDtoPosition.find(sensorID) == _sipmIDtoPosition.end()){
		return -1;
	}
	return _sipmIDtoPosition[sensorID];
}

void next::Sensors::update_relations(int elecID, int sensorID){
	_elecToSensor[elecID]   = sensorID;
	_sensorToElec[sensorID] = elecID;

	if(sensorID < 999){
		_sortedPmtIDs.push_back(sensorID);
	}else{
		_sortedSipmIDs.push_back(sensorID);
	}
}

void next::Sensors::update_sipms_positions(int sensorID, int position){
	_sipmIDtoPosition[sensorID] = position;
}


int next::Sensors::getNumberOfPmts(){
	return _npmts;
}

int next::Sensors::getNumberOfSipms(){
	return _nsipms;
}

std::vector<int> next::Sensors::getSortedPmtsDB(){
	return _sortedPmtIDs;
}

std::vector<int> next::Sensors::getSortedSipmsDB(){
	return _sortedSipmIDs;
}

void next::Sensors::setNumberOfPmts(int npmts){
	_npmts = npmts;
}

void next::Sensors::setNumberOfSipms(int nsipms){
	_nsipms = nsipms;
}

int SipmIDtoPosition(int id){
	int position = ((id/1000)-1)*64 + id%1000;
	return position;
}

int PositiontoSipmID(int pos){
	int sipmid = (pos/64+1)*1000 + pos%64;
	return sipmid;
}

int PmtIDtoPosition(int id){
	int position = ((id/100)-1)*24 + id % 100;
	return position;
}

int PositiontoPmtID(int pos){
	int sipmid = (pos/24+1) * 100 + pos % 24;
	return sipmid;
}
