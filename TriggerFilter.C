#include <iostream>
#include "Event.h"
#include "TriggerFilter.h"
#include "EtOutStream.h"

using namespace std;

TriggerFilter::TriggerFilter(const string &name)
{
  ThisName = name;
  Lvl1TrigBits(0x0);
  Lvl2TrigBits(0x0);
  EventType(0);
  ntrig = 0;
  verbosemod = 1;
  outstream = NULL;
}

TriggerFilter::TriggerFilter(const string &name, const unsigned int lvl1trig, const unsigned int lvl2trig, const unsigned int type)
{
  ThisName = name;
  Lvl1TrigBits(lvl1trig);
  Lvl2TrigBits(lvl2trig);
  EventType(type);
  cout << ThisName << ": Setting the lvl1 mask to 0x"
       << hex << lvl1trig
       << ", the lvl2 mask to 0x"
       << lvl2trig << dec
       << " EventType: " << type << endl;
  ntrig = 0;
  verbosemod = 1;
  outstream = NULL;
}

TriggerFilter::~TriggerFilter()
{
  if (outstream)
    {
      delete outstream;
    }
  cout << ThisName << ": No of Events: " << ntrig << endl;
  return ;
}

int TriggerFilter::EventOk(unsigned int lvl1trigger, unsigned int lvl2trigger, unsigned int Evttype)
{
  if ( ((!lvl1trigbits) || lvl1trigger & lvl1trigbits) &&
       ((!lvl2trigbits) || lvl2trigger&lvl2trigbits) &&
       ((!evttype) || Evttype == evttype) )
    {
      if (!(ntrig % verbosemod))
        {
          cout << ThisName << ": OK Event: Lvl1: 0x" << hex << lvl1trigger
	       << " Lvl2: 0x" << lvl2trigger << dec
	       << " EventType: " << Evttype << endl;
        }
      ntrig++;
      if (maxevents && maxevents <= ntrig)
        {
          return EVENTEXCEED;
        }
      return EVENTACC;
    }
  return EVENTNOACC;
}

int TriggerFilter::WriteEvent(Event *evt)
{
  if (outstream)
    {
      outstream->WriteEvent(evt);
    }
  return 0;
}

