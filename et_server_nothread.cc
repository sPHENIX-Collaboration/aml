
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>
#include <csignal>
#include <dlfcn.h>


#include <pthread.h>

#ifdef HAVE_BSTRING_H
#include <bstring.h>
#else
#include <strings.h>
#endif


#include <sys/socket.h>

#ifdef SunOS
#include <sys/filio.h>
#endif

#include <sys/stat.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/ioctl.h>

#include <iostream>
#include <iomanip>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "et.h"

#include <EventTagger.h>
#include <BufferConstants.h>
#include <A_Event.h>

#include <ogzBuffer.h>
#include <buffer.h>
#include <et_control.h>

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

#if defined(SunOS) || defined(Linux) 
void sig_handler(int);
void et_control_handler(int);
#else
void sig_handler(...);
void et_control_handler(...);
#endif

void cleanup(const int exitstatus);



void * disassemble_buffer(void * arg);
//static void * signal_thread (void *arg);
int handle_this_connection ();

int sockfd = 0;
int dd_fd = 0;

PHDWORD *bp0;
PHDWORD *bp1;
PHDWORD *bp_being_received;
PHDWORD *bp_being_sent;


PHDWORD *bp;
EventTagger *ETagger;

msg_control *msgCtrl;

pthread_t ThreadId;
pthread_t       tid;
unsigned long long  *thread_arg[5];

int current_index;

int events_in_run;
int event_scaler = 0;

et_control *nd = 0;
int writeflag;

int the_port = 5001;


#include <msg_control.h>
//#include <filter_msg_buffer.h>

//filter_msg_buffer filter(256);


void exitmsg()
{
  std::cout << "** This is the et_server (non-threaded version). No gimmicks. Pure Power." << std::endl;
  std::cout << "** usage: et_server_nothread  [-t token] et_name port-number" << std::endl;
  exit(0);
}


void et_register(EventTagger *T, msg_control **msg)
{
  if (ETagger) delete ETagger;
  ETagger = T;
  *msg = msgCtrl;
}


void et_unregister(EventTagger *T)
{
  if (T==ETagger)
    {
      delete ETagger;
      ETagger = 0;
    }
}


int current_buffersize0;
int current_buffersize1;
int current_buffersize;
int current_buffersize_sent;


et_att_id	  attach1;
et_sys_id       id;
et_openconfig   openconfig;
et_event       *pe;

using namespace std;

