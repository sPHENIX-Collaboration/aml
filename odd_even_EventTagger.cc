#include "odd_even_EventTagger.h"

int odd_even_EventTagger::setPattern( int  *evtData, int con[], int *nw)
{

  con[0] = evtData[1];
  con[1] = evtData[2] &1;
  *nw = 2;

  return 0;
}

odd_even_EventTagger Tag;

