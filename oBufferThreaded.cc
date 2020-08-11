
#include <oBufferThreaded.h>

#include <unistd.h>

// the constructor first ----------------

oBufferThreaded::oBufferThreaded (int fd, const int length
		, const int irun, const int iseq)
{

  the_fd = fd;

  our_fd = 0;
  good_object = 1;
  bptr0 = ( buffer_ptr) new int[length];
  bptr1 = ( buffer_ptr) new int[length];

  bptr = bptr0;
  bptr_being_written = bptr1;
  data_ptr = &(bptr->data[0]);
  max_length = length;
  max_size = max_length;
  left = max_size - BUFFERHEADERLENGTH;
  bptr->ID = -64;
  bptr->Bufseq = iseq;
  bptr->Runnr = 0;
  current_event = 0;
  current_etype = DATAEVENT;
  sequence = iseq;
  eventsequence = 0;
  runnumber = irun;
  byteswritten = 0;
  ThreadId = 0;

  prepare_next ();

}


// ----------------------------------------------------------
// 
//
int oBufferThreaded::writeout()
{

  //  void *writeThread(void *);

  int status;
  
  if (the_fd == 0) return 0;

  if (ThreadId) 
    {
      status =  pthread_join(ThreadId, NULL);
      if (status) 
	{
	  std::cout << "pthread_join error: " << status << std::endl;
	}
      byteswritten += thread_arg[2];  // the number of bytes written from previosu thread
    }
  /*
  else
    {
      cout << "ThreadId  is" <<  ThreadId << endl;
    }
  */
  //wait for a potential old thread to complete
  

  if (thread_arg[3] < 0)  
    {
      std::cout << "error in write" << std::endl;
    }
  
  if (! dirty) return 0;
  
  if (! has_end) addEoB();
  
  //swap the buffers around
  buffer_ptr tmp = bptr_being_written;
  bptr_being_written = bptr;
  bptr = tmp;
  dirty = 0;
  // now fork off the write thread
  
  thread_arg[0] =  the_fd;
  thread_arg[1] = (unsigned long long )  bptr_being_written;
  thread_arg[2] = 0;
  thread_arg[3] = 0;

  if ( (status = pthread_create(&ThreadId, NULL, oBufferThreaded::writeThread, (void *) thread_arg)) )
    {
  std::cout << "oBufferThreaded: error in thread create " << status << std::endl;
    }
  return 0;
  
}




// ----------------------------------------------------------
oBufferThreaded::~oBufferThreaded()
{
  writeout();
  int status;
  if (ThreadId) 
    {
      status =  pthread_join(ThreadId, NULL);
      if (status) 
	{
	  std::cout << "pthread_join error: " << status << std::endl;
	}
      byteswritten += thread_arg[2];  // the number of bytes written from previosu thread
    }
  /*
  else
    {
      std::cout << "ThreadId  is" <<  ThreadId << std::endl;
    }
  */
  delete [] bptr0;
  delete [] bptr1;


}


// ----------------------------------------------------------
void *oBufferThreaded::writeThread( void *arg)
{

  unsigned long long  *intarg = (unsigned long long  *) arg;
  int f = intarg [0];
  buffer_ptr bptr = (buffer_ptr ) intarg[1];
  
  char *cp = (char *) bptr;
  
  int blockcount = (bptr->Length + BUFFERBLOCKSIZE -1)/BUFFERBLOCKSIZE;
  /*
  int iiw  = write ( f, cp, BUFFERBLOCKSIZE*blockcount);
  intarg[2] = blockcount*BUFFERBLOCKSIZE;
  intarg[3] = iiw;
  */
  /* that's faster (but more confusing than the lines above) */
  intarg[2] = blockcount*BUFFERBLOCKSIZE;
  intarg[3] = write ( f, cp, intarg[2] );

  return 0;
}

