
#include <sharedmemory.h>
#include <string.h>

sharedmemory::sharedmemory(const char * token, const int id, 
	  controlbuffer **structure, const int slen, int &status, const int inew )
{
  int ishmflg;

  lockfile = new char[strlen(token)+1];
  strcpy ( lockfile, token);


  //  cout << "lockfile = " << lockfile << "  inew = " << inew << endl;

  if (inew)
    {
      fp = fopen ( lockfile, "w");
    }
  else
    {
      fp = fopen ( lockfile, "r");
    }

  if ( fp == 0) 
    {
      //perror("sharedmmemory: " );
      
      status =-1;
      *structure = 0;
      theMem = *structure;
      return;
    }
  
  length = ((slen+255)/256)*256;
  tokenid = ftok ( token, id);
  wasnew = inew;
  if (inew)
    {
      ishmflg =  0666 | IPC_CREAT;
    }
  else
    {
      ishmflg =  0666;
    }
  memid = shmget(tokenid, length , ishmflg);

  if (memid < 0) 
    {

      perror("sharedmmemory fail: " );
      status = memid;
      *structure = 0;
      theMem = 0;
      return;
    }
  *structure = (controlbuffer *) shmat(memid, NULL, 0);
  //  cout << "sharedmemory: token = " << token << " memid " << memid 
  //       << " adr = " << *structure << endl;
  theMem = *structure;
  status = 0;
}
 
sharedmemory::~sharedmemory()
{
  if (fp) 
    {
      //  cout << "closing lockfile " << lockfile << endl;
      fclose(fp);
    }

  delete [] lockfile;

  shmdt(theMem);

  if (wasnew)
    {

#if defined(SunOS) || defined(Linux)
      struct shmid_ds shmb;
      shmctl (memid, IPC_RMID,&shmb);

#else
      shmctl (memid, IPC_RMID);

#endif

    }
}
