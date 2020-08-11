#ifndef __FILTERBASE_H__
#define __FILTERBASE_H__
// base 
#include <iostream>
#include <string>

class FilterBase
{
 public:
  //  FilterBase();
  virtual ~FilterBase() {}
  virtual const std::string &Name() {return ThisName;}
  virtual void identify(std::ostream &os = std::cout) const;
  
 protected:
  std::string ThisName;
};

#endif /* __FILTERBASE_H__ */
