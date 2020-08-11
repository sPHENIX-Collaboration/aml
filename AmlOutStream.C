#include "AmlOutStream.h"

#include "oamlBuffer.h"
#include "Event.h"

#include <iostream>
#include <sstream>

using namespace std;

AmlOutStream::AmlOutStream(const string &hostname, const int port)
{
  ostringstream nameparse;
  nameparse << "AML@" << hostname << "::" << port << ends;
  ThisName = nameparse.str();
  amlhost = hostname;
  the_port = port;
  ob = NULL;
}

AmlOutStream::~AmlOutStream()
{
  if (ob)
    {
      delete ob;
    }
  return;
}

int AmlOutStream::WriteEvent(Event *evt)
{
  if (! ob)
    {
      int irun = evt->getRunNumber();
      cout << "Creating new oamlbuffer on " 
	   << amlhost
	   << " on port " << the_port << endl;
      ob = new oamlBuffer ( amlhost.c_str(), the_port, xb, LENGTH, irun);
    }
  
  int status = ob->addEvent(evt);
  if (status)
    {
      cout << ThisName << ": ERROR WRITING OUT FILTERED EVENT "
	   << evt->getEvtSequence() << " FOR RUN "
	   << evt->getRunNumber() << " Status: " << status << endl;
    }
  return 0;
}

int AmlOutStream::CloseOutStream()
{
  if (ob)
    {
      delete ob;
      ob = NULL;
    }
  return 0;
}

void
AmlOutStream::identify(ostream &os) const
{
  os << "AmlOutStream writing to " << amlhost
     << ", on port " << the_port << endl;
  return;
}
