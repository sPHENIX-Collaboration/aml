#include "et_PGConnection.h"


#include <odbc++/setup.h>
#include <odbc++/types.h>
#include <odbc++/errorhandler.h>
#include <sql.h>
#include <odbc++/drivermanager.h>
#include <odbc++/resultset.h>
#include <odbc++/resultsetmetadata.h>
#include <odbc++/preparedstatement.h>
#include <iostream>
#include <odbc++/databasemetadata.h>

#include <cstdlib>
#include <sstream>


et_PGConnection::et_PGConnection *et_PGConnection::single = 0;


et_PGConnection * et_PGConnection::instance(const char * db, const char *host, const char *table)
{

  if ( single )
    {
      return single;
    }

  const char *rdb; 
  const char *rhost;

  if ( db) rdb = db;
  else
    {
      rdb = getenv("PGDB");
      if ( !rdb) 
	{
	  std::cout << "no database specified and PGDB not set" << std::endl;
	  return 0;
	}
    }

  if ( host ) 
    {
      rhost = host;
      //rhost = new char [ strlen(host)+1];
      
    }

  else
    {
      rhost = getenv("PGHOST");
      if ( !rhost) 
	{
	  std::cout << "no host specified and PGHOST not set" << std::endl;
	  return 0;
	}
    }
  try
    {
      single = new et_PGConnection( rdb,  rhost, table );
    }
  catch ( odbc::SQLException e ) 
    {
      std::cout << "Database open failed: " << e.getMessage() << std::endl;
      delete single;
      single =0;
    }

  return single;

}

et_PGConnection::et_PGConnection ( const char * db, const char *host, const char *table )
{

  theconnection = odbc::DriverManager::getConnection(db,"phnxrc","");
  TheTable = table;
  
  LogEnabled = 1;
}


int et_PGConnection::addRecord ( const std::string name, const int runnumber, const int sequence)
{
  if ( ! LogEnabled  ) return 0;  // return if our services are not required. 
 
  std::ostringstream oss;

  //  std::cout <<  "insert into prdflist values ( \'"  << name << "\', " << runnumber << "," << sequence << ", now()" << " )" << std::endl;
  oss <<  "insert into "<< TheTable << " values ( \'"  << name << "\', " << runnumber << "," << sequence 
      << ", 0, \'created by AML on " << getenv("HOSTNAME") << "\', now(), 0, now(), 0, \' \',\'" <<  getenv("HOSTNAME") << "\'  )" << std::ends;

  //  std::cout << oss.str() << std::endl;

  odbc::Statement *st = 0;

  int i;

  try 
    {
      st = theconnection->createStatement( );
      i = st->executeUpdate ( oss.str() );
    }  
  catch ( odbc::SQLException e ) 
    {
      std::cout << "Database transaction failed: " << e.getMessage() << std::endl;
      // delete st;
      return -1;
    }
  
  delete st;

  if ( i ==1) return 0;   // one row should have been affected

  return -1;              // just in case. 

  //  return 0;

}


int et_PGConnection::updateSize_Events ( const std::string name, const unsigned long long size, const int events)
{
  if ( ! LogEnabled  ) return 0;  // return if our services are not required. 
 
  std::ostringstream oss;

  oss <<  "update " << TheTable << " set size=" << size << ",events=" << events  
      <<  " where filename=\'"  << name << "\'" << std::ends;

  //std::cout << oss.str() << std::endl;
  
  odbc::Statement *st = 0;


  int i;
  try 
    {
      st = theconnection->createStatement( );
      i = st->executeUpdate ( oss.str() );
    }  
  catch ( odbc::SQLException e ) 
    {
      std::cout << "Database transaction failed: " << e.getMessage() << std::endl;
      //delete st;
      return -1;
    }
  
  delete st;

  if ( i ==1) return 0;   // one row should have been affected

  return -1;              // just in case. 

  //  return 0;

}

int et_PGConnection::updateMD5sum ( const std::string name, const char *md5)
{
  if ( ! LogEnabled  ) return 0;  // return if our services are not required. 
 
  std::ostringstream oss;

  oss <<  "update " << TheTable << " set md5sum=\'" << md5 << "\'" 
      << " where filename=\'"  << name << "\'" << std::ends;

  //std::cout << oss.str() << std::endl;
  
  odbc::Statement *st;


  int i;
  try 
    {
      st = theconnection->createStatement( );
      i = st->executeUpdate ( oss.str() );
    }  
  catch ( odbc::SQLException e ) 
    {
      std::cout << "Database transaction failed: " << e.getMessage() << std::endl;
      //delete st;
      return -1;
    }
  
  delete st;

  if ( i ==1) return 0;   // one row should have been affected

  return -1;              // just in case. 

  //  return 0;

}


