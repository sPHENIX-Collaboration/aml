#include <iostream>
#include <unistd.h>
#include "Event.h"
#include "EtOutStream.h"

EtOutStream::EtOutStream(const char *name)
{
  et_ok = 0;
  et_open_config_init(&openconfig);

  et_open_config_sethost(openconfig, ET_HOST_ANYWHERE);

  if (et_open(&id, name, openconfig) != ET_OK)
    {
      std::cout << "could not open the ET pool " << name << std::endl;
      et_ok = 1;
    }
  et_open_config_destroy(openconfig);


  /* set level of debug output (everything) */
  et_system_setdebug(id, ET_DEBUG_INFO);

  /* attach to grandcentral station */
  if (et_station_attach(id, ET_GRANDCENTRAL, &attach1) < 0)
    {
      std::cout << "error in et_station_attach" << std::endl;
      et_ok = 2;
    }
  std::cout << "ET connected and ready to take data" << std::endl;
  return ;
}

EtOutStream::~EtOutStream()
{
 CloseOutStream();
 return;
}

int EtOutStream::WriteEvent(Event *evt)
{
    char *pdata;
  // evt->getEvtLength() is words, et needs length in bytes (len*4)
  // <<2 is same as *4
  int lenbytes = (evt->getEvtLength())<<2;
  if (et_event_new(id, attach1, &pe, ET_SLEEP, NULL, lenbytes) < 0)
    {
      std::cout << "Could not get new event slot" << std::endl;
      return -1;
    }
  et_event_getdata(pe, (void **) &pdata);
  int nw;
  evt->Copy((int *) pdata, evt->getEvtLength(), &nw);
  
  et_event_setlength(pe, lenbytes);
  et_event_put(id, attach1, pe);


  return 0;
}

int EtOutStream::CloseOutStream()
{
      et_station_detach(id, attach1);
      sleep(2);
      return 0;
}
