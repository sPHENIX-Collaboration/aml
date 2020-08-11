#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <float.h>
#include <netdb.h>
#include <signal.h>

#include "et.h"
#include "Event.h"

#if defined(SunOS) || defined(Linux)
void sig_handler(int);
#else
void sig_handler(...);
#endif

#define DAQMON_OK 1
#define DAQMON_FAIL 0

int daq_monitor(Event *);
void cleanup(int);

et_sys_id	et_in_id, et_good_out_id;
et_stat_id	in_stat;
et_att_id	in_att, attach1, attach2;

int main(int argc, char **argv)
{
  et_sys_id	et_out_id;
  et_statconfig	sconfig;
  et_att_id attach;
  et_openconfig et_in_config;
  et_openconfig et_good_out_config;
  int	err, status;
  et_event *ev, *pe;
  struct timespec twait;
  struct timespec tsleep;
  char statbuf[100];
  char	*station, *pdata, *outdata;
  unsigned long int neventstotal;
  struct timeval t1;
  int iret;
  //int daqret;
  //  int prio;

  int len;
  station = statbuf;
  if ((argc != 3) && (argc != 4))
    {
      printf("Usage: et_online_monitor <et_input_filename> <et_output_filename>  [<input station>]\n");
      exit(1);
    }
  if (argc == 4)
    {
      sprintf(statbuf, "%s", argv[argc - 1]);
    }
  else
    {
      sprintf(statbuf, "%s", "MONITOR");
    }
  printf("will create station %s on input box\n", station);
  twait.tv_sec = 1;
  twait.tv_nsec = 0;

  tsleep.tv_sec = 0;
  tsleep.tv_nsec = (int) 999999999 / 50;

  /* input et pool */
  et_open_config_init(&et_in_config);
  et_open_config_sethost(et_in_config, ET_HOST_ANYWHERE);

  /*  if (et_open(&id, argv[1], et_in_config) != ET_OK) { */
  if ( (iret = et_open(&et_in_id, argv[1], et_in_config)) != ET_OK)
    {
      printf("et_online_monitor: cannot open input ET system %s iret: %d\n", argv[1], iret);
      printf("et_online_monitor: check with daq if ET system %s is alive\n", argv[1]);
      exit(1);
    }
  et_open_config_destroy(et_in_config);


  /* define a station */
  et_station_config_init(&sconfig);
  et_station_config_setuser(sconfig, ET_STATION_USER_MULTI);
  et_station_config_setrestore(sconfig, ET_STATION_RESTORE_OUT);
  et_station_config_setcue(sconfig, 10);


  /* Make all stations non blocking */
  et_station_config_setblock(sconfig, ET_STATION_NONBLOCKING);

  /* load the shared lib which contains the selection functions */
  if (et_station_config_setlib(sconfig, "libevent_selector.so") == ET_ERROR)
    {
      printf("%s: Error setting library libevent_selector.so\n", argv[0]);
      exit(1);
    }

  et_station_config_setselect(sconfig, ET_STATION_SELECT_USER);

  if (!strcmp(station, "MONITOR"))
    {
      if (et_station_config_setfunction(sconfig, "SelectData") == ET_ERROR)
        {
          printf("%s: Error setting select function SelectData\n", argv[0]);
          exit(1);
        }
    }
  else if (!strcmp(station, "SCALER"))
    {
      if (et_station_config_setfunction(sconfig, "SelectScaler") == ET_ERROR)
        {
          printf("%s: Error setting select function SelectScaler\n", argv[0]);
          exit(1);
        }
    }
  else
    {
      if (et_station_config_setfunction(sconfig, "SelectPPG") == ET_ERROR)
        {
          printf("%s: Error setting select function SelectData\n", argv[0]);
          exit(1);
        }
    }


  /* create a station */
  if ((status = et_station_create(et_in_id, &in_stat, station, sconfig)) < ET_OK)
    {
      switch (status)
        {
        case ET_ERROR_EXISTS:
          printf("%s: station already exists, don't worry I'll just attach to it\n", argv[0]);
          break;

        case ET_ERROR_TOOMANY:
          printf("%s: too many stations created\n", argv[0]);
          exit(1);

        case ET_ERROR_REMOTE:
          printf("%s: memory or improper arg problems\n", argv[0]);
          exit(1);

        case ET_ERROR_READ:
          printf("%s: network reading problem\n", argv[0]);
          exit(1);

        case ET_ERROR_WRITE:
          printf("%s: network writing problem\n", argv[0]);
          exit(1);

        default:
          printf("%s: error in station creation\n", argv[0]);
          exit(1);
        }
    }
  et_station_config_destroy(sconfig);

  /* create an attachment */
  if (et_station_attach(et_in_id, in_stat, &in_att) < 0)
    {
      printf("%s: error in station attach for ET system %s\n", argv[0], argv[1]);
      printf("%s: check with daq if ET system %s is well\n", argv[0], argv[1]);
      exit(1);
    }


  /* output et pool for good data */

  printf("%s: trying to connect to ET system %s\n", argv[0], argv[2]);
  et_open_config_init(&et_good_out_config);
  et_open_config_sethost(et_good_out_config, ET_HOST_ANYWHERE);
  if (et_open(&et_good_out_id, argv[2], et_good_out_config ) != ET_OK)
    {
      printf("%s: et_open problems connecting to ET system %s\n", argv[0], argv[2]);
      printf("%s: check if ET system %s is alive\n", argv[0], argv[2]);
      exit(1);
    }
  et_open_config_destroy(et_good_out_config);

  /* attach to grandcentral station */
  if (et_station_attach(et_good_out_id, ET_GRANDCENTRAL, &attach1) < 0)
    {
      printf("%s: error in et_station_attach for ET system %s\n", argv[0], argv[2]);
      printf("%s: check if ET system %s is okay (you probably have to restart it)\n", argv[0], argv[2]);
      exit(1);
    }
  else
    {
      printf("%s: successfully attached to Grand Central of ET system %s, so far so good\n", argv[0], argv[2]);
    }

  signal(SIGKILL, sig_handler);
  signal(SIGTERM, sig_handler);
  signal(SIGINT, sig_handler);

  neventstotal = 0;
  gettimeofday(&t1, NULL);

  printf("%s is up and running\n\n", argv[0]);
  /* FOREVER */
  while (1)
    {
      nanosleep(&tsleep, NULL);
      err = et_event_get(et_in_id, in_att, &ev, ET_SLEEP, NULL);
      if (err < ET_OK)
        {
          printf("%s: error calling et_event_get from input ET system %s\n", argv[0], argv[1]);
          printf("%s: check with daq if ET system %s is alive\n", argv[0], argv[1]);
          exit(1);
        }

      et_event_getdata(ev, (void **) &pdata);
      et_event_getlength(ev, &len);
      //    et_event_getcontrol(ev,ctrl);  // get control array for this event
      // printf("event ctrl: %x %x, len: %d\n",ctrl[0],ctrl[1],len);
      /*
        et_event_getpriority(ev,&prio);
        et_event_getcontrol(ev,control);

        if (prio == ET_HIGH)
        {
        printf("high prio for evt # %d\n",neventstotal);
        }
        else
        {
        //            printf("low prio for evt # %d\n",neventstotal);
        }
      */

      /*
        {
        int select[ET_STATION_SELECT_INTS], icnt;
        printf("select: %d %d %d %d\n",select[0],select[1],select[2],select[3]);
        }
      */
      /* No Filtering of bad events for now
	 if ( (daqret = daq_monitor((Event *) pdata)) )
	 {
	 et_out_id = et_good_out_id;
	 attach = attach1;
	 }
	 else
	 {
	 et_out_id = et_bad_out_id;
	 attach = attach2;
	 }
      */

      et_out_id = et_good_out_id;
      attach = attach1;
      status = et_event_new(et_out_id, attach, &pe, ET_SLEEP, NULL, len);
      if (status < ET_OK)
        {
          printf("%s: error calling et_event_new for an output ET pool\n", argv[0]);
          /*
	    switch (daqret)
	    {
	    case DAQMON_OK:
	    printf("%s: Check if et pool %s is alive\n", argv[0], argv[2]);
	    break;
	    case DAQMON_FAIL:
	    printf("%s: Check if et pool %s is alive\n", argv[0], argv[3]);
	    break;
	    default:
	    printf("%s: NOT GOOD, Please mail to pinkenburg@bnl.gov, code 01, status: %d, daqmon: %d \n", argv[0], status, daqret);
	    break;
	    }
          */
          exit(1);
        }
      et_event_getdata(pe, (void **) &outdata);

      memcpy( (void *) outdata, (void *) pdata, len);
      /* put event back in input et pool, if done before memcpy, access from
         remote machine coredumps */
      status = et_event_put(et_in_id, in_att, ev);
      if (status < ET_OK)
        {
          printf("%s: error calling et_event_put for input ET pool %s, status %d\n", argv[0], argv[1], status);
          printf("%s: check with daq if ET system %s is alive\n", argv[0], argv[1]);
          exit(1);
        }
      et_event_setlength(pe, len);

      status = et_event_put(et_out_id, attach, pe);
      if (status < ET_OK)
        {
          printf("%s: error calling et_event_put for an output ET pool status %d\n", argv[0], status);
          /*
	    switch (daqret)
	    {
	    case DAQMON_OK:
	    printf("%s: Check if et pool %s is alive\n", argv[0], argv[2]);
	    break;
	    case DAQMON_FAIL:
	    printf("%s: Check if et pool %s is alive\n", argv[0], argv[3]);
	    break;
	    default:
	    printf("%s: NOT GOOD, Please inform pinkenburg@bnl.gov, code 01, status: %d, daqmon: %d \n", argv[0], status, daqret);
	    break;
	    }
          */
          exit(1);
        }

      neventstotal ++;

    }

}

