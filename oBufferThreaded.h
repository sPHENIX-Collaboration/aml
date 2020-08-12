#ifndef __OBUFFERTHREADED_H__
#define __OBUFFERTHREADED_H__


#include <cstdio>
#include <iostream>
#include <iomanip>

#include <Event/phenixTypes.h>
#include <Event/oBuffer.h>
#include <Event/Event.h>

#include <Event/BufferConstants.h>
#include <Event/EventTypes.h>

#include <pthread.h>




#ifndef __CINT__
class WINDOWSEXPORT oBufferThreaded : public oBuffer{
#else
class  oBufferThreaded : public oBuffer {
#endif

public:

  oBufferThreaded (int fd, const int length,
		const int irun=1 , const int iseq=0 );


  virtual  ~oBufferThreaded();

  // now the transfer routine
  //int transfer ( dataProtocol * dprotocol );

  virtual int writeout ();


protected:
  //  add end-of-buffer

  static void *writeThread(void * arg);

  int the_fd;

  pthread_t ThreadId;

  unsigned long long thread_arg[4];


};



#endif /* __OBUFFERTHREADED_H__ */

