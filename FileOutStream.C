#include <iostream>
#include "Event.h"
#include "FileOutStream.h"

using namespace std;

FileOutStream::FileOutStream(const char *name)
{
  ThisName = "FILE";
}

FileOutStream::~FileOutStream()
{
  return;
}

int FileOutStream::WriteFile(Event *evt)
{

  cout << "writing Event" << endl;
  return 0;
}

int FileOutStream::CloseOutStream()
{
  return 0;
}