#if defined(SunOS) || defined(Linux)
void sig_handler(int i)
#else
  void sig_handler(...)
#endif
{
  printf("sig_handler: signal seen \n");
  cleanup(0);
}

void cleanup( const int exitstatus)
{

  int err;

  printf("sig_handler: cleaning up \n");
  err = et_station_detach(et_in_id, in_att);
  if (err != ET_OK)
    {
      printf("et_online_monitor: error detaching from station %d of input ET system, error code %d\n", in_stat, err);
    }
  else
    {
      printf("et_online_monitor: detached from stat %d of input ET system\n", in_stat);
      err = et_station_remove(et_in_id, in_stat);
      if (err == ET_OK)
        {
          printf("et_online_monitor: removed stat %d from input ET system\n", in_stat);
        }
      else
        {
          printf("et_online_monitor: error removing stat %d from input ET system error code: %d)\n", in_stat, err);
        }
    }

  err = et_close(et_in_id);
  if (err == ET_OK)
    {
      printf("et_online_monitor: connection to input ET system closed\n");
    }
  else
    {
      printf("et_online_monitor: error closing connection to input ET system, error code %d\n", err);
    }

  err = et_station_detach(et_good_out_id, attach1);
  if (err == ET_OK)
    {
      printf("et_online_monitor: detached from stat %d of 1st output ET system\n", attach1);
    }
  else
    {
      printf("et_online_monitor: error detaching from station %d of 1st output ET system, error code %d\n", attach1, err);
    }

  err = et_close(et_good_out_id);
  if (err == ET_OK)
    {
      printf("et_online_monitor: connection to 1st output ET system closed\n");
    }
  else
    {
      printf("et_online_monitor: error closing connection to 1st output ET system, error code %d\n", err);
    }
  /*
    err = et_station_detach(et_bad_out_id, attach2);
    if (err == ET_OK)
    printf("et_online_monitor: detached from stat %d of 2nd output ET system\n", attach2);
    else
    {
    printf("et_online_monitor: error detaching from station %d of 2nd output ET system, error code %d\n", attach2, err);
    }

    err = et_close(et_bad_out_id);
    if (err == ET_OK)
    printf("et_online_monitor: connection to 2nd output ET system closed\n");
    else
    {
    printf("et_online_monitor: error closing connection to 2nd output ET system, error code %d\n", err);
    }
  */
  exit(exitstatus);
}

int daq_monitor(Event *ev)
{
  /*
    static int i = 0;
    i++;
    //  printf("daq_monitor: event no: %d\n",i);
    if (i == 100)
    {
    i = 0;
    return(DAQMON_FAIL);
    }
  */ 
  return (DAQMON_OK);
}
