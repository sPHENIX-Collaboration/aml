
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <dlfcn.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#define KEY (1493)


#include <pthread.h>

#ifdef HAVE_BSTRING_H
#include <bstring.h>
#else
#include <strings.h>
#endif

#include <sys/time.h>

#include <sys/socket.h>
#include <netdb.h> 

#include <sys/types.h>

#ifdef SunOS
#include <sys/filio.h>
#endif

#include <sys/stat.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/ioctl.h>

#include <stdio.h>
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

#include <date_filter_msg_buffer.h>


#define CTRL_BEGINRUN 1
#define CTRL_ENDRUN   2
#define CTRL_DATA     3
#define CTRL_CLOSE    4

#define CTRL_REMOTESUCCESS 100
#define CTRL_REMOTEFAIL    101

#define TRUE 1
#define FALSE 0

#define NUMBEROFBUFFERS 128
#define WRITESEM 128
#define ENDRUNSEM 129
#define STARTRUNSEM 130


int run_number = 0;
int verbose = 0;
int sockfd = 0;
int dd_fd = 0;

oBuffer *ob;
FILE *fp;
int fd;

msg_control *msgCtrl;

pthread_mutex_t M_cout;
pthread_mutex_t M_evtctr;
pthread_mutex_t M_runnr;

pthread_mutex_t StartRun;



pthread_t ThreadId;
pthread_t       tid;


int current_index;

int events_in_run;
int event_scaler = 0;

et_control *nd = 0;
int writeflag;

int the_port = 5001;

int RunIsActive = 0; 
int NumberWritten = 0; 
int file_open = 0;

int pid_key ;


typedef struct 
{
  int index;
  int donemark;
  int writtenmark;
  int takenmark;
  int runnumber;
  unsigned int bytecount;
  int ended;
  int error;
  unsigned int buffersize;
  PHDWORD *bf;
} bufferstructure;

bufferstructure buffers[NUMBEROFBUFFERS];

struct thread_arg
{
  int dd_fd;
  bufferstructure *b0;
  bufferstructure *b1;
  int index0;
  int index1;
  char *host;
};



et_att_id	  attach1;
et_sys_id       id;
et_openconfig   openconfig;
et_event       *pe;




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


char token[L_cuserid+128];

void * disassemble_buffer(void * arg);
//static void * signal_thread (void *arg);
void  * handle_this_connection ( void * arg);



//#include <msg_control.h>

//#include <date_filter_msg_buffer.h>

//date_filter_msg_buffer filter(256);


using namespace std;


void exitmsg()
{
  cout << "** This is the Advanced Multithreaded Server. No gimmicks. Pure Power." << endl;
  cout << "** usage: aml [-t token] port-number filerule" << endl;
  cout << "   -d enable database logging" << endl;
  cout << "   -s file size chunks in MB" << endl;
  cout << "   -a file sequence increment" << endl;
  cout << "   -b file sequence start " << endl;
  exit(0);
}


int semid;



int sem_set (const int semnumber, const int value)
{
  union semun 
  {
    int val;                    /* value for SETVAL */
    struct semid_ds *buf;       /* buffer for IPC_STAT, IPC_SET */
    unsigned short int *array;  /* array for GETALL, SETALL */
    struct seminfo *__buf;      /* buffer for IPC_INFO */
  } semset;

  semset.val = value;
  
  return semctl(semid, semnumber, SETVAL, semset);

}


int sem_inc (const int semnumber)
{
  struct sembuf operations[1];

  operations[0].sem_num = semnumber;

  operations[0].sem_op = 1;
  operations[0].sem_flg = 0;
  return semop(semid, operations, 1);

}

int sem_dec (const int semnumber)
{
  struct sembuf operations[1];

  operations[0].sem_num = semnumber;

  operations[0].sem_op = -1;
  operations[0].sem_flg = 0;
  return semop(semid, operations, 1);

}

int sem_lock (const int semnumber)
{
  struct sembuf operations[2];
  operations[0].sem_num = semnumber;
  operations[0].sem_op = 0;
  operations[0].sem_flg = 0;

  operations[1].sem_num = semnumber;
  operations[1].sem_op = 1;
  operations[1].sem_flg = 0;

  return semop(semid, operations, 2);

}

int sem_wait (const int semnumber)
{
  struct sembuf operations[1];

  operations[0].sem_num = semnumber;
  operations[0].sem_op = 0;
  operations[0].sem_flg = 0;

  return semop(semid, operations, 1);

}