int main( int argc, char* argv[])
{

  msgCtrl = new msg_control(MSG_TYPE_ONLINE,  MSG_SOURCE_ET,
			    MSG_SEV_DEFAULT, "ET_server");


  std::cout << *msgCtrl << "this is et_server" << std::endl;
  

  int status;
  const char *et_name;
  int i;


#if defined(SunOS) || defined(Linux) 
  struct sockaddr client_addr;
#else
  struct sockaddr_in client_addr;
#endif
  struct sockaddr_in server_addr;

#if defined(SunOS)
  int client_addr_len = sizeof(client_addr);
#else
  unsigned int client_addr_len = sizeof(client_addr);
#endif


  ThreadId = 0;

  // ----------------------------
  // assemble a convenient token

  char token[L_cuserid+128];
  char c;
  int token_set = 0;
  strcpy(token,"/tmp/et_");

  while ((c = getopt(argc, argv, "t:")) != EOF)
    {
      switch (c) 
	{
	case 't':
	  strcat (token,optarg);
	  token_set = 1;
	  break;
	}
    }


  //  ------------------------------
  // process the command_line parameters and set up the et_control object

  if (argc <= optind+1) exitmsg();

  if (argc >= optind+2)  et_name = argv[optind];
  else et_name = "ONLINE";

  //et_name = "ONLINE";

  nd = new et_control (status, et_name , token );
  if (status) 
    {
      cleanup(1);
    }
  sscanf (argv[optind+1],"%d", &the_port);
  nd->set_port(the_port);

  std::cout << *msgCtrl  << "port is " << the_port << std::endl;

  /* spawn signal handling thread */
  //  pthread_create(&tid, NULL, signal_thread, (void *)NULL);
  

  // initially, we don't have a Tagger
  ETagger = 0;
  if (getenv("EVENTTAGGER") != NULL)
    {
      void *s = dlopen(getenv("EVENTTAGGER"), RTLD_GLOBAL | RTLD_LAZY);
      if (!s) 
	{
	  msgCtrl->set_severity(MSG_SEV_ERROR);
	  std::cout << *msgCtrl  << "Loading of the Event Tagger shared library failed" << dlerror()<< std::endl;
	  msgCtrl->reset_severity();

	  ETagger = 0;
	}
    }



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
  et_open_config_sethost(openconfig,ET_HOST_LOCAL);

  if (et_open(&id, et_name, openconfig) != ET_OK) {
    printf("%s: et_open problems\n", argv[0]);
    exit(1);
  }
  et_open_config_destroy(openconfig);

 
  /* set level of debug output (everything) */
  et_system_setdebug(id, ET_DEBUG_INFO);
  
  /* attach to grandcentral station */
  if (et_station_attach(id, ET_GRANDCENTRAL, &attach1) < 0) {
    printf("%s: error in et_station_attach\n", argv[0]);
    exit(1);
  }
  else
    printf("attached to Grand Central\n");

  
  signal(SIGKILL, sig_handler);
  signal(SIGTERM, sig_handler);
  signal(SIGINT, sig_handler);

  signal(ET_CONTROL_SIGNAL, et_control_handler);
  

  current_buffersize0 =BUFFERSIZE;
  current_buffersize1 =BUFFERSIZE;


  bp0 = new PHDWORD[current_buffersize0];
  bp1 = new PHDWORD[current_buffersize1];
  bp_being_received = bp0;
  bp_being_sent = bp1;
  current_buffersize = current_buffersize0;
  current_buffersize_sent = current_buffersize1;


  // ------------------------
  // now set up the sockets

  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) 
    {
      msgCtrl->set_severity(MSG_SEV_ERROR);
      std::cout << *msgCtrl  << "cannot create socket" << std::endl;
      msgCtrl->reset_severity();

      cleanup(1);
    }
  int xs = 64*1024;

  int s = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF,
		     &xs, 4);
  if (s) std::cout << *msgCtrl  << "setsockopt status = " << s << std::endl;


  bzero( (char*)&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(nd->get_port());


  int retries = 0;
  while ( (i = bind(sockfd, (struct sockaddr*) &server_addr, 
		    sizeof(server_addr))) < 0 )
    {

      std::cout << *msgCtrl  << "tried port " <<   nd->get_port() << std::endl;
      nd->set_port( nd->get_port() +1 );
      server_addr.sin_port = htons(nd->get_port());
      if (retries++ > 100) cleanup(1);

    }

  std::cout << *msgCtrl  << "bind complete on port " << nd->get_port() << std::endl;

  // ------------------------
  // now the endless loop when we open and close connections

  listen(sockfd, 1);
      
  while (sockfd > 0)
    {
			
      dd_fd = accept(sockfd, (struct sockaddr*) &client_addr, &client_addr_len);
      if ( dd_fd < 0 ) 
	{
	  msgCtrl->set_severity(MSG_SEV_ERROR);
	  std::cout << *msgCtrl  << "error in accept socket" << std::endl;
	  msgCtrl->reset_severity();
	  
	  cleanup(1);
	  delete nd;
	  exit(1);
	}

      // should be the default, but set it to blocking mode anyway
      i = ioctl (dd_fd,  FIONBIO, 0);

      // find out where we were contacted from

#if defined (SunOS) || defined(Linux)
      struct hostent *hp_new = 0;
      //	gethostbyaddr((char *)&client_addr.sa_data,4 , AF_INET);
#else
      struct hostent *hp_new = 
	gethostbyaddr(&client_addr.sin_addr, client_addr_len, AF_INET);
#endif

      if (hp_new) std::cout << *msgCtrl  << "new connection accepted from " <<  hp_new->h_name << std::endl;
      else 
	{
	  // std::cout << *msgCtrl  << "error in gethostbyaddr" << std::endl;
	  std::cout << *msgCtrl  << "new connection accepted" << std::endl;
	}

      status = handle_this_connection();
      
      if (status == -1 || status == 1)
	{
	  if (dd_fd > 0) close (dd_fd);
	  std::cout << *msgCtrl  << "Socket closed" <<   std::endl;
	  nd->set_connection_open(0);
	}
      else
	{
      

	  msgCtrl->set_severity(MSG_SEV_WARNING);
	  std::cout << *msgCtrl  << "Strange status return from connection: " << status << std::endl;
	  msgCtrl->reset_severity();
	  
	}

    }
  return 0;
}


