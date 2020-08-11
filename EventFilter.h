#ifndef __EVENTFILTER_H__
#define __EVENTFILTER_H__

#include "FilterBase.h"

class EventOutStream;
class Event;

// return codes for EventOK method
static const int EVENTNOACC = 0;
static const int EVENTACC = 1;
static const int EVENTEXCEED = 2;


class EventFilter: public FilterBase
{
 public:
  EventFilter();
  virtual ~EventFilter() {}
  virtual int EventOk(unsigned int lvl1trigger, unsigned int lvl2trigger, unsigned int Evttype);
  virtual int WriteEvent(Event *evt);
  virtual int StreamStatus() {return 0;}
  virtual int SetOpt(const char *name, const char *opt) {return -1;}
  virtual int AddOutStream(EventOutStream *newstream) {outstream = newstream; return 0;}
  virtual void identify(std::ostream &os = std::cout) const;
  virtual void VerboseMod(const unsigned int i) {return;}
  virtual void MaxEvents(const unsigned int i) {return;}
  virtual int MaxEvents() {return -1;}

 protected:
  EventOutStream *outstream;
};

#endif /* __EVENTFILTER_H__ */
