#ifndef __GL1_EVENTTAGGER_H__
#define __GL1_EVENTTAGGER_H__


#include <EventTagger.h>

/** This is a tagger that set the 2nd word to 0 or 1 depending
on whether the event number is odd or even. Just to show the recipe.

*/


class gl1_EventTagger: public EventTagger
{

 public:
  gl1_EventTagger()
    {
    }
  
  virtual ~gl1_EventTagger()
    {
    }

  const char *idString() const 
    { 
      return "gl1  event tagger version 1.0";
    };
  int setPattern( int *evtData, int con[], int *nw);
  
 protected:
  
  
};



#endif /* __GL1_EVENTTAGGER_H__ */
