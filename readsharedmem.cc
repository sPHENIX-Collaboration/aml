
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
  cout << "** usage: readsharedmem [-t token] " << endl;
  exit(0);
}


int main( int argc, char* argv[])
{
  int status =-1;
  //  int oldrun = 0;
  //  char userid[L_cuserid];
  char token[L_cuserid+128];
  char c;
  int token_set = 0;

  //  int waitinterval = 5;

  strcpy(token,"/tmp/aml_");
  while ((c = getopt(argc, argv, "t:w:")) != EOF)
    {
      switch (c) 
	{
	case 't':
	  strcat (token,optarg);
	  token_set = 1;
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

  cout << "token: " << token << endl;
  et_control *nd = new et_control (status, token, 1);
  if ( status) 
    {
      cout << "no et_control" << endl;
      delete nd;
      exit(1);
    }

  cout << "prdfname: " << nd->get_filename() << endl;
  cout << "runnumber: " << nd->get_current_run() << endl;
  cout << "mb: " << nd->get_mb_written() << endl;
  cout << "nr_events_run: " << nd->get_nr_events_run() << endl;
  cout << "nr_events_segment: " << nd->get_nr_events_segment() << endl;
  delete nd;

}
