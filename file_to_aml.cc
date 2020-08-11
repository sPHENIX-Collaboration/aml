/**

*/



#include <cerrno>
#include <cstdlib>

#include <iomanip>
#include <iostream>
#include <string>

#include <unistd.h>
#include <netdb.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>



#ifdef HAVE_BSTRING_H
#include <bstring.h>
#else
#include <strings.h>
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif


#include "oamlBuffer.h"
#include "Event.h"
#include "testEventiterator.h"
#include "fileEventiterator.h"

#define CTRL_BEGINRUN 1
#define CTRL_ENDRUN   2
#define CTRL_DATA     3
#define CTRL_CLOSE    4

#define CTRL_REMOTESUCCESS 100
#define CTRL_REMOTEFAIL    101

#define TRUE 1
#define FALSE 0

int readn(int , char *, int);
int writen(int , char *, int);

  //16 MB buffer (this is in dwords)
#define LENGTH (4*1024*1024)

PHDWORD xb[LENGTH];



void exitmsg()
{
  std::cout << "** usage: file_to_aml  -n numberofevents -w waitinseconds -p portnumber datafile hostname" << std::endl;
  exit(0);
}


int main( int argc, char* argv[])
{
  int the_port;
  int number_of_buffers;
  int number_of_events;
  //  int controlword, i;
  int status;
  char c;
  char *host_name = 0;
  char *file_name = 0;
  

  number_of_buffers = 1;
  number_of_events = 1;
  the_port=5900;
  int waitinterval = 0;
  int identifyflag = 0;

  while ((c = getopt(argc, argv, "iw:n:p:")) != EOF)
    {
      switch (c) 
	{

	case 'n':
	  sscanf (optarg,"%d", &number_of_events);
	  break;

	case 'p':
	  sscanf (optarg,"%d", &the_port);
	  break;
	case 'w':
	  sscanf (optarg,"%d", &waitinterval);
	  break;
	case 'i':
	  identifyflag =1;
	  break;
	}
    }

  if (argc >= optind+2)  
    {
      
      file_name = argv[optind];
      host_name = argv[optind+1];
    }
  else exitmsg();

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



  oBuffer *ob = new oamlBuffer ( host_name, the_port, xb, LENGTH);

  int count = 0;
  while ( (evt = testIt->getNextEvent()) && ( !number_of_events || ( count <  number_of_events )) )
    {
      if ( identifyflag ) evt->identify();
      ob->addEvent(evt);
      delete evt;
    }
    
  delete ob;

  return 0;
}

