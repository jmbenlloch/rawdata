#ifndef _READCONFIG
#define _READCONFIG
#endif

#ifndef SPDLOG_VERSION
#include "spdlog/spdlog.h"
#endif

#include <jsoncpp/json/json.h> // or jsoncpp/json.h , or json/json.h etc.

class ReadConfig {
	public:
		ReadConfig(std::string& filename);
		~ReadConfig();

		std::string config();
		std::string file_in();
		std::string file_out();
		std::string file_out2();
		int max_events();
		bool two_files();
		bool no_db();
		void parse();
		int verbosity();
		int extTrigger();
		int skip();
		bool discard();
		bool copyEvts();
		bool splitTrg();
		int  trgCode1();
		int  trgCode2();
		std::string host();
		std::string user();
		std::string pass();
		std::string dbname();


	private:
		Json::Reader _reader;
		Json::Value _obj;
		std::string _filename;
		std::string _filein;
		std::string _fileout;
		std::string _fileout2;
		int _maxevents;
		bool _twofiles;
		bool _nodb;
		bool _discard;
		int _verbosity;
		int _skip;
		int _extTrigger;
		bool _copyEvts;
		bool _splitTrg;
		int _trgCode1;
		int _trgCode2;
		std::string _host;
		std::string _user;
		std::string _passwd;
		std::string _dbname;
		std::shared_ptr<spdlog::logger> _log;
};

inline std::string ReadConfig::config(){return _filename;}

inline std::string ReadConfig::file_in(){ return _filein;}

inline std::string ReadConfig::file_out(){return _fileout;}

inline std::string ReadConfig::file_out2(){return _fileout2;}

inline int ReadConfig::max_events(){return _maxevents;}

inline bool ReadConfig::two_files(){return _twofiles;}

inline bool ReadConfig::no_db(){return _nodb;}

inline bool ReadConfig::discard(){return _discard;}

inline bool ReadConfig::splitTrg(){return _splitTrg;}

inline int ReadConfig::skip(){return _skip;}

inline int ReadConfig::trgCode1(){return _trgCode1;}

inline int ReadConfig::trgCode2(){return _trgCode2;}

inline int ReadConfig::verbosity(){return _verbosity;}

inline int ReadConfig::extTrigger(){return _extTrigger;}

inline std::string ReadConfig::host(){return _host;}

inline std::string ReadConfig::user(){return _user;}

inline std::string ReadConfig::pass(){return _passwd;}

inline std::string ReadConfig::dbname(){return _dbname;}

inline bool ReadConfig::copyEvts(){return _copyEvts;}
