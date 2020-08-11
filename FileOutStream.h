#ifndef __FILEOUTSTREAM_H__
#define __FILEOUTSTREAM_H__

#include <string>
#include "EventOutStream.h"

class Event;

class FileOutStream: public EventOutStream
{
 public:
  FileOutStream(const char *name = "out.prdf");
  virtual ~FileOutStream();
  int WriteFile(Event *evt);
  int CloseOutStream();

 protected:

};

#endif /* __FILEOUTSTREAM_H__ */
