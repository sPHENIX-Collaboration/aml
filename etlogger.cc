/* if you want a printout of bytes/sec written define BYTESPERSEC */

//#define BYTESPERSEC
 
#include <stdlib.h>
#include <unistd.h>
#include <fileEventiterator.h>
#include <testEventiterator.h>
//#include <etEventiterator.h>
#include <phenixTypes.h>
#include <ogzBuffer.h>
#include <oBufferThreaded.h>
#include <EventTypes.h>
#include <oEvent.h>
#include <et_control.h>
#include <A_Event.h>
#include <stdio.h>
#include <et.h>


#ifdef BYTESPERSEC
#include <sys/time.h>
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifdef __WITH_OBJY
#include <rcObjyManager.h>
rcObjyManager *rcm;
#endif

#if defined(SunOS) || defined(Linux) 
void sig_handler(int);
void et_control_handler(int);
#else
void sig_handler(...);
void et_control_handler(...);
#endif

et_control *nd = 0;

int multiple_files = 0;  //by default, do not break up in individual files 
int filesize = 0;  //by default, do not break up in individual files 

// et pool stuff

et_sys_id	 et_id;
et_stat_id	 log_stat_id;
et_att_id	 log_att_id;



void exitmsg()
{
  std::cout << "** usage: etlogger filerule et_name " << std::endl;
  std::cout << "          etlogger -h for more help" << std::endl;
  exit(0);
}

void exithelp()
{
  std::cout <<  std::endl;
  std::cout << " Syntax:"<< std::endl;
  std::cout <<  std::endl;
  std::cout << "      etlogger [-v] [-i] [-n number] [-u] [-h] filerule ddpool"<< std::endl;
  std::cout << " e.g  etlogger -v \'/rcf_data/dcm_data/run_%07d_0%2d.prdfz\' DD_0" << std::endl;
  std::cout <<  std::endl;
  std::cout << " etlogger reads events from the given DD pool and writes them to a file. " << std::endl;
  std::cout << " (please note the quotes around the file rule to protect it from the shell)" << std::endl;
  std::cout << " This would retrieve events from the dd pool called DD_0 and write them to" << std::endl;
  std::cout << "  a file named according to the file rule." << std::endl;
  std::cout << "" << std::endl;
  std::cout << " With the above rule, events from run 1331 would go to a file called" << std::endl;
  std::cout << " /rcf_data/dcm_data/run_0001331_00.prdfz" << std::endl;
  std::cout << " where the latter number is the file sequence number." << std::endl;
  std::cout << " A new file gets started if" << std::endl;
  std::cout << "  - the event number changes" << std::endl;
  std::cout << "  - we get an end-run-event on the previous event" << std::endl;
  std::cout << "  - the file rollover limit is reached." << std::endl;
  std::cout << "" << std::endl;
  std::cout << " Options:" << std::endl;
  std::cout << "  -t  <token>  " << std::endl;
  std::cout << "  -f  <station> set the station name in ET pool" << std::endl;
  std::cout << "  -v   verbose, without that etlogger does its work silently" << std::endl;
  std::cout << "  -i   identify the events as they are processed, good for debugging" << std::endl;
  std::cout << "  -n  <number>  stop after so many events - not yet implemnented" << std::endl;
  std::cout << "              (note: dpipe may be better for this)" << std::endl; 
  std::cout << "  -s  <size in MB>    rollover the file after so many Megabytes writte" << std::endl;
  std::cout << "  -h  this message" << std::endl;
  std::cout << "" << std::endl;

  exit(0);
}

char *file_rule;

// The global pointer to the Eventiterator (we must be able to 
// get at it in the signal handler) 
//etEventiterator *it;

oBuffer *ob;
int fd;

int file_open = 0;

void cleanup(const int exitstatus);



