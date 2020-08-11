 
#include <iostream.h>
#include <stdlib.h>
#include <unistd.h>
#include <ddEventiterator.h>
#include <ddEventiterator.h>
#include <dd_factory.h>
#include <EventTypes.h>
#include <oEvent.h>
#include <A_Event.h>
#include <stdio.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#define DDEVENTITERATOR 1
#define FILEEVENTITERATOR 2
#define TESTEVENTITERATOR 3
#define DDPOOL 4
#define DFILE 5
#define DNULL 6



void exitmsg()
{
  cout << "** usage: addSpecialEvent -i run_number type dd_pool" << endl;
  cout << "          addSpecialEvent -h for more help" << endl;
  exit(0);
}

void exithelp()
{
  cout <<  endl;
  cout << " -h this message" << endl << endl;
  exit(0);
}
char dd_config_file[256];
char *dd_config_dir;

DD *my_dd ;
DD_factory *pDDFactory; 

int insertinPool (Event *evt, DD *dd);
//int writetofile(Event *evt, oBuffer *ob);

// The global pointer to the Eventiterator (we must be able to 
// get at it in the signal handler) 
Eventiterator *it;



int 
main(int argc, char *argv[])
{
  int c;
  int status;

  int sourcetype =DFILE;
  int destinationtype = DDPOOL;
  int waitinterval = 0;
  int verbose = 0;
  int identify = 0;
  int maxevents = 0;
  int eventnr = 0;
  int gzipcompress = 0;


  extern char *optarg;
  extern int optind;


  while ((c = getopt(argc, argv, "vih")) != EOF)
    {
      switch (c) 
	{

	case 'v':   // verbose
	  verbose = 1;
	  break;

	case 'i':   // identify
	  identify = 1;
	  break;

	case 'h':
	  exithelp();
	  break;

	default:
	  break;
						
	}
    }

  // get run number
  int irun;
  int etype;
  if (argc < optind+2 ) exitmsg();

  cout << argv[optind  ] << endl;
  cout << argv[optind+1] << endl;
  cout << argv[optind+2] << endl;

  sscanf (argv[optind], "%d", &irun);

  if ( strncasecmp(argv[optind+1],"b",1) == 0) etype = BEGRUNEVENT;
  else if  ( strncasecmp(argv[optind+1],"e",1) ==0 ) etype = ENDRUNEVENT;
  else if (  strncasecmp(argv[optind+1],"i",1) == 0)  etype = RUNINFOEVENT;
  else
    {
      sscanf (argv[optind+1],"%d",&etype);
    }

  PHDWORD ebuf[300];

  oEvent *e = new oEvent(ebuf,300, irun, etype, 0);

  Event *evt = new A_Event(ebuf);

  if (identify) evt->identify();


  dd_config_dir = getenv("DD_DATA");
  sprintf( dd_config_file, "%s/%s", dd_config_dir, "dd_config_small.txt");
  pDDFactory = new DD_factory();
  my_dd = pDDFactory->create_dd( argv[optind+2], "w", dd_config_file );
  if ( my_dd->get_status() != DD_STATUS_OK ) 
    {
      cout << "error in creating DD system "  << endl;
      exit(1);
    }
  my_dd->create_fifo("INPUT");

	
  status = insertinPool(evt,my_dd);

  delete evt;


  return 0;
}
	
int insertinPool (Event *evt, DD *dd)
{
  Fifo_entry fev;
  int iw;
  int len = evt->getEvtLength();
  int status = dd->req_fev(len, fev);
  if (status != ddcSuccess)
    {
      cout << "req_fev returns " << status << endl;
      return -1;
    }
  else 
    {
      evt->Copy(fev.get_pData(), len, &iw);
      fev.set_len (len);
      Fifo_entry::fifo_ctl ctl = { evt->getEvtType(),  0xffffffff, 3 , 0};
      fev.set_ctl( ctl );
	      
      status = my_dd->put_fev(fev);
      if ( status != ddcSuccess )
	{
	  cout << "put_fev returns " << status << endl;
	  return -2;
	}
    }
  return 0;
}



  