void *waitforendrun ( void * arg)
{

  
  pthread_mutex_lock(&M_cout);
  { 
    time_t x =  time(0);
    cout <<  ctime( &x)  << "  "  << token << "  waitforendrun thread started" << endl;
  }

  pthread_mutex_unlock(&M_cout);

  while(1)
    {
      pthread_mutex_lock(&StartRun);
      nd->clearGlobalErrorStatus();

      pthread_mutex_lock(&M_cout);
      { 
	time_t x =  time(0);
	cout << ctime(&x) << "  "  << token << "  waitforendrun recognized start of run" << endl;
      }
      pthread_mutex_unlock(&M_cout);

      sem_wait(ENDRUNSEM);
      RunIsActive = 0;

      int i;
      for (i=0; i< NUMBEROFBUFFERS; i++)
	{
	  
	  if ( buffers[i].takenmark )
	    {
	      while ( buffers[i].donemark==1 &&  buffers[i].writtenmark == 0)
		{
		  usleep(10000);
		}
	    }
	}


      pthread_mutex_lock(&M_cout);
      { 
	time_t x =  time(0);
	cout << ctime(&x) << "  "  << token << "  waitforendrun recognized end of run, buffers= " << NumberWritten << endl;
      }
      pthread_mutex_unlock(&M_cout);
      nd->close_file();
      nd->set_runnumber(0);

      file_open = 0;
      NumberWritten = 0;
    }

}

void *writebuffers ( void * arg)
{
  int i;
  int previous_run = -1;

  int bytes;

  int events_in_buffer;

  RunIsActive = 0;

#ifdef __EXTRAVERBOSE
  if (verbose) 
    {
      
      time_t x =  time(0);
      pthread_mutex_lock(&M_cout);
      cout << ctime(&x) << "  "  << token << "  here comes the write thread..." <<endl;
      pthread_mutex_unlock(&M_cout);
    }
#endif

  while(1)
    {


      sem_dec( NUMBEROFBUFFERS);

      //      pthread_mutex_lock(&M_cout);
      //      cout << __LINE__ << "  " << __FILE__ << " write thread going  " <<  endl;
      //      pthread_mutex_unlock(&M_cout);


     if ( ! RunIsActive) 
	{

	  RunIsActive =1;
	  pthread_mutex_unlock(&StartRun);
	  
	}



      for (i=0; i< NUMBEROFBUFFERS; i++)
	{
	  
	  if ( buffers[i].donemark )
	    { 
	      //	      cout << " logger: Seeing run " << buffers[i].runnumber 
	      //		   << " buffer count  " <<  buffers[i].bf[3] << " in b " << i << endl;

	      //run = buffers[i].runnumber;

	      // new if we are here, and we don't have a file open, we do it now.
	      // or, if we see that a run number chnages, we close and open a new file.
	      // the idea is that this rarely happens, but who knows.
	      if ( file_open == 0 || run_number != previous_run ) 
		{
		  if ( file_open ) 
		    {
		      nd->close_file();
		      //   if (verbose) cout << " File " << nd->get_filename() << " closed." <<endl;
		    }
		  file_open = 0;

		  // a kludge to get the run number certified.
		  // the eventbuilder sends the wrong run number on occasion.
		  // buffers[i].bf[7] is the run number. Ok. it is a kludge.
		  

		  // cout  << "run number from evb  " << run_number 
		  //	   << " from buffer " <<   buffers[i].bf[7] << endl;
		  //if ( run_number != buffers[i].bf[7] )
		  //  {
		  //    cout << "wrong run number detected " << run_number 
		  //	   << " but should be " <<   buffers[i].bf[7] << endl;
	  
		  //      run_number = buffers[i].bf[7];
		  // }


		  // update the run
		  previous_run = run_number;
		  int i = nd->open_file(run_number, &fd);
		  if (i) 
		    {
		      pthread_mutex_lock(&M_cout);
		      cout << "Could not open file: " <<  nd->get_filename() << endl;
		      pthread_mutex_unlock(&M_cout);
		      cleanup(1);
		    }
		  // remember that we opened a file
		  file_open = 1;
		  pthread_mutex_lock(&M_cout);
		  cout << "Opened file: " <<  nd->get_filename() << endl;
		  pthread_mutex_unlock(&M_cout);

		}

	      int blockcount = ( buffers[i].bytecount + BUFFERBLOCKSIZE -1)/BUFFERBLOCKSIZE;
	      int bytecount = blockcount*BUFFERBLOCKSIZE;

	      if ( 
		  ( buffers[i].bf[0] !=  buffers[i].bytecount && buffer::u4swap( buffers[i].bf[0] ) != buffers[i].bytecount) 
		  || ( buffers[i].bf[1] != BUFFERMARKER  
		       && buffers[i].bf[1] != GZBUFFERMARKER
		       && buffers[i].bf[1] != LZO1XBUFFERMARKER 
		       && buffer::u4swap(buffers[i].bf[1]) != BUFFERMARKER  
		       && buffer::u4swap(buffers[i].bf[1]) != GZBUFFERMARKER
		       && buffer::u4swap(buffers[i].bf[1]) != LZO1XBUFFERMARKER ) 
		  )
		{
		  pthread_mutex_lock(&M_cout);
		  cout << __LINE__ << "  " << __FILE__ << " Corrupt buffer detected, buffer " << i << endl;
		  pthread_mutex_unlock(&M_cout);
		}

	      else
		{
		  
		  bytes = writen ( fd, (char *) buffers[i].bf , bytecount );
		  nd->updateErrorStatus();  // this checks for diskspace after this write
		  

#ifdef __EXTRAVERBOSE
		  pthread_mutex_lock(&M_cout);
		  {
		    time_t x =  time(0);
		    cout << ctime(&x) << "  " << token  << "  writing  b# " << buffers[i].index 
			 << "  in thread  " << hex << pthread_self() << dec << endl;
		  }
		  pthread_mutex_unlock(&M_cout);
#endif

		  if ( bytes < 0)
		    {
#ifdef __EXTRAVERBOSE
		      if ( nd->getGlobalErrorStatus() == AML_ERROR ) // print this message only if this status is new
			{
			  pthread_mutex_lock(&M_cout);
			  cout << "write error return = " << bytes << "  " << bytecount << "  " << hex << buffers[i].bf << dec << endl;
			  perror("writeout");
			  pthread_mutex_unlock(&M_cout);
			}
#endif
		      nd->setGlobalErrorStatus();
		      bytecount = 0;  // see that we don't update the bytecount here
		      buffers[i].error =1;
		    }

		  else   // no write error
		    {
		      nd->add_md5 ( (char *) buffers[i].bf , bytes);
		      NumberWritten++;
		      // extract the number of events from the buffer header, unswapped first
		      if ( buffers[i].bf[1] == LZO1XBUFFERMARKER 
			   || buffers[i].bf[1] == BUFFERMARKER  
			   || buffers[i].bf[1] == GZBUFFERMARKER )
			{
			  events_in_buffer =  buffers[i].bf[2] & 0xffff;
			}
		      else   // need to swap
			{
			  events_in_buffer =   buffer::i4swap(buffers[i].bf[2]) & 0xffff;
			}
		      // and update the event count
		      pthread_mutex_lock(&M_evtctr); 
		      nd->addEventCount (events_in_buffer );
		      pthread_mutex_unlock(&M_evtctr); 
		      buffers[i].error =0;
		      buffers[i].donemark = 0;
		      buffers[i].writtenmark = 1;
		    }

		  if ( nd->updateSize(bytecount) )
		    {
		      //int i =  nd->open_next_file(&fp);
		      int i =  nd->open_next_file(&fd);
		      if (i) 
			{
			  pthread_mutex_lock(&M_cout);
			  cout << "Could not open file: " <<  nd->get_filename() << endl;
			  pthread_mutex_unlock(&M_cout);
			  cleanup(1);
			}
		      // remember that we opened a file
		      file_open = 1;
		      pthread_mutex_lock(&M_cout);
		      cout << "Opened file: " <<  nd->get_filename() << endl;
		      pthread_mutex_unlock(&M_cout);

		    }
		}

#ifdef __EXTRAVERBOSE
	      pthread_mutex_lock(&M_cout);
	      { 
		time_t x =  time(0);
		cout << ctime(&x) << "  " <<  token <<  "  releasing  b# " << i << " (semaphore " 
		     << buffers[i].index << ")" << endl;
	      }
	      pthread_mutex_unlock(&M_cout);
#endif
	      sem_inc(buffers[i].index);
	      //	      cout << "got semaphore..." << endl;
	    }
	      
	}
	  
    }

}

