
#include <et_control.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <cstdlib>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <string.h>

using namespace std;

void exitmsg()
{
  cout << "** usage: aml_display  [-t token] " << endl;
  exit(0);
}


int main( int argc, char* argv[])
{
  int status = -1;
  int verbose = 0;

  //  char userid[L_cuserid];
  char token[L_cuserid+128];
  char c;
  int token_set = 0;
  strcpy(token,"/tmp/aml_");
  while ((c = getopt(argc, argv, "t:v")) != EOF)
    {
      switch (c) 
	{
	case 't':
	  strcat (token,optarg);
	  token_set = 1;
	  break;
	case 'v':
	  verbose=1;
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

  if ( status) cout << "the server is not running." << endl
		    << "maybe the token is wrong?" << endl;

  else
    {
			
      status = nd->wipe_mem();
      if (status) cout << "Error clearing memory" << endl;
      delete nd;
      return 1;

    } 
      
  delete nd;
  return 0;
}
