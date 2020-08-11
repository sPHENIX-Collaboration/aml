#ifndef __EVENTOUTSTREAM_H__
#define __EVENTOUTSTREAM_H__

#include "FilterBase.h"

class Event;

class EventOutStream: public FilterBase
{
 public:
  //  EventOutStream();
  virtual ~EventOutStream() {}
  virtual int StreamStatus() {return 0;}
  virtual int WriteEvent(Event *evt) {return 0;}
  virtual int CloseOutStream() {return 0;}
  
 protected:
};

#endif /* __EVENTOUTSTREAM_H__ */
