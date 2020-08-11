
#include <iomanip>
#include <et_control.h>

#ifdef DATABASE
#include "et_PGConnection.h"
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <sys/vfs.h>
#include <sys/statfs.h>


using namespace std;

et_control::et_control(int &status, const char * token,  const int id)
{
  
  the_token = new char[strlen(token)+1];
  strcpy (the_token, token);
  the_id = id;
  pthread_mutex_init(&M, 0); 

  controlmemid = new sharedmemory (the_token, the_id, 
				   &ctrlbuf, (int) sizeof(*ctrlbuf),
				   status, 0);
  ObjectIsDefunct= status;

#ifdef DATABASE
  //  pgconn = et_PGConnection::instance("daq", "phnxdb0.phenix.bnl.gov", "prdflist");
  pgconn = 0;  // all utils use this one, they don't need db access
#endif

}

et_control::et_control(int &status, const char *et_name, 
		       const char * token,  const int id, const int db_flag, 
		       const int dfree_warning, const int dfree_severe )
{
  
  the_token = new char[strlen(token)+1];
  strcpy (the_token, token);
  the_id = id;
  pthread_mutex_init(&M, 0); 

  controlmemid = new sharedmemory (the_token, the_id, 
				   &ctrlbuf, (int) sizeof(*ctrlbuf),
				   status, 1);

  ObjectIsDefunct= status;

  if ( ObjectIsDefunct) return;

  diskfree_warning = dfree_warning;
  diskfree_severe = dfree_severe;

  ctrlbuf->has_started_ok = 0; // changed to not ok yet

  for (int i = 0; i < MAX_THREADS; i++)
    {
      ctrlbuf->threadid[i] = 0;
      strcpy (ctrlbuf->host_name[i], " ");
      ctrlbuf->errorstatus[i]=0;
    }
  ctrlbuf->globalerrorstatus = 0;
  
  strcpy(ctrlbuf->filerule,"run_%07d.evt");
  strcpy(ctrlbuf->et_name,et_name);
  strcpy(ctrlbuf->current_filename, " ");

  ctrlbuf->writeflag = 1;
  ctrlbuf->pid = getpid();

  ctrlbuf->event_fraction =1;

  ctrlbuf->bytes_written =0;
  ctrlbuf->current_buffer=0;
  ctrlbuf->current_event=0;
  ctrlbuf->current_run=0;

  ctrlbuf->filesequence_increment=1;
  ctrlbuf->filesequence_start=0;

  ctrlbuf->filesize=0;
  ctrlbuf->current_filesequence=0;
  
  file_is_open = 0;
  ctrlbuf->number_of_connections=0;
  ctrlbuf->number_evts_segment=0;
  ctrlbuf->number_evts_run=0;
  ctrlbuf->connection_open=0;
  

#ifdef DATABASE
  if ( db_flag) 
    {
      pgconn = et_PGConnection::instance("daq", "phnxdb0.phenix.bnl.gov", "prdflist");
    }
  else
    {
      pgconn = 0;
    }
#endif


}

et_control::~et_control()
{
  delete the_token;
  delete controlmemid;
  pthread_mutex_destroy(&M); 
}


int et_control::wait_for_ok(const int i) const
{

  return 0;
  
  //  if ( controlmemid->i_am_owner() ) return 0;

  //  for ( int j = 0; j < i; j++)
  //   {
  //    if  ( ctrlbuf->has_started_ok ) return 0;
  //    sleep (1);
  //  }
  // return 1;
}




int et_control::set_event_fraction (const int fraction)
{
  if (ObjectIsDefunct) return -1;
  if ( fraction > 0)
    ctrlbuf->event_fraction = fraction;
  return 0;
}

int et_control::set_filerule (const char *rule)
{
  
  if (ObjectIsDefunct) return -1;
  strcpy(ctrlbuf->filerule,rule);
  return 0;
}

// this routine evaluates the disk space left.
int et_control::updateErrorStatus()
{
  
  if (ObjectIsDefunct) return -1;
  if ( !file_is_open) return -1;

  // from here on, the_filedescriptor points to a file
  // and we figure out the free space on the file system it's on

  struct statfs sf;
 
  long long free_diskspace; 
 

  int status = fstatfs ( the_filedescriptor, &sf);
  if (status < 0)
    {
      perror ("error");
      return 1;
    }

  //  sf.f_bavail * sf.f_bsize is bytes, and we get MB
  free_diskspace = sf.f_bavail;
  free_diskspace = free_diskspace * sf.f_bsize /( 1024 * 1024);
  
  // cout << __LINE__ << "  " <<__FILE__ << "  " << sf.f_bavail << "  " << sf.f_bsize << "  " << free_diskspace << "  " << diskfree_warning << "  " << diskfree_severe << endl;

  if ( free_diskspace < diskfree_warning)
    {
       ctrlbuf->globalerrorstatus |= AML_WARNING;
    }
  if ( free_diskspace < diskfree_severe)
    {
       ctrlbuf->globalerrorstatus |= AML_SEVERE;
    }

  return 0;
}



