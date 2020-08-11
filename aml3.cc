
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <string>

#include <dlfcn.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/mman.h>   /* sendfile */
#include <sys/sem.h>
#include <sys/sendfile.h>   /* sendfile */
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>




#define KEY (1492)



#ifdef HAVE_BSTRING_H
#include <bstring.h>
#else
#include <strings.h>
#endif

#ifdef SunOS
#include <sys/filio.h>
#endif

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

#define WRITESEM 1
#define ENDRUNSEM 2
#define STARTRUNSEM 3


int verbose = 0;
int sockfd = 0;
int dd_fd = 0;

oBuffer *ob;
FILE *fp;
int fd;
off_t file_offset = 0;  // "file" offset = 0;

EventTagger *ETagger;

msg_control *msgCtrl;

pthread_t ThreadId;
pthread_t       tid;
int *thread_arg;

int current_index;

int events_in_run;
int event_scaler = 0;

et_control *nd = 0;
int writeflag;

int the_port = 5001;

int RunIsActive = 0; 
int NumberWritten = 0; 
int file_open = 0;

int currentRun = -1;
int previousRun = -2;



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

void  * handle_this_connection ( void * arg);

int write_to_output ( int theFD , const int len);


#include <msg_control.h>
//#include <filter_msg_buffer.h>

//filter_msg_buffer filter(256);


void exitmsg()
{
  std::cout << "** This is the aml (threaded version). No gimmicks. Pure Power." << std::endl;
  std::cout << "** usage: aml   [-t token] port-number file-rule" << std::endl;
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

  while(1)
    {
      if ( sem_lock(STARTRUNSEM) < 0 )  perror("semlock");
      sem_set(STARTRUNSEM, 1);
      std::cout << "waitforendrun recognized start of run" << std::endl;
      sem_lock(ENDRUNSEM);
      RunIsActive = 0;
      std::cout << "waitforendrun recognized end of run, buffers= " << NumberWritten << std::endl;
      nd->close_file();
      file_open = 0;
      NumberWritten = 0;
    }

}


