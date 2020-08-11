
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
  cout << "** usage: aml_nextfile [-t token] " << endl;
  exit(0);
}


int main( int argc, char* argv[])
{
  int status =-1;

  //  char userid[L_cuserid];
  char token[L_cuserid+128];
  char c;
  int token_set = 0;

  int waitinterval = 5;

  strcpy(token,"/tmp/aml_");
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
  if ( status) 
    {
      cout << endl;
      delete nd;
      exit(1);
    }

  char oldfile[256];
  
  strcpy (oldfile, nd->get_filename() );
  
  while ( strcmp(oldfile,  nd->get_filename()) == 0 )
    {
      delete nd;

      sleep(waitinterval);
      
      nd = new et_control (status, token, 1);
      if ( status) 
	{
	  cout << oldfile << endl;
	  delete nd;
	  exit(1);
	}

    }
  
  cout << oldfile << endl;
  delete nd;

}