int et_control::set_runnumber(const int run_number)
{
  if (ObjectIsDefunct) return -1;
  ctrlbuf->current_run = run_number;
  return 0;
}

int et_control::open_file(const int run_number, int *fd)
{
  
  if (ObjectIsDefunct) return -1;
  if ( file_is_open) return -1;

  //  start md5 computing
  md5_init(&md5state);

  ctrlbuf->pid = getpid();
  ctrlbuf->bytes_written = 0;
  ctrlbuf->current_filesize = 0;
  ctrlbuf->current_filesequence = ctrlbuf->filesequence_start;
  ctrlbuf->current_run = run_number;
  ctrlbuf->number_evts_segment=0;
  ctrlbuf->number_evts_run=0;

 tryagain:
  sprintf( ctrlbuf->current_filename, ctrlbuf->filerule
	   , run_number,  ctrlbuf->current_filesequence);


  // test if the file exists, do not overwrite
  int ifd =  open(ctrlbuf->current_filename, O_WRONLY | O_CREAT | O_EXCL | O_LARGEFILE , 
		  S_IRWXU | S_IROTH | S_IRGRP );
  if (ifd < 0) 
    {
      //  cerr << "file exists, I will not overwrite " << ctrlbuf->current_filename << endl;
      ctrlbuf->current_filesequence += ctrlbuf->filesequence_increment;
      goto tryagain;
    }

  the_filedescriptor = ifd;

  *fd = ifd;

  file_is_open = FILE_DESCRIPTOR;

#ifdef DATABASE
  if ( pgconn) 
    {
      cout << "adding database record for " << ctrlbuf->current_filename << endl;
      pgconn->addRecord( basename (ctrlbuf->current_filename) , run_number , ctrlbuf->current_filesequence);
    }
#endif


  return 0;
}

int et_control::add_md5 ( char * buffer, const int bytecount)
{
  if (ObjectIsDefunct) return -1;
  if ( !file_is_open) return -1;
  //  if ( !pgconn) return 0;


  md5_append(&md5state, (const md5_byte_t *)buffer,bytecount );
	
  return 0;
}


int et_control::open_next_file(int *fd)
{
  if (ObjectIsDefunct) return -1;
  if ( !file_is_open) return -1;

  close (the_filedescriptor);
  
  cout << "Closing file " << ctrlbuf->current_filename << endl;

  calculate_digeststring();
  cout << "md5sum: " <<  digest_string << "  " << ctrlbuf->current_filename << endl;


#ifdef DATABASE
  if ( pgconn) 
    {
      time_t x =  time(0);
      cout <<  ctime(&x) << "  " << "updating database record for " << ctrlbuf->current_filename << " md5sum is " << digest_string << endl;
      pgconn->updateSize_Events( basename (ctrlbuf->current_filename) , ctrlbuf->current_filesize, ctrlbuf->number_evts_segment);
      pgconn->updateMD5sum( basename (ctrlbuf->current_filename) , digest_string);
    }
#endif


  if ( ctrlbuf->current_filesequence > 9999) 
    {
      cerr << "file sequence > 9999" << endl;
      return -2;
    }



  ctrlbuf->current_filesequence += ctrlbuf->filesequence_increment;
  ctrlbuf->current_filesize = 0;
  ctrlbuf->number_evts_segment=0;

  sprintf( ctrlbuf->current_filename, ctrlbuf->filerule
	   , ctrlbuf->current_run,  ctrlbuf->current_filesequence);


  // test if the file exists, do not overwrite
  int ifd =  open(ctrlbuf->current_filename, O_RDWR | O_CREAT | O_EXCL | O_LARGEFILE,
		  S_IRWXU | S_IROTH | S_IRGRP );
  if (ifd < 0) 
    {
      cerr << "file exists, I will not overwrite " << ctrlbuf->current_filename << endl;
      return -2;
    }

  //  start md5 computing
  md5_init(&md5state);


  the_filedescriptor = ifd;

#ifdef DATABASE
  if ( pgconn) 
    {
      cout << "adding database record for " << ctrlbuf->current_filename << endl;
      pgconn->addRecord( basename (ctrlbuf->current_filename) , ctrlbuf->current_run , ctrlbuf->current_filesequence);
    }

#endif

  *fd = ifd;
  file_is_open = FILE_DESCRIPTOR;
  // cout << "opened file " <<  ctrlbuf->current_filename << endl;
  return 0;
}


