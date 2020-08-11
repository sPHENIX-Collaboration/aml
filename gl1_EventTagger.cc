#include "gl1_EventTagger.h"

int gl1_EventTagger::setPattern( int *evtData, int con[], int *nw)
{

  con[0] = evtData[1];  //set the first word to the event type
  con[1] = evtData[6];  //set the second word to the (EVB-supplied) TAG word

  *nw = 2;

  return 0;
}

gl1_EventTagger Tag;
