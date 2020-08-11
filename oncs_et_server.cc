
#include <string>
#include <sys/types.h>
#include <ctime>
#include <unistd.h>
#include <cstdlib>
#include <cerrno>
#include <csignal>
#include <dlfcn.h>


#include <pthread.h>

#ifdef HAVE_BSTRING_H
#include <bstring.h>
#else
#include <strings.h>
#endif

#include <ctime>

#include <sys/socket.h>

#ifdef SunOS
#include <sys/filio.h>
#endif

#include <sys/stat.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/ioctl.h>

#include <cstdio>
#include <iostream>
#include <iomanip>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "et.h"

#include <EventTagger.h>
#include <BufferConstants.h>
#include <A_Event.h>

#include <oncsBuffer.h>
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

int fp;

EventTagger *ETagger;

msg_control *msgCtrl;

pthread_t ThreadId;
pthread_t       tid;
unsigned long long *thread_arg[6];

int current_index;

int events_in_run;
int event_scaler = 0;

et_control *nd = 0;
int writeflag;

int the_port = 5001;


#include <msg_control.h>
//#include <filter_msg_buffer.h>

//filter_msg_buffer filter(256);

using namespace std;

void exitmsg()
{
  cout << "** This is the oncs et_server (threaded version). No gimmicks. Pure Power." << endl;
  cout << "** usage: et_server_nothread  [-t token] et_name port-number filerule " << endl;
  exit(0);
}


void et_register(EventTagger *T, msg_control **msg)
{
  if (ETagger) delete ETagger;
  ETagger = T;
  *msg = msgCtrl;
  cout << "Event typer to tag events registered" << endl;

}
 

void et_unregister(EventTagger *T)
{
  if (T==ETagger)
    {
      //      delete ETagger;
      ETagger = 0;
      cout << "Event typer to tag events de-registered" << endl;
    }
}



int write_buffer (int bp[], int fp)
{
  int ip =0;
  char *cp = (char *) bp;

  while (ip<bp[0])
    {
      write ( fp, cp, 8192);
      cp += 8192;
      ip+=8192;
    }
  return 0;
}



int current_buffersize0;
int current_buffersize1;
int current_buffersize;
int current_buffersize_sent;


et_att_id	  attach1;
et_sys_id       id;
et_openconfig   openconfig;
et_event       *pe;

