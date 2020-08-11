#ifndef __ODD_EVEN_EVENTTAGGER_H__
#define __ODD_EVEN_EVENTTAGGER_H__


#include <EventTagger.h>

/** This is a tagger that set the 2nd word to 0 or 1 depending
on whether the event number is odd or even. Just to show the recipe.

*/


class odd_even_EventTagger: public EventTagger
{

 public:
  odd_even_EventTagger()
    {
    }
  
  virtual ~odd_even_EventTagger()
    {
    }

  const char *idString() const 
    { 
      return "odd/even event tagger version 1.0";
    };
  int setPattern( int *evtData, int con[], int *nw);
  
 protected:
  
  
};



#endif /* __ODD_EVEN_EVENTTAGGER_H__ */