int 
main(int argc, char *argv[])
{
  int c;
  int status;

  //  int waitinterval = 0;
  int verbose = 0;
  int identify = 0;
  int maxevents = 0;
  int eventnr = 0;
  int gzipcompress = 0;
  int filesequence_increment=1;
  int filesequence_start=0;


  extern char *optarg;
  extern int optind;

  PHDWORD  *buffer;
  //char *stationname;

  char stationname[132];

  //  et pool stuff
  et_statconfig	 stat_config;
  et_openconfig  et_config;
  et_event       *pe;

  // initialize the pointers to 0;
  fd = 0;
  ob = 0;
  //it = 0;

  int buffer_size = 256*1024*4;  // makes 4MB (specifies how many dwords, so *4)
  //int buffer_size = 256*1024;  // makes 16MB (specifies how many dwords, so *4)

#ifdef __WITH_OBJY
  if (getenv ("ETLOGGER_OBJYBOOT"))
    {
      ooInit();
    }
  else
    {
      rcm = 0;
    }
#endif


#ifdef BYTESPERSEC
  unsigned long byteswritten = 0;
  double bytespersec;
  struct timeval  t1, t2;
  double time;
#endif

  //char userid[L_cuserid];
  char token[L_cuserid+128];
  int token_set = 0;
  strcpy(token,"/tmp/etlogger_");

  if (argc < 2) exitmsg();

  while ((c = getopt(argc, argv, "t:s:a:b:vizh")) != EOF)
    {
      switch (c) 
	{

	case 'a':   // size of the file pieces
	  if ( !sscanf(optarg, "%d", &filesequence_increment) ) exitmsg();
	  std::cout << "File sequence increment is " << filesequence_increment << std::endl;
	  break;

	case 'b':   // size of the file pieces
	  if ( !sscanf(optarg, "%d", &filesequence_start) ) exitmsg();
	  std::cout << "File sequence start is " << filesequence_start << std::endl;
	  break;

	case 't':
	  strcat (token,optarg);
	  token_set = 1;
	  break;

	case 'v':   // verbose
	  verbose = 1;
	  break;

	case 'i':   // identify
	  identify = 1;
	  break;

	case 'z':   // do gzip-compress
	  gzipcompress = 1;
	  break;

	case 'h':
	  exithelp();
	  break;

	case 's':   // size of the file pieces
	  if ( !sscanf(optarg, "%d", &filesize) ) exitmsg();
	  std::cout << "File size to roll over at is " << filesize << std::endl;
	  break;

	default:
	  break;
						
	}
    }


  // install some handlers for the most common signals
  signal(SIGKILL, sig_handler);
  signal(SIGTERM, sig_handler);
  signal(SIGINT,  sig_handler);
  signal(ET_CONTROL_SIGNAL, et_control_handler);
  



  if (! token_set)
    {
      if (getenv("ETLOGGER_ID") == NULL)
	{
	  // strcat(token, cuserid(userid));
	}
      else
	{
	  strcat(token,getenv("ETLOGGER_ID"));
	}
    }


  nd = new et_control (status, argv[optind+1] , token );
  if (status) 
    {
      cleanup(1);
    }

  nd->set_compression(gzipcompress);

  nd->set_maxfilesize(filesize);

  nd->set_filesequence_start(filesequence_start);
  nd->set_filesequence_increment(filesequence_increment);

  if (argc > 1) nd->set_filerule(argv[optind]);
  std::cout << "ctrl string: " <<  nd->get_filerule() << std::endl;

  //  copy the file rule
  //  file_rule = new char[strlen(argv[optind]+1)];
  //  strcpy ( file_rule, argv[optind] );
  


  // see if we can open the stream
  // we make a "blocking" dd client, one which requests all events.
  //if (!station_name_is_set) 
  //  stationname = "TAPE";

  // it = new etEventiterator(argv[optind+1], stationname, status);

  // set up logging station
  //sprintf(stationname,"LOGGER_%d",getpid()); // attach pid to station name
  sprintf(stationname,"LOGGER"); // attach pid to station name
    
  et_open_config_init(&et_config);
  et_open_config_setwait(et_config, ET_OPEN_WAIT);
  et_open_config_sethost(et_config,ET_HOST_LOCAL);
  if (et_open(&et_id, argv[optind+1], et_config) != ET_OK) {
    printf("etlogger: et_open problems for pool %s\n",argv[optind+1]);
    exit(1);
  }
  et_open_config_destroy(et_config);
 
  et_station_config_init(&stat_config);
  //  et_station_config_setuser(stat_config, ET_STATION_USER_SINGLE);
  et_station_config_setuser(stat_config, ET_STATION_USER_MULTI);
  et_station_config_setrestore(stat_config, ET_STATION_RESTORE_OUT);
  et_station_config_setprescale(stat_config, 1);
  /* et_station_config_setcue(stat_config, 10); no effect on blocking stats */
  et_station_config_setselect(stat_config, ET_STATION_SELECT_ALL);
  et_station_config_setblock(stat_config, ET_STATION_BLOCKING);

  if ((status = et_station_create(et_id, &log_stat_id, stationname, stat_config)) < ET_OK) {
    if (status == ET_ERROR_EXISTS) {
      /* my_stat contains pointer to existing station */
      printf("etlogger: station already exists\n");
    }
    else if (status == ET_ERROR_TOOMANY) {
      printf("%s: too many stations created\n",argv[0]);
      cleanup(1);
    }
 
    else {
      printf("%s: error in station creation\n",argv[0]);
      cleanup(1);
    }
  }
  et_station_config_destroy(stat_config);
  status = et_station_setposition(et_id,log_stat_id,1,0);
  if (status == ET_OK)
    {
      printf("%s: station positioned to front of chain\n",argv[0]);
    }
  else
    {
      printf("%s: failed station to position to front of chain\n",argv[0]);
    }
  if (et_station_attach(et_id, log_stat_id, &log_att_id) < 0) {
    printf("et_client: error in station attach\n");
    cleanup(1);
  }

  if (verbose) std::cout << "opened pool " << argv[optind+1] << std::endl;

  //  std::cout << "waitinterval is " << waitinterval << std::endl;
  // ok. now go through the events

  int run;
  int *rawEvent;

  static int previous_run = -999999;
  static int current_filesequence = 0;

  buffer = new PHDWORD [buffer_size];

  while ( ( maxevents == 0 || eventnr < maxevents) )

    {

      //  if (!et_alive(et_id)) 
      //{
      //cout << "not alive" << std::endl;
      // return 0;
      //}
      status = et_event_get(et_id, log_att_id, &pe, ET_SLEEP, 0);
      if (status != ET_OK) 
	{
	  std::cout << "not ok" << std::endl;
	  return 0;
	}

      et_event_getdata(pe, (void **) &rawEvent);

      // ( rawEvent = it->getNextEventData())  )
      //while ( ( maxevents == 0 || eventnr < maxevents)  &&
      //	  ( evt = it->getNextEvent())  )

      //      if (identify) 
      //	{
      //	  evt->identify();
      //	}
	  
      //run = evt->getRunNumber();
      run = rawEvent[3];

      // now we use the run number and construct the file name 
      if ( file_open == 0 ||   run != previous_run)
	{
	  
	  // get rid of the old output buffer, if any (flushes the output)
	  if (ob) delete ob;
	  ob = 0;

	  if ( file_open ) 
	    {
	      nd->close_file();
	      if (verbose) std::cout << " File " << nd->get_filename() << " closed." <<std::endl;
	      //#ifdef __WITH_OBJY
	      //	      if (rcm)
	      //		{
	      //		  int dummy = 0;
	      //		  rcm->endRun( &dummy,1, (PHDWORD *) &dummy);
	      //		}
	      //#endif
	      
	    }
	      
	  file_open = 0;

	  // new run - we start a new file sequence no matter what 
	  current_filesequence = 0;
	  eventnr = 0;

	  // update the run
	  previous_run = run;


#ifdef BYTESPERSEC
          byteswritten = 0;
	  gettimeofday(&t1, NULL);
#endif
	  if (verbose) std::cout << "Opened file: " <<  nd->get_filename() << std::endl;
	  
	  gzipcompress = nd->get_compression();
	  if (gzipcompress) 
	    {
	      // use the file rule to make a new file name
	      int i = nd->open_file(run, &fd);
	      if (i) 
		{
		  std::cout << "Could not open file: " <<  nd->get_filename() << std::endl;
		  cleanup(1);
		}
	      // remember that we opened a file
	      file_open = 1;

	      ob = new ogzBuffer (fd, buffer, buffer_size,3,run);
	    }
	  else
	    {
	      int i = nd->open_file(run, &fd);
	      if (i) 
		{
		  std::cout << "Could not open file: " <<  nd->get_filename() << std::endl;
		  cleanup(1);
		}
	      // remember that we opened a file
	      file_open = 1;
	      //ob = new oBuffer (fd, buffer, buffer_size);
	      ob = new oBufferThreaded (fd, buffer_size, run);
	    }
#ifdef __WITH_OBJY
	  rcm =  new rcObjyManager( getenv ("ETLOGGER_OBJYBOOT") );
	  int dummy = 0;
	  rcm->logNewRun( run , (PHDWORD *) &dummy);
	  rcm->endRun( &dummy,1, (PHDWORD *) &dummy);
	  delete rcm; 
	  rcm = 0;
#endif



	}
      
      //ob->addEvent(evt);
      ob->addRawEvent(rawEvent);

      eventnr++;

#ifdef BYTESPERSEC
      byteswritten += rawEvent[0]*4;
#endif
      // see if we hit the end run event
      //if ( evt->getEvtType() == ENDRUNEVENT)
      if ( rawEvent[1] == ENDRUNEVENT)
	{

	  // get rid of the old output buffer, if any (flushes the output)

	  if (ob) delete ob;
	  ob = 0;

	  if ( file_open ) 
	    {
	      nd->close_file();
	      if (verbose) std::cout << " End of Run -- File " << nd->get_filename() << " closed." <<std::endl;
	    }
	      
	  file_open = 0;

	  // new run - we start a new file sequence no matter what 
	  current_filesequence = 0;
	  eventnr = 0;

	  // update the run
	  previous_run = -9999999;
	}
	  
      else  // if it's not the endrun...
	{
	  //   if ( nd->updateSize(evt->getEvtLength()*4) ) 
	  if ( nd->updateSize(rawEvent[0]*4) ) 
	    {
	      //     std::cout << "Starting next file..." << std::endl;
             
#ifdef BYTESPERSEC
	      gettimeofday(&t2, NULL);
	      time = (double)(t2.tv_sec - t1.tv_sec) + 1.e-6*(t2.tv_usec - t1.tv_usec);
	      bytespersec = (byteswritten/time)/1000000.; /* MB/sec */
	      std::cout << "wrote " << bytespersec << " MB/sec" << std::endl;
	      byteswritten = 0;
#endif
	      if (ob) delete ob;
	      ob = 0;

#ifdef BYTESPERSEC
	      gettimeofday(&t1, NULL);
#endif
	      if (verbose) std::cout << "Starting next file " << nd->get_filename() << std::endl;
	      if (gzipcompress) 
		{
		  int i =  nd->open_next_file(&fd);
		  if (i) 
		    {
		      std::cout << "Could not open file: " <<  nd->get_filename() << std::endl;
		      cleanup(1);
		    }
		  // remember that we opened a file
		  file_open = 1;
	      
		  ob = new ogzBuffer (fd, buffer, buffer_size ,3, run);
		}
	      else
		{
		  int i = nd->open_next_file(&fd);
		  if (i) 
		    {
		      std::cout << "Could not open file: " <<  nd->get_filename() << std::endl;
		      cleanup(1);
		    }
		  // remember that we opened a file
		  file_open = 1;
		  //ob = new oBuffer (fd, buffer, buffer_size);
		  ob = new oBufferThreaded (fd, buffer_size, run);
		}
	    }
	  
	}
      //delete evt;
      //it->releaseEventData();
      status = et_event_put(et_id, log_att_id, pe);
      //   if (waitinterval > 0) sleep(waitinterval);
	  

    }
  et_station_detach(et_id, log_att_id);
  status = et_station_remove(et_id, log_stat_id);
  if (status == ET_OK)
    {
      printf("etlogger: removed stat %d\n", log_stat_id);
    }
  else 
    {
      printf("etlogger: error removing stat (%d)\n", status);
    }
      
      
  delete ob;
  nd->close_file();
      
  
  return 0;
}

  
#if defined(SunOS) || defined(Linux) 
void sig_handler(int i)
#else
  void sig_handler(...)