int main( int argc, char* argv[])
{

  msgCtrl = new msg_control(MSG_TYPE_ONLINE,  MSG_SOURCE_ET,
			    MSG_SEV_DEFAULT, "ET_server");


  //  cout << *msgCtrl << "this is et_server" << endl;
  

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


  if (argc >= optind+3)
    {
      writeflag = 1;
      status = nd->set_filerule(argv[optind +2]);
      cout << "File rule is " << nd->get_filerule() << endl;
      nd->enable_logging();
    }
  else
    {
      nd->disable_logging();
      writeflag = 0;
    }



  //  cout << *msgCtrl  << "port is " << the_port << endl;

  /* spawn signal handling thread */
  //  pthread_create(&tid, NULL, signal_thread, (void *)NULL);
  

  // initially, we don't have a Typer
  ETagger = 0;
  if (getenv("EVENTTAGGER") != NULL)
    {
      void *s = dlopen(getenv("EVENTTAGGER"), RTLD_GLOBAL | RTLD_NOW);
      if (!s) 
	{
	  msgCtrl->set_severity(MSG_SEV_ERROR);
	  cout << *msgCtrl  << "Loading of the Event Tagger shared library failed: " << dlerror() << endl;
	  msgCtrl->reset_severity();

	  ETagger = 0;
	}
      else
	{
	  if (ETagger) // see if we have such a thing
	    {
	      
	      cout << "Event Tagger: " << ETagger->idString() << " loaded" << endl;	
	    }
	  
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
      cout << *msgCtrl  << "cannot create socket" << endl;
      msgCtrl->reset_severity();

      cleanup(1);
    }
  //  int xs = 64*1024+21845;
  int xs = 512*1024;
  int old_rdbck,rdbck;
  socklen_t opt_rdbck = sizeof(int);

  getsockopt(sockfd,SOL_SOCKET, SO_RCVBUF,&old_rdbck,
	     &opt_rdbck);
  // printf("getsockopt: opt_rdbck: %d\n",opt_rdbck);
  int s = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF,
		     &xs, 4);
  if (s) cout << *msgCtrl  << "setsockopt status = " << s << endl;

  getsockopt(sockfd,SOL_SOCKET, SO_RCVBUF,&rdbck,
	     &opt_rdbck);
  //  printf("et_server: RCVBUF changed from %d to %d\n",old_rdbck,rdbck);

  getsockopt(sockfd,SOL_SOCKET, SO_SNDBUF,&old_rdbck,
	     &opt_rdbck);
  
  xs = 32768;
  s = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF,
		 &xs, 4);
 
  getsockopt(sockfd,SOL_SOCKET, SO_SNDBUF,&rdbck,
	     &opt_rdbck);

  //  printf("et_server: SNDBUF changed from %d to %d\n",old_rdbck,rdbck);

  bzero( (char*)&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(nd->get_port());


  int retries = 0;
  while ( (i = bind(sockfd, (struct sockaddr*) &server_addr, 
		    sizeof(server_addr))) < 0 )
    {

      cout << *msgCtrl  << "tried port " <<   nd->get_port() << endl;
      nd->set_port( nd->get_port() +1 );
      server_addr.sin_port = htons(nd->get_port());
      if (retries++ > 100) cleanup(1);

    }

  cout << *msgCtrl  << "bind complete on port " << nd->get_port() << endl;

  // ------------------------
  // now the endless loop when we open and close connections

  listen(sockfd, 1);
      
  while (sockfd > 0)
    {
			
      dd_fd = accept(sockfd, (struct sockaddr*) &client_addr, &client_addr_len);
      if ( dd_fd < 0 ) 
	{
	  msgCtrl->set_severity(MSG_SEV_ERROR);
	  cout << *msgCtrl  << "error in accept socket" << endl;
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

      if (hp_new) cout << *msgCtrl  << "new connection accepted from " <<  hp_new->h_name << endl;
      else 
	{
	  // cout << *msgCtrl  << "error in gethostbyaddr" << endl;
	  cout << *msgCtrl  << "new connection accepted" << endl;
	}

      status = handle_this_connection();
      
      if (status == -1 || status == 1)
	{
	  if (dd_fd > 0) close (dd_fd);
	  cout << *msgCtrl  << "Socket closed" <<   endl;
	  nd->set_connection_open(0);
	}
      else
	{
      

	  msgCtrl->set_severity(MSG_SEV_WARNING);
	  cout << *msgCtrl  << "Strange status return from connection: " << status << endl;
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
  int s;
  // we look at go_on and if the pool is still attached, just to be sure
  //  while ( (stat = et_alive(id)) && go_on  ) et_alive returns garbage sometimes!
  while (  go_on  )
    {
      // we always wait for a controlword which tells us what to do next.
      if ( (status = readn (dd_fd, (char *) &controlword, 4) ) < 0)
	{
	  cout << *msgCtrl  << "readn status = " << status << endl;
	  controlword = 0;
	  return -1;
	}
      controlword = ntohl(controlword);
      
      // 	  cout << *msgCtrl  << " controlword = " << controlword << endl;
      
      switch (controlword) 
	{
	  // ------------------------
	  // Begin run
	case  CTRL_BEGINRUN:
	  readn (dd_fd, (char *) &run_number, 4);
	  
	  run_number = ntohl(run_number);
	  //cout << *msgCtrl  << " new run = " << run_number << endl;
	  events_in_run = 0;
	  if (nd->get_logging())
	    {
	      status = nd->open_file(run_number, &fp);
	      if (status) 
		{
		  i = CTRL_REMOTEFAIL;
		}
	      else
		{
		  i = CTRL_REMOTESUCCESS;
		  cout << "opened file " << nd->get_filename() << endl;
		}
	    }
	  else
	    {
	      i = CTRL_REMOTESUCCESS;
	    }
	  i = htonl(i);

	  writen (dd_fd, (char *)&i, 4);
	  break;

	  // ------------------------
	  // end run
	case  CTRL_ENDRUN:

	  if (ThreadId) 
	    {
	      // cout << "1 -- pthread_join" << endl;
	      pthread_join(ThreadId, NULL);
	      ThreadId = 0;
	      events_in_run += (unsigned long long) thread_arg[4];
	      nd->set_current_event(events_in_run);
	      // cout << "done - events now " << events_in_run << endl;
	    }

	  if (nd->get_logging())
	    {
	      nd->close_file();
	      cout << "closed file, events: " << events_in_run << endl;
	    }

	  //cout << *msgCtrl  << " end run -- number of events " <<events_in_run << endl;
	      
	  i = htonl(CTRL_REMOTESUCCESS);
	  writen (dd_fd, (char *)&i, 4);
	  break;

	  // ------------------------
	  // close connection
	case CTRL_CLOSE:
	  //cout << *msgCtrl  << "Closing connection..." << endl;
	  return 1;
	  break;

	  // ------------------------
	  // receive data
	case  CTRL_DATA:
	  ievent++;
	  readn (dd_fd, (char *) &len, 4);
	  len = ntohl(len);
	  //	  cout << "in CTRL_DATA , len = " << len << endl;

	  if ( len/4 > current_buffersize)
	    { 
	      delete [] bp_being_received;
	      current_buffersize = len/4 + 2048;
	      bp_being_received = new PHDWORD[ current_buffersize];
	    }

	  p = &bp_being_received[0];
	  readn (dd_fd, (char *) p,   len );
	  if (ThreadId) 
	    {
	      //cout << "1 -- pthread_join" << endl;
	      pthread_join(ThreadId, NULL);
	      events_in_run += (unsigned long long) thread_arg[4];
	      ThreadId = 0;
	      nd->set_current_event(events_in_run);
	      //      cout << "done - events now " << events_in_run 
	      //	   << "   " << nd->get_current_event()<< endl;
	    }
	  //swap the buffer pointers -- dont remove the {}'s
	  {
	    PHDWORD *tmp = bp_being_received;
	    bp_being_received = bp_being_sent;
	    bp_being_sent = tmp;
	    int xx = current_buffersize;
	    current_buffersize =  current_buffersize_sent;
	    current_buffersize_sent = xx;

	  }

	  thread_arg[0] = (unsigned long long *)  bp_being_sent;
	  thread_arg[1] = (unsigned long long *)  &id;
	  thread_arg[2] = (unsigned long long *)  &attach1;
	  thread_arg[3] = (unsigned long long *)  (len/4);   // the buffer length in bytes/4 -> buffer length in words
	  thread_arg[4] = 0;
	  thread_arg[5] = 0;


	  //cout << "starting thread" << endl;
	  
	  s = pthread_create(&ThreadId, NULL, 
			     disassemble_buffer, 
			     (void *) thread_arg);
	  if (s ) cout << "error in thread create " << s << endl;
	  
	  //cout << "acknowledging... " <<  endl;

          i = CTRL_REMOTESUCCESS;
	  
	  {

	    fd_set rfds;
	    struct timeval tv;
	    int retval;

	    FD_ZERO(&rfds);
	    FD_SET(dd_fd, &rfds);
	    tv.tv_sec = 0;
	    tv.tv_usec = 0;

	    retval = select(dd_fd+1, &rfds, NULL, NULL, &tv);
	    if (retval)
	      {
		int aargh;
		int n = 0;
		while(select(dd_fd+1, &rfds, NULL, NULL, &tv))
		  {
		    read (dd_fd, (char *) &aargh, 1);
		    n++;
              
		  }
		cout << "<E> et_server: Bad length, buffer contained " << n << " too many bytes, flushed them" << endl; 
		i = CTRL_REMOTEFAIL;
	      }
	  }
      
	  i = htonl(i);
	  writen (dd_fd, (char *)&i, 4);
	  //cout << "... done" <<  endl;



	  break;

	default:
	  break;
	}
    }
  //  cout << "returning 0!!" << endl;
  return 0;

}



int readn (int fd, char *ptr, int nbytes)
{
  int nread, nleft;
  //int nleft, nread;
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
  //cout << *msgCtrl  << "sig_handler: signal seen " << endl;
  cleanup(0);
}


#if defined(SunOS) || defined(Linux) 
void et_control_handler(int i)
#else
  void et_control_handler(...)
#endif
{
  //  cout << "ndd_control_handler: signal seen " << endl;

  nd->confirm_alive();
  signal(ET_CONTROL_SIGNAL, et_control_handler);

}

void * disassemble_buffer(void *arg)
{

  // arguments: 
  //  [0] pointer to data buffer
  //  [1] et "id"
  //  [2] et "attach1"
  //  [3] length of that buffer in 32 bit words
  //  [4] to return the number of events sent out
  //  [5] to return the status of ET

  unsigned long long *intarg = (unsigned long long *) arg;
  
  et_att_id	  *attach1;
  et_sys_id       *id;
  et_event       *pe;

  Event * evt;
  int len;
  int status = 0;
  //  int iw;
  int nw;
  int iev = 0;
  char *pdata;

  int *bp = (int *) intarg[0];
  id = (et_sys_id *) intarg[1];
  attach1 = (et_att_id *) intarg[2];

  len = (int) intarg[3];

    

  //  cout << "in disassemble_buffer, (buffer) len = " << len << endl;
  oncsBuffer *bptr = new oncsBuffer( bp, len );
  //cout << "in disassemble_buffer, buffer created" << endl;

  if ( nd->get_logging() )
    {
      status = write_buffer (bp,fp);
      if(status) cout << "write to file failed" << endl;
    } 
  else

    cout << "no output file open" << endl;


  while ( (evt =  bptr->getEvent()) )
    {
      iev++;
      len = 4*evt->getEvtLength();


      //  cout << "Event " << evtData[2] << endl;
      //cout << "getting new event space" << endl;
      //cout << "Event length(in bytes) = " << len<< " in words: "<< evtData[0] << endl;
      status = et_event_new(*id, *attach1, &pe, ET_SLEEP, NULL, len);
      // cout << "1" << endl;

      if (status != ET_OK)
	{
	  // cout << "7" << endl;
	  delete bptr;
	  intarg[4] = 0;
	  intarg[5] = status;
	  delete evt;
	  return NULL;
	}
      // cout << "2" << endl;
      et_event_getdata(pe, (void **) &pdata);
      // cout << "3" << endl;

      evt->Copy( (int *) pdata, evt->getEvtLength(), &nw, "RAW");


      //      if (ETagger) // see if we have such a thing
      //	{
      //	  int con[ET_STATION_SELECT_INTS];
      //	  int nw;
      //	  ETagger->setPattern(evtData, con,&nw);
      //	  et_event_setcontrol(pe, con, nw);

      //	}


      et_event_setlength(pe, len);      

      //cout << "before et_put, len = " << len << endl;
      status = et_event_put(*id, *attach1, pe);
      //cout << "after et_put" << endl;
      // cout << "4" << endl;

      if (status != ET_OK) 
	{
	  // cout << "5" << endl;
	  delete bptr;
	  intarg[4] = iev-1; //subtract this event
	  intarg[5] = status;
	  delete evt;
	  return NULL;
	}

    }

  //   cout << "ending thread, " << iev << " events" << endl;
  delete bptr;
  intarg[4] = iev;
  intarg[5] = status;
  delete evt;
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