int main( int argc, char* argv[])
{



  msgCtrl = new msg_control(MSG_TYPE_ONLINE,  MSG_SOURCE_ET,
			    MSG_SEV_DEFAULT, "ET_server");


  int i;
  int filesequence_increment=1;
  int filesequence_start=0;
  int filesize=1500;

  struct sockaddr_in out;

  // we get (numberofbuffers +2) semaphores. The +1 is to control
  // the writebuffers thread, the +2 (referred to as ENDRUNSEM)
  // to control the endrun thread.

  semid = semget(KEY, 4, 0666 | IPC_CREAT);
  std::cout << "Semid = " << semid << std::endl;
  for ( i = 0; i <= 3; i++)
    {
      sem_set(i, 0);
    }


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

  while ((c = getopt(argc, argv, "a:b:s:t:v")) != EOF)
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

	case 's':   // size of the file pieces
	  if ( !sscanf(optarg, "%d", &filesize) ) exitmsg();
	  std::cout << "File size to roll over at is " << filesize << std::endl;
	  break;

	case 'v':   // verbose
	  verbose = 1;
	  break;

	case 't':
	  strcat (token,optarg);
	  token_set = 1;
	  break;
	}
    }

  //  ------------------------------
  // process the command_line parameters and set up the et_control object

  if (argc < optind+2) exitmsg();


  nd = new et_control (status, "aml" , token );
  if (status) 
    {
      cleanup(1);
    }


  sscanf (argv[optind],"%d", &the_port);
  nd->set_port(the_port);
  std::cout << "The port: " << the_port << std::endl;



  nd->set_filerule(argv[optind+1]);

  nd->set_maxfilesize(filesize);

  nd->set_filesequence_start(filesequence_start);
  nd->set_filesequence_increment(filesequence_increment);


  
  signal(SIGKILL, sig_handler);
  signal(SIGTERM, sig_handler);
  signal(SIGINT, sig_handler);
  signal(ET_CONTROL_SIGNAL, et_control_handler);
 
  

  // ------------------------
  // now set up the sockets

  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) 
    {
      msgCtrl->set_severity(MSG_SEV_ERROR);
      std::cout << *msgCtrl  << "cannot create socket" << std::endl;
      msgCtrl->reset_severity();

      cleanup(1);
    }
  //  int xs = 64*1024+21845;
  int xs = 1024*1024;
  int old_rdbck,rdbck;
  socklen_t opt_rdbck = sizeof(int);

  getsockopt(sockfd,SOL_SOCKET, SO_RCVBUF,&old_rdbck,
	     &opt_rdbck);
  //printf("getsockopt: opt_rdbck: %d\n",opt_rdbck);
  int s = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF,
		     &xs, 4);
  if (s) std::cout << *msgCtrl  << "setsockopt status = " << s << std::endl;

  getsockopt(sockfd,SOL_SOCKET, SO_RCVBUF,&rdbck,
	     &opt_rdbck);
  printf("et_server: RCVBUF changed from %d to %d\n",old_rdbck,rdbck);

  getsockopt(sockfd,SOL_SOCKET, SO_SNDBUF,&old_rdbck,
	     &opt_rdbck);
  
  xs = 1024*1024;
  s = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF,
		 &xs, 4);
 
  getsockopt(sockfd,SOL_SOCKET, SO_SNDBUF,&rdbck,
	     &opt_rdbck);

  //  printf("et_server: SNDBUF changed from %d to %d\n",old_rdbck,rdbck);

  bzero( (char*)&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(nd->get_port());


  //  s = pthread_create(&ThreadId, NULL, 
  //		     writebuffers, 
  //		     (void *) 0);
  //if (s ) std::cout << "error in thread create " << s << std::endl;
  //else
  //  std::cout << "write thread created" << std::endl;


  sem_set (STARTRUNSEM,1);

  s = pthread_create(&ThreadId, NULL, 
		     waitforendrun, 
		     (void *) 0);
  if (s ) std::cout << "error in thread create " << s << std::endl;
  else
    std::cout << "waitforendrun thread created" << std::endl;


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
  // now the std::endless loop when we open and close connections

  listen(sockfd, 1);
      
  while (sockfd > 0)
    {
			
      std::cout << "before accept..." << std::endl;
      dd_fd = accept(sockfd, (struct sockaddr*) &out, &client_addr_len);
      if ( dd_fd < 0 ) 
	{
	  msgCtrl->set_severity(MSG_SEV_ERROR);
	  std::cout << *msgCtrl  << "error in accept socket" << std::endl;
	  msgCtrl->reset_severity();
	  
	  cleanup(1);
	  delete nd;
	  exit(1);
	}

      std::cout << "socket number " << dd_fd << std::endl;

      // should be the default, but set it to blocking mode anyway
      i = ioctl (dd_fd,  FIONBIO, 0);

      // find out where we were contacted from
      char *host = new char[64];  // we'll pass this on to the thread
                                  // and let it delete it

      getnameinfo((struct sockaddr *) &out, sizeof(struct sockaddr_in), host, 64,
		  NULL, 0, NI_NOFQDN); 


      std::cout << *msgCtrl  << "new connection accepted from " << host << std::endl;


      thread_arg = new int[6];
      thread_arg[0] =  dd_fd;
      thread_arg[5] = (int) host;

      s = pthread_create(&ThreadId, NULL, 
			 handle_this_connection, 
			 (void *) thread_arg);
      if (s ) std::cout << "error in thread create " << s << std::endl;
	  
    }
  return 0;
}


int write_to_output ( int theFD , const int len)
{

  
  int status; 

  //  std::cout << "trying to lock " << pthread_self << " for " << len << " bytes" << std::endl;
  if ( sem_lock(WRITESEM) < 0 )  perror("semlock");
  //  std::cout << "aquiring " << pthread_self << std::endl;


   if ( ! RunIsActive) 
    {
      RunIsActive =1;
      sem_set(STARTRUNSEM,0);
    }

   if ( file_open == 0 || currentRun != previousRun ) 
     {
       if ( file_open ) 
	 {
	   nd->close_file();
	   if (verbose) std::cout << " File " << nd->get_filename() << " closed." <<std::endl;
	 }
       file_open = 0;
       previousRun = currentRun;
       int i = nd->open_file(currentRun, &fd);
       if (i) 
	 {
	   std::cout << "Could not open file: " <<  nd->get_filename() << std::endl;
	   sem_dec(WRITESEM);
	   return 1;
	 }
       // remember that we opened a file
       file_offset = 0;
       file_open = 1;
       std::cout << "Opened file: " <<  nd->get_filename() << std::endl;
       
     }

   int map_length = len;
   if (len%8192)
     {
       map_length += ( 8192 - len%8192);
     } 

   //  std::cout << "map_length = " << map_length << std::endl; 
   if ( nd->updateSize(map_length) ) 
     {

       int i = nd->open_next_file(&fd);
       if (i) 
	 {
	   std::cout << "Could not open file: " <<  nd->get_filename() << std::endl;
	   return 1;
	 }
       if (verbose) std::cout << "Starting next file " << nd->get_filename() << std::endl;
       file_offset = 0;
     }

   if ( ( status = ftruncate( fd, file_offset + map_length)) )
   {
     perror("ftruncate");
     return 2;
   }

   char * tmp_buf = (char * ) mmap(0, map_length, 
			 PROT_READ|PROT_WRITE , MAP_SHARED, 
			 fd, file_offset );
   if ( tmp_buf == (char *) 0xffffffff) 
     {
       perror("mmap");
       return errno;
     }
 
   if ( (status = readn (theFD, (char *) tmp_buf, len)) )
     {
       return 3;
     }
 
   if ( (status = munmap(tmp_buf, map_length)) )
     {
       perror ("munmap");
       return errno;
     }

   file_offset += map_length;
   
   NumberWritten++;

   sem_dec(WRITESEM);
   

   return 0;

}

