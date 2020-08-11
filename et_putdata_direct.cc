/**

this is et_send_data, as realistic as possible, data as they would
come from the ATP's. It reads a buffer worth of data from an existing data file.
Then it transmits the data over and over again to a ET_server. 

The syntax is 

et_putdata_direct   -n <number of buffers> -w <wait in secs> -H <approx hertz>  filename

*/



#include <string.h>

#ifdef HAVE_BSTRING_H
#include <bstring.h>
#else
#include <strings.h>
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <iostream>
#include <iomanip>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/times.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <et.h>


#include <oBuffer.h>
#include <Event.h>
#include <testEventiterator.h>
#include <fileEventiterator.h>
#include <EventTypes.h>

void exitmsg()
{
  std::cout << "** usage: et_putdata_direct  -n numberofevents -w waitinseconds  -H hz datafile etname" << std::endl;
  exit(0);
}

et_att_id	attach1;
et_sys_id id;
et_openconfig openconfig;
et_event *pe;

int main( int argc, char* argv[])
{
  int number_of_events;
  int status;
  char c;
  char *et_name = 0;
  char *file_name = 0;
  char *pdata;


  number_of_events = 1;
  int waitinterval = 0;
  int identifyflag = 0;
  int Hertz = 0;
  timespec nsleep;
  nsleep.tv_sec = 0;
  while ((c = getopt(argc, argv, "iw:n:H:")) != EOF)
    {
      switch (c)
        {
        case 'n':
          sscanf (optarg, "%d", &number_of_events);
          break;
        case 'w':
          sscanf (optarg, "%d", &waitinterval);
          break;
        case 'i':
          identifyflag = 1;
          break;
        case 'H':
          sscanf (optarg, "%d", &Hertz);
          nsleep.tv_nsec = 999999999 / Hertz;
          break;
        }
    }

  if (argc >= optind + 2)
    {

      file_name = argv[optind];
      et_name = argv[optind + 1];
    }
  else
    {
      exitmsg();
    }

  // now we assemble the buffer from the file
  // when we read, we skip the first couple of events in order to get to
  // over the begrun etc events in the beginning.

  Eventiterator *testIt = new fileEventiterator(file_name, status);

  if (status)
    {
      std::cout << "could not open the event data file" << std::endl;
      exit(1);
    }

  Event *evt;



  /* open ET system */
  et_open_config_init(&openconfig);

  /* here we set the policy how to find an et pool identified by et_name:
     ET_HOST_ANYWHERE: uses a broadcast to find et system but we really don't 
     want to send data over the network to a remote et pool
     in case the local et died and we didn't notice (avoid
     the 3 am phone call: 'how come we only write 8 MB/sec
     instead of 50 MB/sec'
     ET_HOST_LOCAL: check for et only on local machine and exit with complain
     if not found (which is what we really want)
  */
  /*  et_open_config_sethost(openconfig,ET_HOST_ANYWHERE); */
  et_open_config_sethost(openconfig, ET_HOST_LOCAL);

  if (et_open(&id, et_name, openconfig) != ET_OK)
    {
      printf("%s: et_open problems\n", argv[0]);
      exit(1);
    }
  et_open_config_destroy(openconfig);


  /* set level of debug output (everything) */
  et_system_setdebug(id, ET_DEBUG_INFO);

  /* attach to grandcentral station */
  if (et_station_attach(id, ET_GRANDCENTRAL, &attach1) < 0)
    {
      printf("%s: error in et_station_attach\n", argv[0]);
      exit(1);
    }
  else
    {
      printf("%s: attached to Grand Central\n", argv[0]);
    }

  //  signal(SIGKILL, sig_handler);
  //  signal(SIGTERM, sig_handler);
  //  signal(SIGINT, sig_handler);


  //look for the first data event
  /*
    evt = testIt->getNextEvent();
    while (evt->getEvtType() != 1 )
    {
    delete evt;
    evt = testIt->getNextEvent();
    }

    if (identifyflag)
    {
    evt->identify();
    }
  */ 
  //now send a begin run instruction

  while (number_of_events-- > 0 )
    {
      evt = testIt->getNextEvent();
      if (!evt)
        {
          std::cout << "Error reading Event from file" << file_name << std::endl;
          break;
        }
      if (identifyflag)
        {
          evt->identify();
        }

      // evt->getEvtLength() is words, et needs length in bytes (len*4)
      // <<2 is same as *4
      status =
        et_event_new(id, attach1, &pe, ET_SLEEP, NULL, (evt->getEvtLength()) << 2 );
      //       std::cout << number_of_events << std::endl;

      if (status != ET_OK)
        {
          printf("%s: ERROR in et_event_new, status: %d\n", argv[0], status);
          goto deleteevent;
        }
      status = et_event_getdata(pe, (void **) & pdata);
      if (status != ET_OK)
        {
          printf("%s: ERROR in et_event_getdata, status: %d\n", argv[0], status);
          break;
        }
      int nw;
      evt->Copy((int *) pdata, evt->getEvtLength(), &nw);

      et_event_setlength(pe, (evt->getEvtLength()) << 2 );
   static int ctrl[4] = {-1,-1,-1,-1};
  ctrl[0] = evt->getEvtType();
  // this will make non data events pass the et trigger selection
  if (ctrl[0] != DATAEVENT)
    {
      ctrl[1] = 0xFFFFFFFF;
      ctrl[3] = 0xFFFFFFFF;
    }
  else
    {
      ctrl[1] = evt->getTagWord(0);
      ctrl[3] = evt->getTagWord(1);
    }
  et_event_setcontrol(pe,ctrl,4);
     status = et_event_put(id, attach1, pe);
      if (status != ET_OK)
        {
          printf("%s: ERROR in et_event_put, status: %d\n", argv[0], status);
          break;
        }
      if (Hertz > 0)
        {
          nanosleep(&nsleep, NULL);
        }
      if (waitinterval > 0)
        {
          sleep(waitinterval);
        }
    deleteevent:
      delete evt;
    }
  et_station_detach(id, attach1);
  sleep(2);

  return 0;
}
