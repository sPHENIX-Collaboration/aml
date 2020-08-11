#ifndef __TRIGGERFILTER_H__
#define __TRIGGERFILTER_H__

#include <string>
#include "EventFilter.h"
#include "EventOutStream.h"

class Event;

class TriggerFilter: public EventFilter
{
 public:
  //  TriggerFilter();
  TriggerFilter(const std::string &myname = "TRIGGER");
  TriggerFilter(const std::string &myname, const unsigned int lvl1trig = 0x0, const unsigned int lvl2trig = 0x0, const unsigned int type = 0x0);
  virtual ~TriggerFilter();
  int StreamStatus() {return outstream->StreamStatus();}
  int EventOk(unsigned int lvl1trigger, unsigned int lvl2trigger, unsigned int Evttype);
  int WriteEvent(Event *evt);
  void EventType(const unsigned int i) {evttype = i;}
  void Lvl1TrigBits(const unsigned int i) {lvl1trigbits = i;}
  void Lvl2TrigBits(const unsigned int i) {lvl2trigbits = i;}
  void MaxEvents(const unsigned int i) {maxevents = i;}
  int MaxEvents() {return maxevents;}
  void VerboseMod(const unsigned int i) {verbosemod = i;}

 protected:
  unsigned int lvl1trigbits;
  unsigned int lvl2trigbits;
  unsigned int ntrig;
  unsigned int evttype;
  unsigned int verbosemod;
  unsigned int maxevents;
};

#endif /* __TRIGGERFILTER_H__ */