void * handle_this_connection ( void * arg)
{
  int *intarg = (int *) arg;


  int theFD = (int) intarg[0];
  char *host = ( char *) intarg[5] ;


  std::cout << "New thread " << std::hex << pthread_self() << std::dec 
       << " started " << std::endl;

  delete [] intarg;

  pthread_detach( pthread_self());

  int go_on, len, status;
  int controlword;
  int run_number;

  int i;
  
  go_on = 1;




  while (  go_on  )
    {
      // we always wait for a controlword which tells us what to do next.
      if ( (status = readn (theFD, (char *) &controlword, 4) ) < 0)
	{
	  //cout << *msgCtrl  << "readn status = " << status << std::endl;
	  controlword = 0;
	  close(theFD);

	  std::cout << "thread " << std::hex << pthread_self() << std::dec << " ending ( " 
	       << host  << " )"  << std::endl;
	  delete [] host;

	  return 0;
	}
      controlword = ntohl(controlword);
      
      //       std::cout << *msgCtrl  << " controlword = " << controlword << std::endl;
      
      switch (controlword) 
	{
	  // ------------------------
	  // Begin run
	case  CTRL_BEGINRUN:
	  readn (theFD, (char *) &run_number, 4);
	  
	  run_number = ntohl(run_number);
	  std::cout << *msgCtrl  << " new run = " << run_number << std::endl;
	  events_in_run = 0;
	  currentRun = run_number;
	  // now we increase the semaphore count by one.
	  // eventually if one after the other gets a endrun,
	  // we decrease and unlock the semaphore.
	  sem_inc (ENDRUNSEM);


	  i = CTRL_REMOTESUCCESS;
	  i = htonl(i);
	  writen (theFD, (char *)&i, 4);

	  break;

	  // ------------------------
	  // end run
	case  CTRL_ENDRUN:

	  // we count down the number of non-ended threads
	  sem_dec (ENDRUNSEM);

	  std::cout << "Run end signal from " << host << std::endl;
	  // we acknowledge the receipt
	  i = htonl(CTRL_REMOTESUCCESS);
	  writen (theFD, (char *)&i, 4);

	  break;

	  // ------------------------
	  // close connection
	case CTRL_CLOSE:
	  // std::cout << *msgCtrl  << "Closing connection..." << std::endl;
	  std::cout << *msgCtrl  << "Closing connection from " << host << std::endl;
	  delete [] host;

	  close(theFD);

	  return 0;
	  break;

	  // ------------------------
	  // receive data
	case  CTRL_DATA:
	  readn (theFD, (char *) &len, 4);
	  len = ntohl(len);
	  //  std::cout << "in CTRL_DATA , len = " << len << std::endl;

	  status = write_to_output ( theFD , len);

	  if (status)
	    {
	      i = CTRL_REMOTEFAIL;
	    }
	  else
	    {
	      i = CTRL_REMOTESUCCESS;
	    }
	      
	  i = htonl(i);

	  // acknowledgement
	  writen (theFD, (char *)&i, 4);

	  break;

	default:
	  break;
	}
    }
  //  std::cout << "returning 0!!" << std::endl;
  
  
  close(theFD);
  std::cout << "thread -- " << std::hex << pthread_self() << std::dec << " ending" << std::endl;
  delete [] host;

  return 0;

}



int readn (int fd, char *ptr, int nbytes)
{

  int total = 0;
  int cnt;

  while ( ( cnt = read(fd,&ptr[total],(nbytes-total)) ) )
    {
      total+= cnt;
    }
  /*  
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
  */
  return ( nbytes - total);
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
  //cout << *msgCtrl  << "sig_handler: signal seen " << std::endl;
  cleanup(0);
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