int et_control::close_file()
{
  if (ObjectIsDefunct) return -1;
  if ( !file_is_open) return -1;

  close (the_filedescriptor);

  cout << "Closing file " << ctrlbuf->current_filename << endl;

  calculate_digeststring();
  cout << "md5sum: " <<  digest_string << "  " << ctrlbuf->current_filename << endl;

#ifdef DATABASE
  if ( pgconn) 
    {
      
      time_t x =  time(0);
      cout <<  ctime(&x) << "  " << "updating database record for " << ctrlbuf->current_filename << " md5sum is " << digest_string << endl;
      
      pgconn->updateSize_Events( basename (ctrlbuf->current_filename) , ctrlbuf->current_filesize, ctrlbuf->number_evts_segment);
      pgconn->updateMD5sum( basename (ctrlbuf->current_filename) , digest_string);
    }
#endif

  ctrlbuf->current_filesize = 0;

  the_file = 0;
  file_is_open = 0;
  strcpy(ctrlbuf->current_filename, " ");
  return 0;
}


int et_control::calculate_digeststring()
{
  
  md5_finish(&md5state, md5_digest);

  for ( int i=0; i< 16; i++) 
    {
      sprintf ( &digest_string[2*i], "%02x",  md5_digest[i]);
    }
  digest_string[32] = 0;

  return 0;
}


int et_control::updateSize(const int nbytes)
{
  
  if (ObjectIsDefunct) return -1;
  ctrlbuf->bytes_written += nbytes;
  ctrlbuf->current_filesize += nbytes;
   
  if ( ctrlbuf->filesize !=0 && 
       ctrlbuf->current_filesize >= ctrlbuf->filesize*1024*1024) 
    {
      return 1;  // signal "make a new file"
    }
  else
    return 0;
}


// routines to test whether the server is alive
int et_control::is_alive()
{
  int i;
  if (ObjectIsDefunct) return -1;
  ctrlbuf->alive = 0;
  kill (ctrlbuf->pid, ET_CONTROL_SIGNAL);
  usleep(10);
  for (i = 1000; i--;)
    {
      if ( ctrlbuf->alive) 
	{
	  //	  cout << i << " retrires.." << endl;
	  return 1;
	}
    }
  //  for (i = 3; i--;)
  //    {
  //      if ( ctrlbuf->alive) 
  //	{
  //	  cout << i << " retries with sleep.." << endl;
  //	  return 1;
  //	}
  //      sleep(1);
  //    }


  return 0;
}

int et_control::confirm_alive()
{
  if (ObjectIsDefunct) return -1;
  ctrlbuf->alive = 1;
  return 0;
}

int et_control::wipe_mem()
{
  if (ObjectIsDefunct) return -1;
  
  memset (ctrlbuf, 0, sizeof (*ctrlbuf));
  return 0;
}


int et_control::set_started_ok(const int i)
{
  if (ObjectIsDefunct) return -1;
  if (i)
    ctrlbuf->has_started_ok = 1;
  else
    ctrlbuf->has_started_ok = 0;
  return 0;
}

int et_control::started_ok() const
{
  if (ObjectIsDefunct) return -1;
  return ctrlbuf->has_started_ok;
}


int et_control::incConnectionCount()
{
  pthread_mutex_lock(&M);
  ctrlbuf->number_of_connections++;
  pthread_mutex_unlock(&M);
  return ctrlbuf->number_of_connections;
}

int et_control::decConnectionCount()
{
  pthread_mutex_lock(&M);
  ctrlbuf->number_of_connections--;
  pthread_mutex_unlock(&M);
  return ctrlbuf->number_of_connections;
}

void et_control::setThreadId(const int my_index, pthread_t p )
{
  if (my_index < 0 || my_index >= MAX_THREADS) return;
  ctrlbuf->threadid[my_index] = p;
}


void et_control::clearThreadId(const int my_index )
{
  if (my_index < 0 || my_index >= MAX_THREADS) return;
  ctrlbuf->threadid[my_index] = 0;
}

pthread_t et_control::getThreadId(const int my_index)
{
  if (my_index < 0 || my_index >= MAX_THREADS) return 0;
  return  ctrlbuf->threadid[my_index];
}

int et_control::getErrorStatus(const int my_index) const
{
  if (my_index < 0 || my_index >= MAX_THREADS) return 0;
  return  ctrlbuf->errorstatus[my_index];
}
void et_control::setErrorStatus(const int my_index, const int i)
{
  if (my_index < 0 || my_index >= MAX_THREADS) return;
  ctrlbuf->errorstatus[my_index] = i;
}


void et_control::setHostName(const int my_index, const char *host )
{
  if (my_index < 0 || my_index >= MAX_THREADS) return;
  strcpy ( ctrlbuf->host_name[my_index], host);
}

void et_control::clearHostName(const int my_index )
{
  if (my_index < 0 || my_index >= MAX_THREADS) return;
  strcpy ( ctrlbuf->host_name[my_index], " ");
}

const char * et_control::getHostName(const int my_index)
{
  if (my_index < 0 || my_index >= MAX_THREADS) return " ";
  return  ctrlbuf->host_name[my_index];
}
