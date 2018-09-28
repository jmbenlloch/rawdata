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

void next::Sensors::update_relations(int elecID, int sensorID){
	_elecToSensor[elecID]   = sensorID;
	_sensorToElec[sensorID] = elecID;
}

int next::Sensors::getNumberOfPmts(){
	return _npmts;
}

int next::Sensors::getNumberOfSipms(){
	return _nsipms;
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
