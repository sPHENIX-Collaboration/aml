#ifndef __ET_CONTROL_H__
#define __ET_CONTROL_H__

#include <pthread.h>
#include <sharedmemory.h>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <Event/md5.h>
#include <et_structure.h>

//#include "et_PGConnection.h"

#ifdef DATABASE
class et_PGConnection;
#endif

#define ET_CONTROL_SIGNAL 63

#define FILE_HANDLE 1
#define FILE_DESCRIPTOR 2


// these are the bit-mapped values for globalerrorstatus

#define AML_WARNING 1
#define AML_SEVERE  2
#define AML_ERROR   4



// this control class interfaces to the 
// a shared memory section. The shm is identified through
// a token and and id, which defaults to 1. 
// the class provides access to some values steering the behavior
// of the server. 

// enable_logging 
// disable_logging        switch the logging on or off. Note that
//                        this takes effect only at the begin of
//                        of a run, so that the next run is affected.
//                        It does not have an affect on the current run.

// get_logging            tell whether or not logging is enabled.

// then there is a so-called file rule, which specifies the
// way a real file name is constructed. It defaults to
// "run_%07d_%02d.prdf", which is taken as a control string for sprintf
// and results, for run 1234, in a file called "run_0001234_00.evt"

//  set_filerule( const char * rule) sets the rule
//  char *get_filerule() returns the rule
//  char *get_filename() returns the current filename

// the server can adjust the fraction of events actually put
// into the et pool by the event_fraction value. Set to 1, it
// will put all events there. Set to 5, it will only put each
// 5th event there.
// int set_event_fraction(const int fraction); sets it
// int get_event_fraction() returns the current value 

// In addition, the ndd_event_server provides some useful
// info like kb_written, current_buffer number, and so on.



class et_control {
 public:


  // this is the constructor which must be used by ndd_event_server
  // itself. It creates the shared memory and sets up the default values.
  // also, when deleted, it will free the memory again.


  // status == 0 means all ok, non-zero is bad.  
  et_control (int &status, const char *et_name, 
	      const char * token, const int id=1, const int db_flag=1, const int diskfree_warning=50*1024, const int diskfree_severe=10*1024 );

  // this is the constructor which must be used by utilities other
  // than the server (it just attaches to the memory but does not
  // create it). 

  // status == 0 means all ok, non-zero is bad.  
  et_control (int &status, const char * token,  const int id=1);
  ~et_control();

  // enable and disable logging to disk   
  //If this Object is defunct, we return non-zero status.
  inline int enable_logging()
    {
      if (ObjectIsDefunct) return -1;
      ctrlbuf->writeflag=1;
      return 0;
    };

  //If this Object is defunct, we return non-zero status.
  inline int disable_logging()
    {
      if (ObjectIsDefunct) return -1;
      ctrlbuf->writeflag=0;
      return 0;
    };      


  // -1 means error, 0 means logging disabled, 1 means enabled. 
  inline int get_logging() const 
    {
      if (ObjectIsDefunct) return -1;
      wait_for_ok();
      return ctrlbuf->writeflag;
    }; 

  // size here is in MB
  // -1 means error, 0 means ok
  inline int set_maxfilesize(const unsigned long long  size) 
    {
      if (ObjectIsDefunct) return -1;
      if (size >=0) ctrlbuf->filesize=size;
      return 0;
    }; 


  inline int set_filesequence_start(const int start) 
    {
      if (ObjectIsDefunct) return -1;
      if (start >=0) ctrlbuf->filesequence_start=start;
      return 0;
    }; 

  inline int set_filesequence_increment(const int incr) 
    {
      if (ObjectIsDefunct) return -1;
      if (incr >=1) ctrlbuf->filesequence_increment=incr;
      return 0;
    }; 

  //returns -1 on error, pid else
  inline int get_pid() const 
    {
      if (ObjectIsDefunct) return -1;
      wait_for_ok();
      return ctrlbuf->pid;
    }; 

  //returns -1 on error, filesize else
  inline unsigned long long get_maxfilesize() const 
    {
      if (ObjectIsDefunct) return 0;
      wait_for_ok();
      return ctrlbuf->filesize;
    }; 

  inline int get_current_file_sequence()const 
    {
      if (ObjectIsDefunct) return -1;
      wait_for_ok();
      return ctrlbuf->current_filesequence;
    }; 

  // nbytes is in bytes.
  // -1 : error. 1: make a new file, please. 0: don't. 
  int updateSize(const int nbytes);


  // 0- disabled, 1- enabled

  // get and set the port
  // -1 is error, 0 ok
  inline int set_port(const int i)
    {
      if (ObjectIsDefunct) return -1;
      ctrlbuf->port=i;
      return 0;
    };   

  // -1 error, otherwise it's the port
  inline int get_port()
    {
      if (ObjectIsDefunct) return -1;
      wait_for_ok();
      return ctrlbuf->port;
    };      

  // set the rule to construct a new filename
  // -1 is error, 0 ok
  int set_filerule( const char * rule); 

  // get the current rule
  // returns empty string on error
  const char * get_filerule() 
    {
      if (ObjectIsDefunct) return "";
      wait_for_ok();
      return ctrlbuf->filerule;
    };


  // -1 error, 0 ok
  int set_compression (const int c) 
    {  
      if (ObjectIsDefunct) return -1;
      ctrlbuf->compression = c; 
      return 0;
    };

