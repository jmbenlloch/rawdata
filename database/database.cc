#include <iostream>
#include "database/database.h"

namespace spd = spdlog;

void finish_with_error(MYSQL *con, std::shared_ptr<spdlog::logger> log)
{
  log->error("{}", mysql_error(con));
  mysql_close(con);
  exit(1);
}

void getHuffmanFromDB(ReadConfig * config, Huffman * huffman, int run_number){
	MYSQL *con = mysql_init(NULL);
	std::shared_ptr<spdlog::logger> log = spd::stdout_color_mt("db");
	std::shared_ptr<spdlog::logger> logerr = spd::stderr_color_mt("huffman");

	if (con == NULL){
		logerr->error("mysql_init() failed");
		exit(1);
	}

	if (mysql_real_connect(con, config->host().c_str(), config->user().c_str(),
				config->pass().c_str(), config->dbname().c_str(), 0, NULL, 0) == NULL){
		finish_with_error(con, logerr);
	}

	std::string sql = "SELECT value, code from HuffmanCodes WHERE MinRun <= RUN and MaxRun >= RUN";

	size_t start_pos = sql.find("RUN");
	while(start_pos != std::string::npos){
		sql.replace(start_pos, 3, std::to_string(run_number));
		start_pos = sql.find("RUN");
	}

	if (mysql_query(con, sql.c_str())){
		finish_with_error(con, logerr);
	}

	MYSQL_RES *result = mysql_store_result(con);
	if (result == NULL){
		finish_with_error(con, logerr);
	}

	log->info("Huffman codes read from {} in {}", config->dbname(),
			config->host());

	MYSQL_ROW row;

	while ((row = mysql_fetch_row(result))){
		parse_huffman_line(std::stoi(row[0]), row[1], huffman);
	}

	mysql_free_result(result);
	mysql_close(con);

}

void getSensorsFromDB(ReadConfig * config, next::Sensors &sensors, int run_number, bool masked){
	MYSQL *con = mysql_init(NULL);
	std::shared_ptr<spdlog::logger> log = spd::stdout_color_mt("DB");
	std::shared_ptr<spdlog::logger> logerr = spd::stderr_color_mt("mysql");

	if (con == NULL){
		logerr->error("mysql_init() failed");
		exit(1);
	}

	if (mysql_real_connect(con, config->host().c_str(), config->user().c_str(),
				config->pass().c_str(), config->dbname().c_str(), 0, NULL, 0) == NULL){
		finish_with_error(con, logerr);
	}

	//Add run number
	std::string sql = "SELECT ElecID, SensorID from ChannelMapping WHERE MinRun <= RUN and MaxRun >= RUN and SensorID NOT IN (SELECT SensorID FROM ChannelMask WHERE MinRun <= RUN and MaxRun >= RUN) ORDER BY SensorID";
	if(masked){
		sql = "SELECT ElecID, SensorID FROM ChannelMapping WHERE MinRun <= RUN and MaxRun >= RUN ORDER BY SensorID";
	}

	size_t start_pos = sql.find("RUN");
	while(start_pos != std::string::npos){
		sql.replace(start_pos, 3, std::to_string(run_number));
		start_pos = sql.find("RUN");
	}

	if (mysql_query(con, sql.c_str())){
		finish_with_error(con, logerr);
	}

	MYSQL_RES *result = mysql_store_result(con);
	if (result == NULL){
		finish_with_error(con, logerr);
	}

	log->info("Sensors mapping read from {} in {}", config->dbname(),
			config->host());

	MYSQL_ROW row;

	std::vector<int> sipms_sensor_ids;

	int elecid, sensorid;
	//This id separates between pmts and sipms elecIDs
	// Sipms starts at 1000, external pmt is 99
	int threshold = 90;
	int npmts  = 0;
	int nsipms = 0;
	while ((row = mysql_fetch_row(result))){
		elecid   = std::stoi(row[0]);
		sensorid = std::stoi(row[1]);
		sensors.update_relations(elecid, sensorid);

		if(elecid < threshold){
			npmts += 1;
		}else{
			nsipms += 1;
			sipms_sensor_ids.push_back(sensorid);
		}
	}
	sensors.setNumberOfPmts (npmts);
	sensors.setNumberOfSipms(nsipms);

	// sort Sipm ids and get positions in memory
	std::sort(sipms_sensor_ids.begin(), sipms_sensor_ids.end());
	for(int i=0; i<sipms_sensor_ids.size(); i++){
		sensors.update_sipms_positions(sipms_sensor_ids[i], i);
	}

	mysql_free_result(result);
	mysql_close(con);
}