int handle_this_connection ()
{
  int go_on, len, status;
  int controlword;
  int run_number;

  int i;
  
  nd->set_connection_open(1);
  go_on = 1;

  int ievent = 0;
  PHDWORD *p = &bp[0];
  // we look at go_on and if the pool is still attached, just to be sure
  while ( go_on  )
    {
      
      // we always wait for a controlword which tells us what to do next.
      if ( (status = readn (dd_fd, (char *) &controlword, 4) ) < 0)
	{
	  std::cout << *msgCtrl  << "readn status = " << status << std::endl;
	  controlword = 0;
	  return -1;
	}
      controlword = ntohl(controlword);
      
      //	  std::cout << *msgCtrl  << " controlword = " << controlword << std::endl;
      
      switch (controlword) 
	{
	  // ------------------------
	  // Begin run
	case  CTRL_BEGINRUN:
	  readn (dd_fd, (char *) &run_number, 4);
	  
	  run_number = ntohl(run_number);
	  std::cout << *msgCtrl  << " new run = " << run_number << std::endl;
	  events_in_run = 0;
	  
	  i = CTRL_REMOTESUCCESS;

	  i = htonl(i);

	  writen (dd_fd, (char *)&i, 4);
	  break;

	  // ------------------------
	  // end run
	case  CTRL_ENDRUN:

	  if (ThreadId) 
	    {
	      // std::cout << "1 -- pthread_join" << std::endl;
	      pthread_join(ThreadId, NULL);
	      events_in_run +=  (unsigned long long) thread_arg[3];
	      // std::cout << "done - events now " << events_in_run << std::endl;
	    }
	  std::cout << *msgCtrl  << " end run -- number of events " <<events_in_run << std::endl;
	      
	  i = htonl(CTRL_REMOTESUCCESS);
	  writen (dd_fd, (char *)&i, 4);
	  break;

	  // ------------------------
	  // close connection
	case CTRL_CLOSE:
	  std::cout << *msgCtrl  << "Closing connection..." << std::endl;
	  return 1;
	  break;

	  // ------------------------
	  // receive data
	case  CTRL_DATA:
	  ievent++;
	  readn (dd_fd, (char *) &len, 4);
	  len = ntohl(len);

	  if ( len/4 > current_buffersize)
	    { 
	      delete [] bp_being_received;
	      current_buffersize = len/4 + 2048;
	      bp_being_received = new PHDWORD[ current_buffersize];
	    }

	  p = &bp_being_received[0];
	  readn (dd_fd, (char *) p,   len );

	  //	      if (ThreadId) 
	  //	{
	  //	  std::cout << "1 -- pthread_join" << std::endl;
	  //	  pthread_join(ThreadId, NULL);
	  //	  events_in_run += (int) thread_arg[3];
	  //	  std::cout << "done - events now " << events_in_run << std::endl;
	  //	}
	  //swap the buffer pointers -- dont remove the {}'s
	  {
	    PHDWORD *tmp = bp_being_received;
	    bp_being_received = bp_being_sent;
	    bp_being_sent = tmp;
	    int xx = current_buffersize;
	    current_buffersize =  current_buffersize_sent;
	    current_buffersize_sent = xx;

	  }

	  thread_arg[0] = (unsigned long long* )  bp_being_sent;
	  thread_arg[1] = ( unsigned long long*)  &id;
	  thread_arg[2] = (unsigned long long*)  &attach1;
	  thread_arg[3] = 0;
	  thread_arg[4] = 0;


	  disassemble_buffer (thread_arg);
	  events_in_run += (unsigned long long)thread_arg[3];

	  i = htonl(CTRL_REMOTESUCCESS);
	  writen (dd_fd, (char *)&i, 4);



	  break;

	default:
	  break;
	}
    }
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


#if defined(SunOS) || defined(Linux) 
void sig_handler(int i)
#else
  void sig_handler(...)
#endif
{
  std::cout << *msgCtrl  << "sig_handler: signal seen " << std::endl;
  cleanup(0);
}


#if defined(SunOS) || defined(Linux) 
void et_control_handler(int i)
#else
  void et_control_handler(...)
#endif
{
  //  std::cout << "ndd_control_handler: signal seen " << std::endl;

  nd->confirm_alive();
  signal(ET_CONTROL_SIGNAL, et_control_handler);

}

void * disassemble_buffer(void *arg)
{

  // arguments: 
  //  [0] pointer to data buffer
  //  [1] et "id"
  //  [2] et "attach1"
  //  [3] to return the number of events sent out
  //  [4] to return the status of ET

  unsigned long long  *intarg = (unsigned long long  *) arg;
  
  et_att_id	  *attach1;
  et_sys_id       *id;
  et_event       *pe;

  int *evtData;
  int len;
  int status = 0;

  int iev = 0;
  char *pdata;

  PHDWORD *bp = (PHDWORD *) intarg[0];
  id = (et_sys_id *) intarg[1];
  attach1 = (et_att_id *) intarg[2];

  buffer *bptr = new buffer( bp, *bp/4);

  while ( (evtData =  bptr->getEventData()) )
    {
      iev++;
      len = 4*evtData[0];
      // std::cout << "Event " << evtData[2] << std::endl;
      // std::cout << "getting new event space" << std::endl;
      status = et_event_new(*id, *attach1, &pe, ET_SLEEP, NULL, len);
      // std::cout << "1" << std::endl;

      if (status != ET_OK)
	{
	  // std::cout << "7" << std::endl;
	  delete bptr;
	  intarg[3] = 0;
	  intarg[4] = status;
	  return NULL;
	}
      // std::cout << "2" << std::endl;
      et_event_getdata(pe, (void **) &pdata);
      // std::cout << "3" << std::endl;

      memcpy((void *)pdata, (const void *) evtData, len);

      et_event_setlength(pe, len);      
      status = et_event_put(*id, *attach1, pe);
      // std::cout << "4" << std::endl;

      if (status != ET_OK) 
	{
	  // std::cout << "5" << std::endl;
	  delete bptr;
	  intarg[3] = iev-1; //subtract this event
	  intarg[4] = status;
	  return NULL;
	}

    }

  // std::cout << "ending thread, " << iev << " events" << std::endl;
  delete bptr;
  intarg[3] = iev;
  intarg[4] = status;
  return NULL;
}


void cleanup( const int exitstatus)
{
  if (nd) delete nd;
  close (sockfd);
  close (dd_fd);

  exit(exitstatus);
}
    
/************************************************************/
/*              separate thread to handle signals           */
/*
static void * signal_thread (void *arg)
{
sigset_t   signal_set;
int        sig_number;
 
sigemptyset(&signal_set);
sigaddset(&signal_set, SIGINT);
  
// Not necessary to clean up as ET system will do it
sigwait(&signal_set, &sig_number);
printf("Got a control-C, exiting\n");
exit(1);
}
*/