  // -1 error, compression value else
  int get_compression () 
    { 
      if (ObjectIsDefunct) return -1;
      wait_for_ok();
      return ctrlbuf->compression; 
    };

  // -1 error, 0 ok
  int set_connection_open (const int c) 
    {  
      if (ObjectIsDefunct) return -1;
      ctrlbuf-> connection_open = c; 
      return 0;
    };

  // -1 error, open value else (1 means open)
  int get_connection_open () 
    { 
      if (ObjectIsDefunct) return -1;
      wait_for_ok();
      return  ctrlbuf-> connection_open; 
    }

  // open a filename made according to the rule
  // -1 error, 0 ok
  int open_next_file(int *fd);

  int open_file(const int run, int *fd);

  // -1 error, 0 ok
  int close_file();

  // get the current filename in use
  // returns empty string on error
  const char * get_filename() const 
    {
      if (ObjectIsDefunct) return "";
      wait_for_ok();
      return ctrlbuf->current_filename;
    } ;

  // -1 error, value else
  inline unsigned long long  get_mb_written() const 
    {
      if (ObjectIsDefunct) return 0;
      wait_for_ok();
      return ctrlbuf->bytes_written/(1024*1024);
    };

  // -1 error, value else
  inline unsigned long long  get_bytes_written() const 
    {
      if (ObjectIsDefunct) return 0;
      wait_for_ok();
      return ctrlbuf->bytes_written;
    };

  // -1 error, value else
  inline int get_current_buffer() const 
    {
      if (ObjectIsDefunct) return -1;
      wait_for_ok();
      return ctrlbuf->current_buffer;
    };

  // -1 error, value else
  inline int get_current_event() const 
    {
      if (ObjectIsDefunct) return -1;
      wait_for_ok();
      return ctrlbuf->current_event;
    };

  // -1 error, value else
  inline void set_current_event( const int n) 
    {
      if (ObjectIsDefunct) return;
      wait_for_ok();
      ctrlbuf->current_event = n;
    };

  // -1 error, value else
  inline int get_current_run() const 
    {
      if (ObjectIsDefunct) return -1;
      return ctrlbuf->current_run;
    };

  int set_runnumber(const int run_number);

  // returns empty string on error
  inline const char * get_et_name() const 
    {
      if (ObjectIsDefunct) return "";
      wait_for_ok();
      return ctrlbuf->et_name;
    };

  // -1 error, 0 ok
  int set_event_fraction (const int fraction);

  // -1 error, value else
  inline int get_event_fraction() const 
    {
      if (ObjectIsDefunct) return -1;
      return ctrlbuf->event_fraction;
    };


  int add_md5 ( char * buffer, const int bytecount);


  // routines to test whether the server is alive
  // -1 error, 0 means dead, 1 means server is alive
  int is_alive();

  // -1 error, 0 ok
  int confirm_alive();

  // -1 error, 0 ok
  int set_started_ok(const int i);

  // -1 error, 0 means dead, 1 means server is alive (weak test)
  int started_ok() const;

  // this allows to wipe the whole shared mem segment. Mem stays
  // in place but all is set to 0.
  int wipe_mem();

  int getConnections() const { return ctrlbuf->number_of_connections; } ;
  int incConnectionCount();
  int decConnectionCount();

  void setThreadId(const int my_index, pthread_t p );
  void clearThreadId(const int my_index );
  pthread_t getThreadId(const int my_index);

  int  getErrorStatus(const int my_index) const;
  void setErrorStatus(const int my_index, const int i);

  int  getGlobalErrorStatus() const {return ctrlbuf->globalerrorstatus;};
  void setGlobalErrorStatus() {ctrlbuf->globalerrorstatus |= AML_ERROR;};
  void clearGlobalErrorStatus() {ctrlbuf->globalerrorstatus =0;};
  int  updateErrorStatus();

  void setHostName(const int my_index, const char *host );
  void clearHostName(const int my_index );
  const char * getHostName(const int my_index);

  int wait_for_ok(const int i=15) const;
  
  int has_started() const { return ctrlbuf->has_started_ok; }

  int addEventCount ( const int n) 
    {   
      ctrlbuf->number_evts_segment+=n;
      ctrlbuf->number_evts_run+=n;
      return n;
    }

  int get_nr_events_segment () const 
    {   
      return ctrlbuf->number_evts_segment;
    }

  int get_nr_events_run () const 
    {   
      return ctrlbuf->number_evts_run;
    }



 protected:

  int calculate_digeststring();

  int ObjectIsDefunct;
  controlbuffer *ctrlbuf;
  sharedmemory *controlmemid;
  char *the_token;
  int the_id;
  FILE *the_file;
  int the_filedescriptor;
  int file_is_open;
  pthread_mutex_t M;

  // the "long" here is no mistake, we have to match what's in the "statfs" structure
  long diskfree_warning; // how many mb free when we warn
  long diskfree_severe; // how many mb free when we issue the "severe" warning

#ifdef DATABASE
  et_PGConnection *pgconn;
#endif

  md5_state_t md5state;
  char buffer[4096 + 72];

  md5_byte_t md5_digest[16];  
  char digest_string[33];


};
 
#endif /* __ET_CONTROL_H__ */
