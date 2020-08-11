#include <iostream>
#include "FilterBase.h"

using namespace std;

void
FilterBase::identify(ostream &os) const
{
  os << "Virtual Filter Base class, identify not implemented by daughter class" << endl;
  return;
}
