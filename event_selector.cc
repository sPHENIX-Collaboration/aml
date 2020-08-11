/* 
functions to select specific events via user function
ET finds these user functions in a shared library  via dlsym so never 
use the C++ compiler with name mangling. You'll never pick the right function 
name (or you have to search for something like 
select_scaler_events__FPviP10et_event_t).
*/

#include "et.h"
#include "EventTypes.h"

extern "C" 
{
int SelectScaler(et_sys_id id, et_stat_id stat_id, et_event *pe)
{
  switch (pe->control[0])
    {
    case SCALEREVENT:
    case BEGRUNEVENT:
    case ENDRUNEVENT:
      return 1;
    default:
      return 0;
    }
}

int SelectPPG(et_sys_id id, et_stat_id stat_id, et_event *pe)
{
  return (pe->control[1] & 0x70000000);
}

int SelectData(et_sys_id id, et_stat_id stat_id, et_event *pe)
{
  return 1;
  switch (pe->control[0])
    {
    case DATAEVENT:
    case REJECTEDEVENT:
    case BEGRUNEVENT:
    case ENDRUNEVENT:
      return 1;
    default:
      return 0;
    }
}
}
