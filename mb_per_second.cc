
#include <sys/time.h>
#include <et_control.h>
#include <iostream>
#include <string>
#include <cstdlib>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <string.h>

using namespace std;

void exitmsg()
{
  cout << "** usage: mb__per_second  [-t token] " << endl;
  exit(0);
}


int main( int argc, char* argv[])
{
  int status = -1;

  struct timeval  t1, t2;
  unsigned int   m1, m2;

  //  char userid[L_cuserid];
  char token[L_cuserid+128];
  char c;
  int token_set = 0;
  int waitinterval = 20;

  strcpy(token,"/tmp/etlogger_");
  while ((c = getopt(argc, argv, "t:w:")) != EOF)
    {
      switch (c) 
	{
	case 't':
	  strcat (token,optarg);
	  token_set = 1;
	  break;
	case 'w':   // wait interval
	  if ( !sscanf(optarg, "%d", &waitinterval) ) exitmsg();
	  break;
	}
    }

  if (! token_set)
    {
      if (getenv("ET_ID") == NULL)
	{
	  // strcat(token, cuserid(userid));
	}
      else
	{
	  strcat(token,getenv("ET_ID"));
	}
    }

  et_control *nd = new et_control (status, token, 1);
  cout << "status = " << status << endl;
  if ( status) cout << "the server is not running." << endl
		    << "maybe the token is wrong?" << endl;

  else
    {
			
      gettimeofday(&t1, NULL);
      m1 = nd->get_mb_written();

      for(;;)
	{
	  sleep (waitinterval);
	  gettimeofday(&t2, NULL);
	  m2 = nd->get_mb_written();
	  double mbs = ( m2 - m1) / ( double(t2.tv_sec - t1.tv_sec) );
	  cout << "mb/s: " << mbs << endl;
	  
	  gettimeofday(&t1, NULL);
	  m1 = nd->get_mb_written();
	}
      
    }

  delete nd;

}
