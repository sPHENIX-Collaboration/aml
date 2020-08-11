#ifndef __ET_PGCONNECTION_H__
#define __ET_PGCONNECTION_H__


#include <odbc++/connection.h>


class et_PGConnection {


 public:

  static et_PGConnection *instance(const char * db =0, const char *host=0, const char *table="prdflist" );

  odbc::Connection * getConnection() { return theconnection;};


  int getLogging() const { return  LogEnabled;};
  void setLogging( const int i) 
    { 
      if (i) LogEnabled = 1; 
      else LogEnabled = 0;
    }

  int addRecord ( const std::string name, const int runnumber, const int sequence);
  int updateSize_Events ( const std::string name, const unsigned long long size, const int events);
  int updateMD5sum ( const std::string name, const char *md5);


 protected:
  

  et_PGConnection ( const char * db, const char *host, const char *table );
  static  et_PGConnection *single; 

  odbc::Connection *theconnection;

  int LogEnabled;
  std::string TheTable;

};


#endif /* __ET_PGCONNECTION_H__ */
