#ifndef __EVFILTER_CONFIG_H__
#define __EVFILTER_CONFIG_H__

#include <string>
#include <vector>

class EvFilterConfig
{
public:
  EvFilterConfig(const std::string &conf_fname = "evfilter.conf");
  virtual ~EvFilterConfig() {}

  void DumpConfig();
  unsigned int Entries() const {return _port.size();}
  unsigned int GetPort(const int i) const { return _port[i]; }
  const std::string& GetMachineName(const int i) const { return _machine_name[i]; }
  const std::string& GetFilterName(const int i) const { return _filter_name[i]; }
  unsigned int GetLevel1Bits(const int i) const { return _level1_bits[i]; }
  unsigned int GetLevel2Bits(const int i) const { return _level2_bits[i]; }
  unsigned int GetEventType(const int i) const { return _evttype[i]; }
  unsigned int GetVerbosity(const int i) const { return _verbmod[i]; }
  unsigned int GetMaxEvents(const int i) const { return _maxevents[i]; }

private:
  int ReadConfig(const std::string &conf_fname);
  std::vector<unsigned int> _evttype;
  std::vector<std::string> _filter_name;
  std::vector<unsigned int> _level1_bits;
  std::vector<unsigned int> _level2_bits;
  std::vector<std::string> _machine_name;
  std::vector<unsigned int> _maxevents;
  std::vector<unsigned int> _port;
  std::vector<unsigned int> _verbmod;
};

#endif	// __EVFILTER_CONFIG_H__
