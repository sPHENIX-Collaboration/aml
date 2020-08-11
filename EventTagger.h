#ifndef __EVENTTAGGER_H__
#define __EVENTTAGGER_H__

#include <Event.h>
#include <msg_control.h>

class EventTagger;

void et_register(EventTagger *, msg_control ** msg);
void et_unregister(EventTagger *);

/** This is the pure virtual parent class to assign the event
a type for the ET pool selection. 

Upon loading, it registers itself with the ET_SERVER, which provides
a msg_control class to be used in here. We are allowed to adjust the
severity.

The ET_SERVER then calls the getPattern function and this class fills 
the 4 words of info we can select on.

*/


class EventTagger
{

 public:
  EventTagger()
    {
      //cout << __FILE__ << " " << __LINE__ << " in  EventTagger()" << endl;
      et_register(this,  &msgCtrl);
      if (!msgCtrl)
	{ 
	  msgCtrl = new msg_control(MSG_TYPE_ONLINE, MSG_SOURCE_ET,
				    MSG_SEV_DEFAULT, "EventTagger");
	  our_msgcontrol = 1;
	}
      else
	{
	  our_msgcontrol = 0;
	}
    }
  
  virtual ~EventTagger()
    {
      //et_unregister(this);
      if (our_msgcontrol) delete msgCtrl;
    }

  virtual const char *idString() const { return " ";}
  virtual int setPattern( int *evtData, int con[], int *nw) = 0;
  
 protected:
  
  msg_control *msgCtrl;
  int our_msgcontrol;
  
};


#endif /* __EVENTTAGGER_H__ */
