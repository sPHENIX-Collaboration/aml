
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
			
      cout << "logger's pid  is " << nd->get_pid() << endl;
      cout << "logger port is " << nd->get_port() << endl;
			
      //      if ( nd->get_logging() ) cout << "Logging is enabled" << endl;
      //else cout << "Logging is disabled" << endl;

      // if ( nd->get_compression() ) cout << "Compression is enabled" << endl;
      //else cout << "Compression is disabled" << endl;

      //cout << "current ET pool name: " <<  nd->get_et_name() << endl;

      cout << "current run: "      <<  nd->get_current_run() << endl;
 
      if ( nd->getGlobalErrorStatus() )
	{

	  if ( nd->getGlobalErrorStatus() & AML_ERROR)
	    {
	      cout << "Error: ";
	      cout << "could not write buffer to disk" << endl;
	    }

	  if ( nd->getGlobalErrorStatus() & AML_SEVERE)
	    {
	      cout << "* Severe Warning: about to run out of disk space" << endl;
	    }
	  else if ( nd->getGlobalErrorStatus() & AML_WARNING)
	    {
	      cout << "* Warning: disk space is running low" << endl;
	    }
	}

      if ( nd->get_maxfilesize() >0 )
	{
	  cout << "File will roll over after " 
	       << nd->get_maxfilesize() << "MB" << endl;
	  cout << "current file in sequence is " 
	       << nd->get_current_file_sequence() << endl;
	}
      else
	cout << "file rollover deactivated" << endl;

      unsigned int x = (unsigned int ) nd->get_mb_written();
      cout << "Mbytes written: " << x << endl;

      //      if (nd->started_ok() ) cout << "server looks ok"  <<endl;
      //else cout << "server did not start up right"  <<endl;

      //      if (nd->is_alive()) cout << "server is alive" << endl;
      // else  cout << "server is dead??" << endl;
      cout << "current filename: " <<  nd->get_filename() << endl;
      cout << "filename rule:    " <<  nd->get_filerule() <<endl;
      cout << "Events in segm.:  " <<  nd-> get_nr_events_segment() <<endl;
      cout << "Events in run:    " <<  nd-> get_nr_events_run() <<endl;
     
      cout << "Number of open connections:    " <<  nd->getConnections() <<endl;

      if (verbose)
	{
	  for (int i = 0; i < MAX_THREADS; i++)
	    {
	      if (nd->getThreadId(i) )
		{
		  cout << hex << setw(8) << nd->getThreadId(i) << dec << "  " << nd->getHostName(i) << endl;
		}
	    }
	}
  
    }
  delete nd;
  
}
