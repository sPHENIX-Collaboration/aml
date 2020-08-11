
#ifndef __ET_STRUCTURE_H__
#define __ET_STRUCTURE_H__

#define MAX_THREADS 64  // must be an even number

  typedef struct control
  {
    unsigned long long bytes_written;
    unsigned long long filesize;             // in MBytes
    unsigned long long current_filesize;     // this is in bytes

    int current_buffer;
    int current_event;
    int current_run;
    int writeflag;

    int port;
    int pid;
    int alive;
    int has_started_ok;

    int event_fraction;
    int current_filesequence;
    int filesequence_start;
    int filesequence_increment;

    int compression;   //0-off 1-on
    int connection_open;
    int number_evts_segment;
    int number_evts_run;

    int number_of_connections;
    int globalerrorstatus;
    int errorstatus[MAX_THREADS];
    int alignment_dummy;

    char et_name[256];
    char current_filename[256];
    char filerule[256];
    char host_name[MAX_THREADS][64];

    pthread_t threadid [MAX_THREADS];

  } controlbuffer;

#endif
