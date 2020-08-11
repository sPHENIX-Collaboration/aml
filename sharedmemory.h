// -*- c++ -*-
#ifndef __SHAREDMEMORY_H__
#define __SHAREDMEMORY_H__

#include <cstddef>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string>
#include <cerrno>
#include <cstdio>
#include <iostream>
#include <et_structure.h>

// 

#ifdef WINDOWS
#include "stdafx.h"
#define WINDOWSEXPORT  __declspec(dllexport)
#else
#define WINDOWSEXPORT 
#endif

class  WINDOWSEXPORT sharedmemory {

public:
  // destructor
  virtual ~sharedmemory();

  sharedmemory ( const char * token, const int id, 
		 controlbuffer **structure, const int slen, int &status, const int inew =0);


  virtual int i_am_owner() const { return wasnew; };


protected:
  controlbuffer *theMem;
  int length;
  int tokenid;
  int memid;
  int wasnew;
  char * lockfile;
  FILE *fp;

};

#endif /* __SHAREDMEMORY_H__ */
