#ifndef __AMLOUTSTREAM_H__
#define __AMLOUTSTREAM_H__

#include "EventOutStream.h"
#include "phenixTypes.h"
#include <iostream>
#include <string>

class Event;
class oBuffer;

static const unsigned int LENGTH = (4*1024*1024);

class AmlOutStream: public EventOutStream
{
 public:
  AmlOutStream(const std::string &hostname, const int port);
  virtual ~AmlOutStream();
  int WriteEvent(Event *evt);
  int CloseOutStream();
  void identify(std::ostream &os = std::cout) const;

 protected:

  std::string amlhost;
  int the_port;
  oBuffer *ob;
  PHDWORD xb[LENGTH];
};

#endif /* __AMLOUTSTREAM_H__ */
