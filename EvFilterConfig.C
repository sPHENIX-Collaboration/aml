#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <stdlib.h>
       
#include "EvFilterConfig.h"

using namespace std;

EvFilterConfig::EvFilterConfig(const string &conf_fname)
{
  ReadConfig(conf_fname);
}

int EvFilterConfig::ReadConfig(const string &conf_fname)
{
  cout << "EvFilterConf: Reading " << conf_fname << endl;

  ifstream infile(conf_fname.c_str());
  if (infile.fail ())
    {
      cout << "EvFilterConf::ReadConfig(), Failed to open file "
	   << conf_fname << endl;
      return 0;
    }

  string FullLine;	// a complete line in the config file
  string label;		// the label
  string name;
  unsigned int ival;
  // get one line first
  getline(infile, FullLine);

  while ( !infile.eof() )
    {
      // skip lines that begin with #
      if ( FullLine[0]=='#' || FullLine[0]==' ' )
        {
	  getline(infile,FullLine);
	  continue;
	}

      // make FullLine an istringstream
      istringstream line( FullLine.c_str() );

      // get label
      line >> label;

      // based on label, fill correct item
      if ( label == "EventType" )
	{
	  line >> ival;
	  _evttype.push_back(ival);
	}
      else if ( label == "FilterName" )
        {
	  line >> name;
          _filter_name.push_back(name);
	}
      else if ( label == "Level1Bits" )
        {
	  line >> hex >> ival;
          _level1_bits.push_back(ival);
	}
      else if ( label == "Level2Bits" )
        {
	  line >> hex >> ival;
          _level2_bits.push_back(ival);
	}
      else if ( label == "MaxEvents" )
	{
	  line >> ival;
	  _maxevents.push_back(ival);
	}
      else if ( label == "MachineName" )
	{
	  // just in case machine name is empty (no output stream)
	  if (line.str() == label)
	    {
	      name = "";
	    }
	  else
	    {
	      line >> name;
	    }
	  _machine_name.push_back(name);
	}
      else if ( label == "Port" )
	{
	  line >> ival;
	  _port.push_back(ival);
	}
      else if ( label == "Verbosity" )
	{
	  line >> ival;
	  if (ival <= 0)
	    {
	      ival = 1;
	    }
	  _verbmod.push_back(ival);
	}
      else
        {
	  // label was not understood
          cout << "EvFilterConf::ReadConfig(), can't grok: "
	       << line << endl;
	  // this is too important to miss
	  exit(1);
	}

      // get next line in file
      getline( infile, FullLine );
    }
  infile.close();
  if ( 
       _port.size() != _evttype.size() ||
       _port.size() != _filter_name.size() ||
       _port.size() != _level1_bits.size() ||
       _port.size() != _level2_bits.size() ||
       _port.size() != _machine_name.size() ||
       _port.size() != _maxevents.size() ||
       _port.size() != _port.size() ||
       _port.size() != _verbmod.size()
     )
    {
      cout << "Bad configuration file, number of entries for each item do not match" << endl;
      cout << "evttypes: " << _evttype.size() << endl;
      cout << "names: " << _filter_name.size() << endl;
      cout << "ports : " << _port.size() << endl;
      cout << "lvl1: " << _level1_bits.size() << endl;
      cout << "lvl2: " << _level2_bits.size() << endl;
      cout << "machine: " << _machine_name.size() << endl;
      cout << "maxevents: " << _maxevents.size() << endl;
      cout << "verbosity: " << _verbmod.size() << endl;

      exit(1);
    }

  //DumpConfig();

  return 1;
}

void EvFilterConfig::DumpConfig()
{
  cout << " **** EvFilter Configuration **** " << endl;
  for (unsigned int i = 0; i < Entries(); i++)
    {
      cout << "Filter Name " << GetFilterName(i) << endl;
      cout << "MachineName " << GetMachineName(i) << endl
       << "Port        " << GetPort(i) << endl
       << "Level1Bits  0x" << hex << GetLevel1Bits(i) << dec << endl
       << "Level2Bits  0x" << hex << GetLevel2Bits(i) << dec << endl
       << "MaxEvents   " << GetMaxEvents(i) << endl
       << "Verbosity   " << GetVerbosity(i) << endl
       << endl;
    }
}