int main( int argc, char* argv[])
{


  ///  msg_buffer *filter = new date_filter_msg_buffer(256);



  //  msgCtrl = new msg_control(MSG_TYPE_ONLINE,  MSG_SOURCE_ET,
  //			    MSG_SEV_DEFAULT, "AML");

  //initialize the "cout" mutex that we'll use to make the output nice
  pthread_mutex_init(&M_cout, 0); 
  pthread_mutex_init(&M_evtctr, 0); 

  pthread_mutex_init(&M_runnr, 0); 

  pthread_mutex_init(&StartRun, 0);  // create this and set to  locked state 
  pthread_mutex_lock(&StartRun);

  
  pthread_mutex_lock(&M_cout);
  cout << "aml version 1.02 Built on " << __DATE__ << "  " << __TIME__ << endl;
  pthread_mutex_unlock(&M_cout);




  int i;

  // now let's assign all signals the handler. 
  // this code assumes that sighup is the first and SIGIO the last signal
  // we know (so it may not be portable, say, to PalmOS).

  //  for ( i=SIGHUP; i<= SIGIO; i++)
  //   {
  //     signal( i , sig_handler);
  //   }



  signal(ET_CONTROL_SIGNAL, et_control_handler);
  

  int filesequence_increment=1;
  int filesequence_start=1;
  int filesize=1500;


  struct sockaddr_in out;


  int databaseflag=0;

  int status;


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

  char c;
  int token_set = 0;
  strcpy(token,"/tmp/aml_");

  while ((c = getopt(argc, argv, "a:b:s:t:vd")) != EOF)
    {
      switch (c) 
	{
	case 'a':   // size of the file pieces
	  if ( !sscanf(optarg, "%d", &filesequence_increment) ) exitmsg();
	  pthread_mutex_lock(&M_cout);
	  cout << "File sequence increment is " << filesequence_increment << endl;
	  pthread_mutex_unlock(&M_cout);
	  break;

	case 'b':   // size of the file pieces
	  if ( !sscanf(optarg, "%d", &filesequence_start) ) exitmsg();
	  pthread_mutex_lock(&M_cout);
	  cout << "File sequence start is " << filesequence_start << endl;
	  pthread_mutex_unlock(&M_cout);
	  break;

	case 's':   // size of the file pieces
	  if ( !sscanf(optarg, "%d", &filesize) ) exitmsg();
	  pthread_mutex_lock(&M_cout);
	  cout << "File size to roll over at is " << filesize << endl;
	  pthread_mutex_unlock(&M_cout);
	  break;

	case 'v':   // verbose
	  verbose = 1;
	  break;

	case 'd':   // no database
	  databaseflag=1;
	  cout << "database access enabled" << endl;
	  break;

	case 't':
	  strcat (token,optarg);
	  token_set = 1;
	  break;
	}
    }


  // kludge an id for the semaphore id's 
  char *xx = token;
  pid_key = 0;
  for ( ; *xx; )  pid_key += *xx++;
  pthread_mutex_lock(&M_cout);
  cout << "pid_key = " << pid_key << endl;
  pthread_mutex_unlock(&M_cout);


  // we get (numberofbuffers +2) semaphores. The +1 is to control
  // the writebuffers thread, the +2 (referred to as ENDRUNSEM)
  // to control the endrun thread.

  semid = semget(pid_key, NUMBEROFBUFFERS+3, 0666 | IPC_CREAT);

  pthread_mutex_lock(&M_cout);
  cout << "Semid = " << semid << endl;
  pthread_mutex_unlock(&M_cout);

  for ( i = 0; i <= NUMBEROFBUFFERS+2; i++)
    {
      sem_set(i, 0);
    }



  //  ------------------------------
  // process the command_line parameters and set up the et_control object

  if (argc <= optind+1) exitmsg();


  nd = new et_control (status, "aml" , token, 1, databaseflag );
  if (status) 
    {
      cleanup(1);
    }
  sscanf (argv[optind],"%d", &the_port);

  

  for (i=0; i< NUMBEROFBUFFERS; i++)
    {
      buffers[i].index = i;
      buffers[i].writtenmark = 0;
      buffers[i].donemark = 0;
      buffers[i].takenmark = 0;
      buffers[i].runnumber = 0;
      buffers[i].buffersize = 0;
      buffers[i].bf = 0;
    }

  // ------------------------
  // now set up the sockets

  if ( (sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0 ) 
    {
      msgCtrl->set_severity(MSG_SEV_ERROR);
      cout  << "cannot create socket" << endl;
      msgCtrl->reset_severity();

      cleanup(1);
    }
  //  int xs = 64*1024+21845;
  int xs = 1024*1024;
  int old_rdbck,rdbck;
  socklen_t opt_rdbck = sizeof(int);

  getsockopt(sockfd,SOL_SOCKET, SO_RCVBUF,&old_rdbck,
	     &opt_rdbck);
  // printf("getsockopt: opt_rdbck: %d\n",opt_rdbck);
  int s = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF,
		     &xs, 4);
  if (s) cout  << "setsockopt status = " << s << endl;

  getsockopt(sockfd,SOL_SOCKET, SO_RCVBUF,&rdbck,
	     &opt_rdbck);
  //  printf("et_server: RCVBUF changed from %d to %d\n",old_rdbck,rdbck);

  getsockopt(sockfd,SOL_SOCKET, SO_SNDBUF,&old_rdbck,
	     &opt_rdbck);
  
  xs = 1024*1024;
  s = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF,
		 &xs, 4);
 
  getsockopt(sockfd,SOL_SOCKET, SO_SNDBUF,&rdbck,
	     &opt_rdbck);

  //  printf("et_server: SNDBUF changed from %d to %d\n",old_rdbck,rdbck);

  bzero( (char*)&server_addr, sizeof(server_addr));
  server_addr.sin_family = PF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  //  server_addr.sin_port = htons(nd->get_port());
  server_addr.sin_port = htons(the_port);


  s = pthread_create(&ThreadId, NULL, 
		     writebuffers, 
		     (void *) 0);
  if (s ) cout << "error in thread create " << s << endl;
  else
    {
      pthread_mutex_lock(&M_cout); 
      cout << "write thread created" << endl;
      pthread_mutex_unlock(&M_cout);
    }

  sem_set (STARTRUNSEM,1);
  s = pthread_create(&ThreadId, NULL, 
		     waitforendrun, 
		     (void *) 0);
  if (s ) cout << "error in thread create " << s << endl;
  else
    {
      pthread_mutex_lock(&M_cout);
      cout << "waitforendrun thread created, ID=" << ThreadId << endl;
      pthread_mutex_unlock(&M_cout);
    }

  int retries = 0;
  while ( (i = bind(sockfd, (struct sockaddr*) &server_addr, 
		    sizeof(server_addr))) < 0 )
    {

      cout  << "tried port " <<   the_port << endl;
      //      nd->set_port( nd->get_port() +1 );
      //server_addr.sin_port = htons(nd->get_port());
      sleep(3);
      if (retries++ > 3) cleanup(1);

    }

  pthread_mutex_lock(&M_cout);
  cout  << "bind complete on port " << the_port << endl;
  pthread_mutex_unlock(&M_cout);


  nd->set_port(the_port);

  nd->set_filerule(argv[optind+1]);

  nd->set_maxfilesize(filesize);

  nd->set_filesequence_start(filesequence_start);
  nd->set_filesequence_increment(filesequence_increment);


  //  cout  << "port is " << the_port << endl;

  /* spawn signal handling thread */
  //  pthread_create(&tid, NULL, signal_thread, (void *)NULL);
  




  // we say it's ok to l;ook at the shared memory now. 
  nd->set_started_ok(1);

  // ------------------------
  // now the endless loop when we open and close connections

  listen(sockfd, 1);
      
  while (sockfd > 0)
    {
			
      //      cout << "before accept..." << endl;

      client_addr_len = sizeof(out); 
      dd_fd = accept(sockfd,  (struct sockaddr *) &out, &client_addr_len);


      if ( dd_fd < 0 ) 
	{
	  msgCtrl->set_severity(MSG_SEV_ERROR);
	  pthread_mutex_lock(&M_cout);
	  cout  << "error in accept socket" << endl;
	  msgCtrl->reset_severity();
	  pthread_mutex_unlock(&M_cout);
	  
	  cleanup(1);
	  delete nd;
	  exit(1);
	}

      

      // should be the default, but set it to blocking mode anyway
      i = ioctl (dd_fd,  FIONBIO, 0);

      // find out where we were contacted from

      char *host = new char[64];  // we'll pass this on to the thread
                                  // and let it delete it

      getnameinfo((struct sockaddr *) &out, sizeof(struct sockaddr_in), host, 64,
		  NULL, 0, NI_NOFQDN); 


      pthread_mutex_lock(&M_cout);
      { 
	time_t x =  time(0);
	cout << ctime(&x) << "  "  << token << "  new connection accepted from " << host << endl;
      }
      pthread_mutex_unlock(&M_cout);

      for (i=0; i< NUMBEROFBUFFERS; i++)
	{
	  
	  if ( buffers[i].takenmark ==0) break;
	}
      if(i == NUMBEROFBUFFERS) cout << "no more buffers left" << endl;
      //cout << "Buffers used are " << i << " and " <<i+1<< endl;

      buffers[i].writtenmark = 1;
      buffers[i+1].writtenmark = 1;
      buffers[i].ended = 0;
      buffers[i+1].ended = 0;
      buffers[i].takenmark = 1;
      buffers[i+1].takenmark = 1;
      struct thread_arg *ta = new struct thread_arg;
      ta->dd_fd =  dd_fd;
      ta->b0 =  &buffers[i];
      ta->b1 =  &buffers[i+1];
      ta->index0 =  i;
      ta->index1 =  i+1;
      ta->host = host;

      sem_set(i,1);
      sem_set(i+1,1);

      s = pthread_create(&ThreadId, NULL, 
			 handle_this_connection, 
			 (void *) ta);
      pthread_mutex_lock(&M_cout);
      if (s ) cout << "error in thread create " << s << endl;
      pthread_mutex_unlock(&M_cout);
		  
    }
  return 0;
}


