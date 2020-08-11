#ifndef __ETOUTSTREAM_H__
#define __ETOUTSTREAM_H__

#include <string>
#include "et.h"
#include "EventOutStream.h"

class Event;

class EtOutStream: public EventOutStream
{
 public:
  EtOutStream(const char *etname);
  virtual ~EtOutStream();
  int StreamStatus() {return et_ok;}
  int WriteEvent(Event *evt);
  int CloseOutStream();

 protected:
  std::string ThisName;
  int et_ok;
  et_att_id	  attach1;
  et_sys_id       id;
  et_openconfig   openconfig;
  et_event       *pe;
  
};

#endif /* __ETOUTSTREAM_H__ */
