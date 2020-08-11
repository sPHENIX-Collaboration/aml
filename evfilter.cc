#include <iostream>
#include <vector>
#include <set>

#include <cstdlib>
#include <csignal>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#include "fileEventiterator.h"
#include "testEventiterator.h"

#include "etEventiterator.h"
#include "ogzBuffer.h"
#include "oBuffer.h"

#include "phenixTypes.h"
#include "oEvent.h"

#include "EventFilter.h"
#include "TriggerFilter.h"
#include "AmlOutStream.h"
#include "EvFilterConfig.h"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#if defined(SunOS) || defined(Linux) || defined(OSF1)
void sig_handler(int);
#else
void sig_handler(...);
#endif

using namespace std;

void exitmsg()
{
  cout << "** usage: evfilter -s -d -v -w -n -i -z source destination" << endl;
  cout << "          evfilter -h for more help" << endl;
  exit(0);
}

void evtcountexitmsg()
{
  cout << "** cannot specify both -e and -c!" << endl;
  cout << "    type  evfilter -h  for more help" << endl;
  exit(0);
}
void exithelp()
{
  cout << endl;
  cout << " evfilter reads events from one source and writes it to a destination. The source can" << endl;
  cout << " be any of the standard sources (ET pool, file, or test stream). The destination " << endl;
  cout << " can be a file or a ET pool, or no destination at all (you cannot write to a test " << endl;
  cout << " stream). Like with a Unix pipe, you can move events through a chain of sources " << endl;
  cout << " and destinations, for example, from one ET pool into another, or into a file. " << endl;
  cout << endl;
  cout << " While the events move through evfilter, you can have them identify themselves. If the " << endl;
  cout << " destination is null, this is a simple way to sift through a stream of events " << endl;
  cout << " and look at their identification messages. " << endl;
  cout << endl;
  cout << " You can throttle the data flow with the -w (wait) option, and you can stop after" << endl;
  cout << " a given number of events with the -n option. " << endl;
  cout << endl;
  cout << " In order to write events from a ET pool called ONLINE to a file (d.evt), use" << endl;
  cout << endl;
  cout << " > evfilter -s etpool -d file ONLINE d.evt" << endl;
  cout << endl;
  cout << " if you want to see which events are coming, aet -i:" << endl;
  cout << endl;
  cout << " > evfilter -s etpool -d file -i ONLINE d.evt" << endl;
  cout << endl;
  cout << " Note that you can abbreviate the etpool, file, and Test to d, f, and T." << endl;
  cout << " > evfilter -s etpool -d file  ONLINE d.evt" << endl;
  cout << " is equivalent to " << endl;
  cout << " > evfilter -s d  -d f  ONLINE d.evt" << endl;
  cout << endl;
  cout << "  List of options: " << endl;
  cout << " -v verbose" << endl;
  cout << " -n <number> stop after so many events" << endl;
  cout << " -h this message" << endl << endl;
  exit(0);
}


// The global pointer to the Eventiterator and use global
// vector of filters (we must be able to
// get at it in the signal handler)
Eventiterator *it;
vector <EventFilter *> filter;

