#ifndef __EVENTOUTSTREAM_H__
#define __EVENTOUTSTREAM_H__

#include <string>

class EventOutStream
{
 public:
  //  EventOutStream();
  virtual ~EventOutStream();
  virtual int WriteEvent(Event *evt) {return 0;}
  virtual int CloseOutStream() {return 0;}

 protected:
  string ThisName;
};

#endif /* __EVENTOUTSTREAM_H__ */
