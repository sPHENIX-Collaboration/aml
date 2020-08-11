
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
  cout << "** usage: et_display  [-t token] " << endl;
  exit(0);
}


int main( int argc, char* argv[])
{
  int status = -1;

  //  char userid[L_cuserid];
  char token[L_cuserid+128];
  char c;
  int token_set = 0;
  strcpy(token,"/tmp/etlogger_");
  while ((c = getopt(argc, argv, "t:")) != EOF)
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

  et_control *nd = new et_control (status, token, 1);

  if ( status) cout << "the server is not running." << endl
		    << "maybe the token is wrong?" << endl;

  else
    {
			
      cout << "logger's pid  is " << nd->get_pid() << endl;
      //      cout << "logger port is " << nd->get_port() << endl;
			
      if ( nd->get_logging() ) cout << "Logging is enabled" << endl;
      else cout << "Logging is disabled" << endl;

      if ( nd->get_compression() ) cout << "Compression is enabled" << endl;
      else cout << "Compression is disabled" << endl;

      cout << "current ET pool name: " <<  nd->get_et_name() << endl;

      cout << "current filename: " <<  nd->get_filename() << endl;
      cout << "filename rule:    " <<  nd->get_filerule() <<endl;

      if ( nd->get_maxfilesize() >0 )
	{
	  cout << "File will roll over after " 
	       << nd->get_maxfilesize() << "Kb" << endl;
	  cout << "current file in sequence is " 
	       << nd->get_current_file_sequence() << endl;
	}
      else
	cout << "file rollover deactivated" << endl;

      unsigned int x = (unsigned int ) nd->get_mb_written();
      cout << "Mbytes written: " << x << endl;

      //      if (nd->started_ok() ) cout << "server looks ok"  <<endl;
      //else cout << "server did not start up right"  <<endl;

      if (nd->is_alive()) cout << "server is alive" << endl;
      else  cout << "server is dead??" << endl;

    }

  delete nd;

}
