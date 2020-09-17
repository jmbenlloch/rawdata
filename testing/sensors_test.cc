#include "catch.hpp"
#include "database/sensors.h"

TEST_CASE("Test Sipm position and sensorID", "[sipm_id2pos]") {
	int sensor_id=999;
	for (unsigned int pos=0; pos<NSIPM; pos++){
		if (pos && pos % 64 == 0){
			sensor_id = sensor_id - 63 + 1000;
		}else{
			sensor_id++;
		}
		REQUIRE(PositiontoSipmID(pos) == sensor_id);
		REQUIRE(SipmIDtoPosition(sensor_id) == pos);
	}
}

TEST_CASE("Test Sensors mapping", "[sipm_id2pos]") {
	next::Sensors sensors;

    SECTION("Test no PMTs") {
		int nchannels = 32;
		for(int i=0; i<nchannels; i++){
			REQUIRE(sensors.elecToSensor(i) == -1);
			REQUIRE(sensors.sensorToElec(i) == -1);
		}
	}

    SECTION("Test no SiPMs") {
		int id;
		for(int i=0; i<NSIPM; i++){
			id = PositiontoSipmID(i);
			REQUIRE(sensors.elecToSensor(id) == -1);
			REQUIRE(sensors.sensorToElec(id) == -1);
		}
	}

    // SECTION("Test total number of PMTs") {
	//     REQUIRE(sensors.getNumberOfPmts() == NPMT);
	// }
    //
    // SECTION("Test total number of SiPMT") {
	//     REQUIRE(sensors.getNumberOfSipms() == NSIPM);
	// }

	//Add pmt mapping
    SECTION("Test PMT mapping") {
		int id;
		for(int i=0; i<NPMT; i++){
			id = PositiontoSipmID(i);
			sensors.update_relations(id, id-1000);
			REQUIRE(sensors.elecToSensor(id) == id-1000);
			REQUIRE(sensors.sensorToElec(id-1000) == id);
		}
	}

	//Add sipm mapping
    SECTION("Test SIPM mapping") {
		for(int i=0; i<NSIPM; i++){
			sensors.update_relations(i, NPMT+i);
			REQUIRE(sensors.elecToSensor(i) == NPMT+i);
			REQUIRE(sensors.sensorToElec(NPMT+i) == i);
		}
	}
}