#endif
{
  int status;

  std::cout << "sig_handler: signal seen " << std::endl;
  //if (it) delete it;

  status = et_station_detach(et_id, log_att_id);
  if (status == ET_OK)
    printf("etlogger: detached stat %d\n", log_att_id);
  else 
    {
      if (status == ET_ERROR)
	{
	  printf("etlogger: error detaching stat (%d)\n", status);
	}
      else if(status == ET_ERROR_READ)
	{
	  printf("etlogger: read error detaching stat (%d)\n", status);
	}
      else if(status == ET_ERROR_WRITE)
	{
	  printf("etlogger: write error detaching stat (%d)\n", status);
	}
      else if(status == ET_ERROR_DEAD)
	{
	  printf("etlogger: et system dead (%d)\n", status);
	}
      else
	{
	  printf("etlogger: unspecified error detaching stat (%d)\n", status);
	}

    }
  status = ET_ERROR;
  printf("etlogger: removing station\n");
  status = et_station_remove(et_id, log_stat_id);

  if (ob) delete ob;
  if ( file_open && fd ) close(fd);
  if (nd) delete nd;

#ifdef __WITH_OBJY
  if (rcm) delete rcm;
#endif

  exit(0);
}

#if defined(SunOS) || defined(Linux) 
void et_control_handler(int i)
#else
  void et_control_handler(...)
#endif
{

  nd->confirm_alive();
  signal(ET_CONTROL_SIGNAL, et_control_handler);

}

void cleanup( const int exitstatus)
{
  int status;
  //if (it) delete it;
  status = et_station_detach(et_id, log_att_id);
  if (status == ET_OK)
    printf("etlogger: detached from stat %d\n", log_stat_id);
  else {
    printf("etlogger: error detaching from stat (%d)\n", status);
  }
  status = et_station_remove(et_id, log_stat_id);

  if (ob) delete ob;
  if (file_open && fd) close(fd);
  if (nd) delete nd;

  exit(exitstatus);
}
