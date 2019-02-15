#ifndef DATABASE_H
#define DATABASE_H
#include <map>

#define NSIPM 1792
#define NPMT 12

namespace next{
	class Sensors
	{
		public:
			Sensors();

			int elecToSensor(int elecID);
			int sensorToElec(int sensorID);
			int sipmIDtoPositionDB(int sensorID);

			int getNumberOfPmts();
			int getNumberOfSipms();

			void setNumberOfPmts (int npmts);
			void setNumberOfSipms(int nsipms);

			void update_relations(int elecID, int sensorID);

			// compute position of sensors by sensorid from DB
			void update_sipms_positions(int sensorID, int position);

		private:
			std::map<int, int> _elecToSensor;
			std::map<int, int> _sensorToElec;
			std::map<int, int> _sipmIDtoPosition;
			int _npmts;
			int _nsipms;
	};

}

int SipmIDtoPosition(int id);
int PositiontoSipmID(int pos);

#endif