void * handle_this_connection ( void * arg)
{
  struct thread_arg  *ta = (struct thread_arg *) arg;

  bufferstructure *bf_being_received;
  bufferstructure *bf_being_written;
  bufferstructure *bf_temp;

  int local_runnr;

  int theFD = ta->dd_fd;
  bufferstructure *bs0 = ta->b0;
  bufferstructure *bs1 = ta->b1;

  // this gets the number n for the nth thread
  int my_index = ta->index0 / 2;
  char *host = ta->host;

  bs0->buffersize = 5*256*1024;
  bs1->buffersize = 5*256*1024;

  pthread_mutex_lock(&M_cout);

  cout << "New thread " << hex << pthread_self() << dec 
       << " started for host " << host << endl;
  pthread_mutex_unlock(&M_cout);

  nd->setThreadId(my_index, pthread_self() );
  nd->setHostName(my_index, host );
  nd->incConnectionCount();

  delete ta;

  pthread_detach( pthread_self());

  int go_on,  status;
  int controlword;
  int len;
  int thread_is_started = 0;


  int i;
  
  go_on = 1;

  int nr_buffers = 0;

  bs0->bf = new PHDWORD[bs0->buffersize];
  bs1->bf = new PHDWORD[bs1->buffersize];
 
  // juts initialize to a good default value, it gets re-done in the begrun sequence
  bf_being_received = bs0;
  bf_being_written = bs1;


  char *p;

  while (  go_on  )
    {
      // we always wait for a controlword which tells us what to do next.
      if ( (status = readn (theFD, (char *) &controlword, 4) ) <= 0)
	{
	  pthread_mutex_lock(&M_cout);
	  { 
	    time_t x =  time(0);
	    cout << ctime(&x) << "  " << token << " connection was broken from  " << host << " status= "<< status << endl;
	  }


	  pthread_mutex_unlock(&M_cout);
	  verbose=1; // mlpdebug
	  controlword = 0;
	  close(theFD);

	  if ( thread_is_started )  // we were in the middle of running, so let's clean up.
	    {
	      sem_dec (ENDRUNSEM);  // signal that we are done here
	      sem_dec ( bf_being_written->index);  // wait for the last buffer to be written

	      sem_set(bf_being_written->index,1);  // re-init all to its initial state
	      sem_set(bf_being_received->index,1);

	    }
	      

	  delete [] bs0->bf;
	  delete [] bs1->bf;
	  bs0->buffersize = 0;
	  bs1->buffersize = 0;

	  bs0->takenmark = 0;
	  bs1->takenmark = 0;
	  bs0->writtenmark = 0;
	  bs1->writtenmark = 0;
	  bs0->donemark = 0;
	  bs1->donemark = 0;
	  bs0->runnumber = 0;
	  bs1->runnumber = 0;
	  bs0->bytecount = 0;
	  bs1->bytecount = 0;
	  bs0->error = 0;
	  bs1->error = 0;

	  pthread_mutex_lock(&M_cout);
	  { 
	    time_t x =  time(0);
	    cout << ctime(&x) << "  " << token << "  serving host " << host << " ending" << "   " 
		 << "Number of buffers: " << nr_buffers << endl;
	  }
	  pthread_mutex_unlock(&M_cout);
	  delete [] host;
	  nd->clearThreadId(my_index );
	  nd->clearHostName(my_index );
	  nd->decConnectionCount();


	  return 0;
	}
      //      cout << " rean status " << status << endl;
      controlword = ntohl(controlword);
      
      //      cout  << " controlword = " << controlword << endl;
      
      switch (controlword) 
	{
	  // ------------------------
	  // Begin run
	case  CTRL_BEGINRUN:

	  verbose=0; // mlpdebug

	  readn (theFD, (char *) &local_runnr, 4);
	  
	  if ( thread_is_started )
	    {
	      pthread_mutex_lock(&M_cout);
	      cout << " got redundant begrunsequence from " << host << endl;
	      pthread_mutex_unlock(&M_cout);
	    }
	      
	  thread_is_started = 1;

	  // just for good measure, lock the update of tne runnr.
	  pthread_mutex_lock(&M_runnr);
	  run_number = ntohl(local_runnr);
	  pthread_mutex_unlock(&M_runnr);

	  pthread_mutex_lock(&M_cout);
	  cout  << " new run = " << run_number << " sent from " << host << endl;
	  pthread_mutex_unlock(&M_cout);

	  nd->set_runnumber( run_number);

	  events_in_run = 0;
	  // now we increase the semaphore count by one.
	  // eventually if one after the other gets a endrun,
	  // we decrease and unlock the semaphore.
	  sem_inc (ENDRUNSEM);

	  bf_being_received = bs0;
	  bf_being_written = bs1;

	  bf_being_received->writtenmark=1;
	  bf_being_received->donemark=0;

	  bf_being_written->writtenmark=1;
	  bf_being_written->donemark=0;

	  sem_dec(bf_being_received->index);

	  i = CTRL_REMOTESUCCESS;

	  i = htonl(i);

	  writen (theFD, (char *)&i, 4);
	  break;

	  // ------------------------
	  // end run
	case  CTRL_ENDRUN:

	  verbose=1; // mlpdebug
	  pthread_mutex_lock(&M_cout);
	  { 
	    time_t x =  time(0);
	    cout << ctime(&x)  << "  " << token << " Run end signal from " 
	       << host << "   " << "Number of buffers: " << nr_buffers << endl;
	  }
	  pthread_mutex_unlock(&M_cout);
	  if ( !thread_is_started )
	    {
	      pthread_mutex_lock(&M_cout);
	      cout <<  " got misplaced endrun sequence from " << host << endl;
	      pthread_mutex_unlock(&M_cout);
	    }
	      
	  thread_is_started = 0;

	  sem_dec (ENDRUNSEM);
	  //	  sem_dec ( bf_being_received->index);

	  //	  pthread_mutex_lock(&M_cout);
	  //	  cout << " waiting for semaphore " << bf_being_written->index << endl;
	  //	  pthread_mutex_unlock(&M_cout);

	  sem_dec ( bf_being_written->index);

	  //	  pthread_mutex_lock(&M_cout);
	  //	  cout << " got it, for  " << bf_being_written->index << endl;
	  //	  pthread_mutex_unlock(&M_cout);

	  sem_set(bf_being_written->index,1);
	  sem_set(bf_being_received->index,1);

	  i = htonl(CTRL_REMOTESUCCESS);
	  writen (theFD, (char *)&i, 4);
	  break;

	  // ------------------------
	  // close connection
	case CTRL_CLOSE:
	  pthread_mutex_lock(&M_cout);
	  { 
	    time_t x =  time(0);
	    cout << ctime(&x) << "  " << token << " Closing connection from " << host 
		 << "   " << "Number of buffers: " << nr_buffers << endl;
	  }
	  pthread_mutex_unlock(&M_cout);

	  if ( thread_is_started )  // what? a close without an endrun. ok...
	    {
	      pthread_mutex_lock(&M_cout);
	      cout << "thread " << hex << pthread_self() << dec 
		   << " got misplaced close sequence from " << host << endl;
	      pthread_mutex_unlock(&M_cout);
	      
	      sem_dec (ENDRUNSEM);  // signal that we are done here
	      sem_dec ( bf_being_written->index);  // wait for the last buffer to be written

	      sem_set(bf_being_written->index,1);  // re-init all to its initial state
	      sem_set(bf_being_received->index,1);

	    }
	      


	  close(theFD);

	  //	  cout  << "deleting..." << endl;

	  delete [] bs0->bf;
	  delete [] bs1->bf;
	  bs0->buffersize = 0;
	  bs1->buffersize = 0;

	  //	  cout  << "cleaning up..." << endl;
	  bs0->takenmark = 0;
	  bs1->takenmark = 0;
	  bs0->writtenmark = 0;
	  bs1->writtenmark = 0;
	  bs0->donemark = 0;
	  bs1->donemark = 0;
	  bs0->runnumber = 0;
	  bs1->runnumber = 0;
	  bs0->bytecount = 0;
	  bs1->bytecount = 0;

	  //	  cout << "thread " << hex << pthread_self() << dec << " ending ( " 
	  //     << host  << " )"  << endl;
	  
	  delete [] host;

	  nd->clearThreadId(my_index );
	  nd->clearHostName(my_index );
	  nd->decConnectionCount();

	  return 0;
	  break;

	  // ------------------------
	  // receive data
	case  CTRL_DATA:
	  nr_buffers++;

	  if ( !thread_is_started )
	    {
	      pthread_mutex_lock(&M_cout);
	      cout << "thread " << hex << pthread_self() << dec 
		   << " got data sequence from " << host << " but no beginrun sequence" << endl;
	      pthread_mutex_unlock(&M_cout);
	    }
	      

	  status = readn (theFD, (char *) &len, 4);
	  len = ntohl(len);
	  
	  if (verbose)
	    {
	      pthread_mutex_lock(&M_cout);
	      { 
		time_t x =  time(0);
		cout << ctime(&x)  << "  "  << token << "  CTRL_DATA from " << host <<", len = " << len << " buffer nr " << nr_buffers << endl;
	      }
	      pthread_mutex_unlock(&M_cout);
	    }
	  if ( bf_being_received->error)
	    {
	      pthread_mutex_lock(&M_cout);
	      { 
		time_t x =  time(0);
		cout << ctime(&x)  << "  "  << token << "  error flag set in buffer"<< endl;
		
	      }
	      pthread_mutex_unlock(&M_cout);
	      i = htonl(CTRL_REMOTEFAIL);
	      
	    }
	  else
	    {
	      i = htonl(CTRL_REMOTESUCCESS);  //assume it's ok, we overwrite it if not
	    }

	  if ( ! thread_is_started) 
	    {
	      //just waste the data 
	      char x[len];
	      readn (theFD, x,   len );
	      
	    }

	  else
	    {
	      if ( (len+3)/4 > (int) bf_being_received->buffersize)
		{ 
		  delete [] bf_being_received->bf;
		  bf_being_received->buffersize = (len+3)/4 + 2048;

		  pthread_mutex_lock(&M_cout);
		  cout << "expanding buffer " << bf_being_received->index 
		       << " to " << bf_being_received->buffersize << " for host " << host << endl;
		  pthread_mutex_unlock(&M_cout);

		  bf_being_received->bf = new PHDWORD[ bf_being_received->buffersize];
		}

	      p = (char *) bf_being_received->bf;
	      status = readn (theFD, p,   len );
	      if ( len != status)
		{

		  pthread_mutex_lock(&M_cout);
		  cout << "error on transfer: Expected " << len 
		       << " got status  " << status  << "  for host " << host << endl;
		  perror("ctrl_data");
		  pthread_mutex_unlock(&M_cout);
		  i = htonl(CTRL_REMOTEFAIL);

		}

	      // just by creating a buffer, we do all checks etc.
	      //	  bptr = new buffer( bf_being_received->bf, len);
	      //delete bptr;
	      
	      bf_being_received->bytecount = len;
	      bf_being_received->runnumber = run_number;
	      
	    }
	  if (verbose)
	    {
	      pthread_mutex_lock(&M_cout);
	      { 
		time_t x =  time(0);
		cout << ctime(&x)  << "  "  << token << "  sending ack to " << host << " for buffer nr " << nr_buffers << endl;
	      }
	      pthread_mutex_unlock(&M_cout);
	    }
	  writen (theFD, (char *)&i, 4);


	  if ( thread_is_started) 
	    {

	      // we switch the buffers. Now being_written is the one we
	      // get rid of. We still have to see if the now-being-received
	      // is actually free.
	      bf_temp = bf_being_received;
	      bf_being_received = bf_being_written;
	      bf_being_written  = bf_temp;
	      
	      bf_being_written->writtenmark = 0; // mark this buffer as to be written out
	      bf_being_written->donemark = 1; // mark this buffer as to be written out
	      
	      //	  pthread_mutex_lock(&M_cout);
	      //	  cout << "kicking the write thread" << endl;
	      //	  pthread_mutex_unlock(&M_cout);
	      
	      sem_inc ( NUMBEROFBUFFERS); // kick the write thread
	      
	      // ok, now we may have to wait for the other buffer to be 
	      // written.
	      //	  if ( strcmp (host,"phoncsb.phenix.bnl.gov") == 0)
	      // {
	      //     pthread_mutex_lock(&M_cout);
	      //    cout << host << ": waiting for semaphore " << bf_being_received->index << endl;
	      //    pthread_mutex_unlock(&M_cout);
	      // }
	      
	      sem_dec ( bf_being_received->index);
	      //if ( strcmp (host,"phoncsb.phenix.bnl.gov") == 0)
	      //  {
	      //    pthread_mutex_lock(&M_cout);
	      //    cout << host << ": got it -- " << bf_being_received->index << endl;
	      //    pthread_mutex_unlock(&M_cout);
	      //  }
	      
	      // if we do get that semaphore, it should be 1. We just check.
	      while ( !bf_being_received->writtenmark && !bf_being_received->error )
		{
		  pthread_mutex_lock(&M_cout);
		  { 
		    time_t x =  time(0);
		    cout << ctime(&x) << "  " << token << "  written mark is " <<  bf_being_received->writtenmark 
		       << " for buffer " << bf_being_received->index << endl;
		  }
		  pthread_mutex_unlock(&M_cout);
		  sleep(1);
		}
	      bf_being_received->writtenmark = 0;
	    }
	  break;

	default:
	  pthread_mutex_lock(&M_cout);
	  cout << " Unknown control word: " << hex << controlword << dec << " from host " << host << endl;
	  pthread_mutex_unlock(&M_cout);

	  break;
	}
    }


  //  cout << "returning 0!!" << endl;
  delete [] bs0->bf;
  delete [] bs1->bf;
  bs0->takenmark = 0;
  bs1->takenmark = 0;
  bs0->writtenmark = 0;
  bs1->writtenmark = 0;
  bs0->donemark = 0;
  bs1->donemark = 0;
  
  
  close(theFD);
  pthread_mutex_lock(&M_cout);
  { 
    time_t x =  time(0);
    cout << ctime(&x) << "  "  << token <<  "  thread -- " << hex << pthread_self() << dec 
       << "serving host " << host << " ending" << "   " << "Number of buffers: " << nr_buffers << endl;
  }
  pthread_mutex_unlock(&M_cout);
  delete [] host;
  nd->clearThreadId(my_index );
  nd->clearHostName(my_index );
  nd->decConnectionCount();


  return 0;

}



int readn (int fd, char *ptr, int nbytes)
{

#ifdef FRAGMENTMONITORING
  int history[300];
  int hpos = 0;
#endif

  int nread, nleft;
  //int nleft, nread;
  nleft = nbytes;
  while ( nleft>0 )
    {
      nread = read (fd, ptr, nleft);
      if ( nread <= 0 ) 
	{
	  return nread;
	}

#ifdef FRAGMENTMONITORING
      history[hpos++] = nread;
#endif
      nleft -= nread;
      ptr += nread;
    }

#ifdef FRAGMENTMONITORING
   if ( hpos >1 ) 
    {
      cout << "Fragmented transfer of " << nbytes << "bytes: ";
      for ( int i=0; i<hpos; i++)
	{
	  cout << " " << history[i]<< ",";
	}
      cout << endl;
    }
#endif
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
  cout  << "sig_handler: signal seen " << endl;
  cleanup(0);
}


#if defined(SunOS) || defined(Linux) 
void et_control_handler(int i)
#else
  void et_control_handler(...)
#endif
{
  //cout << "ndd_control_handler: signal seen " << endl;

  nd->confirm_alive();
  signal(ET_CONTROL_SIGNAL, et_control_handler);

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
