/**

this is et_send_data, as realistic as possible, data as they would
come from the ATP's. It reads a buffer worth of data from an existing data file.
Then it transmits the data over and over again to a ET_server. 

The syntax is 

et_event_generator -p port -n <number of buffers> et_name host_name filename

*/



#include <cstdlib>
#include <cerrno>
#include <string>
#include <iostream>
#include <iomanip>

#include <netdb.h>
#include <unistd.h>


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


#include "oBuffer.h"
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

void exitmsg()
{
  std::cout << "** usage: et_send_data  -n numberofevents -w waitinseconds -p portnumber datafile hostname" << std::endl;
  exit(0);
}


int main( int argc, char* argv[])
{
  int the_port;
  int number_of_buffers;
  int controlword, i;
  int status;
  char c;
  char *host_name = NULL;
  char *file_name = NULL;
  

  number_of_buffers = 1;
  the_port=5900;
  int waitinterval = 0;
  int identifyflag = 0;

  while ((c = getopt(argc, argv, "iw:n:p:")) != EOF)
    {
      switch (c) 
	{
	case 'n':
	  sscanf (optarg,"%d", &number_of_buffers);
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

  PHDWORD  *buffer;
  oBuffer *ob;
  int fd = 0;
  int buffer_size = 256*1024*32;  // makes 32MB (specifies how many dwords, so *4)

  buffer = new PHDWORD[buffer_size];


  int sockfd;
  struct sockaddr_in server_addr;
  struct hostent *p_host;
  p_host = gethostbyname(host_name);
  std::cout << p_host->h_name << std::endl;

  bzero( (char*) &server_addr, sizeof(server_addr) );
  server_addr.sin_family = AF_INET;
  bcopy(p_host->h_addr, &(server_addr.sin_addr.s_addr), p_host->h_length);
  server_addr.sin_port = htons(the_port);


  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0) ) < 0 )
    {
      std::cout << "error in socket" << std::endl;
      exit(1);
    }

  int xs = 512*1024;
  
  int s = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF,
		     &xs, 4);
  if (s) std::cout << "setsockopt status = " << s << std::endl;

  if ( connect(sockfd, (struct sockaddr*) &server_addr
	       , sizeof(server_addr)) < 0 ) 
    {
      std::cout << "error in connect" << std::endl;
      exit(1);
    }

  // read the first event

  sleep(15);

  evt = testIt->getNextEvent();

  //now send a begin run instruction
  controlword = htonl(CTRL_BEGINRUN);
  int run_number = htonl(evt->getRunNumber());
  writen (sockfd,(char *)  &controlword, 4);
  writen (sockfd,(char *) &run_number, 4);
  readn (sockfd, (char *) &i, 4);
  //std::cout << "begrun Acknowledge: " << ntohl(i) << std::endl;

  int n = 0;


  while (evt)
    {
      if (identifyflag) evt->identify();

      ob = new oBuffer(fd, buffer, buffer_size);
      ob->addEvent(evt);
      delete evt;
 
      //      std::cout << "sending data out..." << std::endl;
      controlword = htonl(CTRL_DATA);
      writen (sockfd,(char *)  &controlword, 4);
      i = htonl(buffer[0]);
      writen (sockfd,(char *) &i, 4);
      writen (sockfd,(char *) buffer, buffer[0]);
      
      // std::cout << "waiting for acknowledge...   " << std::endl ;
      readn (sockfd, (char *) &i, 4);

      status = ntohl(i);
      std::cout << "AML ack: " << status  << std::endl;
      if ( (status & 0xffff) == CTRL_REMOTEFAIL )
	{
	  std::cout << "AML reports failure at buffer " << n << ", ending..." << std::endl;
	  break;
	}


     

      // std::cout << "... done" << std::endl;
      delete ob;
      if ( ++n >= number_of_buffers)
	{
	  evt = 0;
	}
      else
	{
	  evt = testIt->getNextEvent();
	}
      if (waitinterval) sleep(waitinterval);
    }

  std::cout << "ending run..." << std::endl;
  controlword = htonl(CTRL_ENDRUN);
  writen (sockfd, (char *)&controlword, 4);

  std::cout << "closing..." << std::endl;
  controlword = htonl(CTRL_CLOSE);
  writen (sockfd, (char *)&controlword, 4);
  close (sockfd);
  
  return 0;
}


int readn (int fd, char *ptr, int nbytes)
{
  int nleft, nread;
  nleft = nbytes;
  while ( nleft>0 )
    {
      nread = read (fd, ptr, nleft);
      if ( nread < 0 ) 
	return nread;
      else if (nread == 0) 
	break;
      nleft -= nread;
      ptr += nread;
    }
  return (nbytes-nleft);
}


int writen (int fd, char *ptr, int nbytes)
{
  int nleft, nwritten;
  nleft = nbytes;
  while ( nleft>0 )
    {
      nwritten = write (fd, ptr, nleft);
      if ( nwritten < 0 ) 
	return nwritten;

      nleft -= nwritten;
      ptr += nwritten;
    }
  return (nbytes-nleft);
}


