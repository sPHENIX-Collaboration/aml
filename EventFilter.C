#include "EventFilter.h"
#include "EventOutStream.h"
#include <iostream>

using namespace std;

EventFilter::EventFilter()
{
  ThisName = "VIRTUAL FILTER";
  outstream = NULL;
}

void
EventFilter::identify(ostream &os) const
{
  os << "EventFilter " << ThisName
     << ", Outstream: " << endl;
  if (outstream)
    {
      outstream->identify(os);
    }
  else
    {
      os << " No Output stream" << endl;
    }
  return;
}

int 
EventFilter::EventOk(unsigned int lvl1trigger, unsigned int lvl2trigger, unsigned int Evttype)
{
  cout << "USING VIRTUAL EventOK method BAAAD, implement it in daughter class" 
       << endl;
  return 0;
}

int 
EventFilter::WriteEvent(Event *evt)
{
  cout << "Virtual WriteEvent method, event is not saved" << endl;
  return 0;
}