int main(int argc, char *argv[])
{
  int c;
  int status;
  int singlefile = 0;
  int verbose = 0;
  int maxevents = 0;
  int eventnr = 0;
  //   int eventnumber = 0;
  //   int countnumber = 0;

  set <string> filelist;
  // initialize the it pointer to 0;
  it = 0;
  string RunNoString = "NONE";
  string DirString = "";
  string RunFile = "NONE";
  string ConfigFile = "";
  //  if (argc < 3) exitmsg();
  //	cout << "parsing input" << endl;

  while ((c = getopt(argc, argv, "R:D:f:c:n:vh")) != EOF)
    {
      switch (c)
        {
        case 'R':
          RunNoString = optarg;
          cout << "Run Number: " << RunNoString << endl;
          if (singlefile)
            {
              cout << "-R <runno> and -f <file> mutually exclusive" << endl;
              exit(1);
            }
          break;

        case 'D':
          DirString = optarg;
          DirString += "/";
          cout << "Dir: " << DirString << endl;
          break;

        case 'f':
          RunFile = optarg;
          singlefile = 1;
          cout << "Running over file " << RunFile << endl;
          if (RunNoString != "NONE")
            {
              cout << "-R <runno> and -f <file> mutually exclusive" << endl;
              exit(1);
            }
          break;

        case 'c':
          ConfigFile = optarg;
          break;

        case 'v':       // verbose
          verbose = 1;
          break;

        case 'n':       // number of events
          if ( !sscanf(optarg, "%d", &maxevents) )
            exitmsg();
          break;

        case 'h':
          exithelp();
          break;

        default:
          break;

        }
    }
  if (ConfigFile == "")
    {
      cout << "You need to specify a config file with -c <configfile>" << endl;
      exit(1);
    }

  // the scaler filter is special and hardcoded for time being
  EventOutStream *amlout;
  EventFilter *trigfilter;

  EvFilterConfig evconf(ConfigFile);
  for (unsigned int i = 0; i < evconf.Entries(); i++)
    {
      trigfilter = new TriggerFilter(evconf.GetFilterName(i), evconf.GetLevel1Bits(i), evconf.GetLevel2Bits(i), evconf.GetEventType(i) );
      trigfilter->MaxEvents(evconf.GetMaxEvents(i));
      trigfilter->VerboseMod(evconf.GetVerbosity(i));
      if (evconf.GetMachineName(i) != "")
	{
          amlout = new AmlOutStream(evconf.GetMachineName(i), evconf.GetPort(i));
          trigfilter->AddOutStream(amlout);
	}
      filter.push_back(trigfilter);
    }


  set <string>::const_iterator fileiter;
  if (! singlefile)
    {
      DIR *rundir;
      rundir = opendir(DirString.c_str());
      if (! rundir)
	{
	  cout << "Directory " << DirString 
	       << " does not exist, exiting" << endl;
	  exit(0);
	}
      struct dirent *entry;
      while ((entry = readdir(rundir)))
        {
          string filnam = entry->d_name;
          if (filnam.find(RunNoString) < filnam.size())
            {
              filelist.insert(filnam);
            }
        }
      for (fileiter = filelist.begin(); fileiter != filelist.end(); fileiter++)
        {
          cout << "found file: " << *fileiter << endl;
        }
    }
  else
    {
      filelist.insert(RunFile);
    }

  // install some handlers for the most common signals
  signal(SIGKILL, sig_handler);
  signal(SIGTERM, sig_handler);
  signal(SIGINT, sig_handler);
  // see if we can open the file

  // ok. now go through the events
  Event *evt;
  vector <EventFilter *>::iterator filtiter;
  vector <string> MarkedForDeletion;
  vector <string>::iterator deliter;

  int removefilter = 0;
  //cout << " max events = " <<  maxevents << endl;
  for (fileiter = filelist.begin(); fileiter != filelist.end(); fileiter++)
    {
      string fullname = DirString;
      fullname += *fileiter;
      cout << "Full filename " << fullname << endl;
      it = new fileEventiterator(fullname.c_str(), status);
      if (status)
        {
          delete it;
          cout << "Could not open input stream " << fullname << endl;
          continue;
        }
      if (maxevents > 0 && maxevents <= eventnr)
        {
          cout << "Copied requested number of events: " << maxevents << endl;
          break;
        }
      if (filter.empty())
        {
          cout << "No more active filters, quitting" << endl;
          break;
        }
      while ( ( maxevents == 0 || eventnr <= maxevents) &&
              ( evt = it->getNextEvent()) )
        {
          if (removefilter)
            {
              removefilter = 0;
              for (deliter = MarkedForDeletion.begin(); deliter != MarkedForDeletion.end(); deliter++)
                {
                  for (filtiter = filter.begin(); filtiter != filter.end(); filtiter++)
                    {
                      if (*deliter == (*filtiter)->Name())
                        {
                          delete *filtiter;
                          filter.erase(filtiter);
                          break;
                        }
                    }
                }
            }

          if (evt->getErrorCode())
            {
              cout << "Discarding event with Error code "
		   << evt->getErrorCode() << endl;
              delete evt;
              continue;
            }

          if (evt->getEvtLength() <= 0 || evt->getEvtLength() > 2500000)
            {
              cout << "Discarding event with length "
		   << evt->getEvtLength() << endl;
              delete evt;
              continue;
            }
          if (evt->getRunNumber() < 0 || evt->getRunNumber() > 500000)
            {
              cout << "Discarding event with runnumber " << evt->getRunNumber() << endl;
              delete evt;
              continue;
            }
          unsigned int Lvl1Trigger = evt->getTagWord(0);
          unsigned int EvtType = evt->getEvtType();
          unsigned int Lvl2Trigger = evt->getTagWord(1);

          for (filtiter = filter.begin(); filtiter != filter.end(); filtiter++)
            {
              int iret = (*filtiter)->EventOk(Lvl1Trigger, Lvl2Trigger, EvtType);
              switch (iret)
                {
                case EVENTNOACC:
                  break;
                case EVENTACC:
                  (*filtiter)->WriteEvent(evt);
                  break;
                case EVENTEXCEED:
                  (*filtiter)->WriteEvent(evt);
                  cout << (*filtiter)->Name() << " reached max events "
		       << (*filtiter)->MaxEvents() << " removing it" << endl;
                  MarkedForDeletion.push_back((*filtiter)->Name());
                  removefilter = 1;
                  break;
                default:
                  cout << "Unknown Return code from Triggerfilter EventOK method: " << iret << endl;
                  break;
                }
            }
          eventnr++;
          delete evt;

        }
      delete it;
    }
  cout << "Filtering Done, deleting filters now" << endl;
  for (filtiter = filter.begin(); filtiter != filter.end(); filtiter++)
    {
      cout << "Deleting " << (*filtiter)->Name() << endl;
      delete *filtiter;
    }

  return 0;
}

#if defined(SunOS) || defined(Linux) || defined(OSF1)
void sig_handler(int i)
#else
void sig_handler(...)
#endif
{
  cout << "sig_handler: signal seen " << endl;
  if (it)
    {
      delete it;
    }
  vector <EventFilter *>::iterator filtiter;
  for (filtiter = filter.begin(); filtiter != filter.end(); filtiter++)
    {
      cout << "Deleting " << (*filtiter)->Name() << endl;
      delete *filtiter;
    }

  exit(0);
}

