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

			int getNumberOfPmts();
			int getNumberOfSipms();

			void update_relations(int elecID, int sensorID);

		private:
			std::map<int, int> _elecToSensor;
			std::map<int, int> _sensorToElec;
	};

}

int SipmIDtoPosition(int id);
int PositiontoSipmID(int pos);

#endif
