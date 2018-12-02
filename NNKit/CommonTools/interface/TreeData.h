/*
 * TreeData.h
 *
 *  Created on: May 23, 2017
 *      Author: hqu
 */

#ifndef NTUPLECOMMONS_INTERFACE_TREEDATA_H_
#define NTUPLECOMMONS_INTERFACE_TREEDATA_H_

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <cassert>
#include <typeinfo>
#include <typeindex>
#include <stdexcept>
#include "FWCore/Utilities/interface/Exception.h"

namespace deepntuples {

class AbstractTreeVar {
public:
  AbstractTreeVar(std::string name_) : name(name_) {}
  virtual ~AbstractTreeVar() {}
  virtual void reset() = 0;
  const std::string name;
};

template<typename Type>
class TreeVar : public AbstractTreeVar {
public:
  TreeVar(std::string name_, const Type defaultValue_)
  : AbstractTreeVar(name_), defaultValue(defaultValue_), value(defaultValue_)  { }
//  void fill() { value = defaultValue; }
  void fill(const Type var) { value = var; }
  void set(const Type var) { fill(var); }
  Type get() { return value; }
  void reset() { value = defaultValue; }
protected:
  const Type        defaultValue;
  Type              value;
};

template<typename Type>
class TreeMultiVar : public AbstractTreeVar {
public:
  TreeMultiVar(std::string name_)
    : AbstractTreeVar(name_) { }
//  void fill() {}
  void fill(const Type var) { value.push_back(var); }
  void set(const std::vector<Type>& vars) { value = vars; }
  void reset() { value.clear(); }
  const std::vector<Type>& get() { return value; }
protected:
  std::vector<Type> value;
};


class TreeData {
  using TypeMap = std::map<std::type_index, std::string>;
public:
  TreeData(std::string prefix = "") : prefix(prefix){}
  virtual ~TreeData() { }

  template<typename Type>
  void add(std::string name, const Type defaultValue){
    assert(!isBooked);
    auto fullname = fullName(name);
    if (data.find(fullname) != data.end()){
      throw cms::Exception("InvalidArgument") << "[TreeData::add] Variable w/ the same name has already been added: " << fullname;
    }
    try{
      data[fullname] = std::make_unique<TreeVar<Type>>(fullname, defaultValue);
    } catch (const std::out_of_range& e){
      throw cms::Exception("InvalidArgument") << std::string(typeid(Type).name()) + " is not a valid type for TTree!";
    }
  }

  template<typename Type>
  void addMulti(std::string name){
    assert(!isBooked);
    auto fullname = fullName(name);
    if (data.find(fullname) != data.end()){
      throw cms::Exception("InvalidArgument") << "[TreeData::addMulti] Variable w/ the same name has already been added: " << fullname;
    }
    data[fullname] = std::make_unique<TreeMultiVar<Type>>(fullname);
  }

  template<typename Type, typename FillType>
  void fill(std::string name, const FillType var){
    auto fullname = fullName(name);
    try {
      auto *tv=dynamic_cast<TreeVar<Type>*>(data.at(fullname).get());
      if (!tv) throw cms::Exception("InvalidArgument") << "[TreeData::fill] Filling a different type than registered: " << fullname;
      tv->fill(var);
    } catch (const std::out_of_range& e){
      throw cms::Exception("InvalidArgument") << "[TreeData::fill] Variable " << fullname << " is not registered!";
    }
  }
  template<typename Type, typename FillType>
  void fillMulti(std::string name, const FillType var){
    auto fullname = fullName(name);
    try {
      auto *tv=dynamic_cast<TreeMultiVar<Type>*>(data.at(fullname).get());
      if (!tv) throw cms::Exception("InvalidArgument") << "[TreeData::fillMulti] Filling a different type than registered: " << fullname;
      tv->fill(var);
    } catch (const std::out_of_range& e){
      throw cms::Exception("InvalidArgument") << "[TreeData::fillMulti] Variable " << name << " is not booked!";
    }
  }

  template<typename Type>
  void set(std::string name, const Type var) { fill<Type, Type>(name, var); }
  template<typename Type>
  void setMulti(std::string name, const std::vector<Type> &vec){
    auto fullname = fullName(name);
    try {
      auto *tv=dynamic_cast<TreeMultiVar<Type>*>(data.at(fullname).get());
      if (!tv) throw cms::Exception("InvalidArgument") << "[TreeData::setMulti] Filling a different type than registered: " << fullname;
      tv->set(vec);
    } catch (const std::out_of_range& e){
      throw cms::Exception("InvalidArgument") << "[TreeData::setMulti] Variable " << name << " is not booked!";
    }
  }

  template<typename Type>
  Type get(std::string name) const {
    auto fullname = fullName(name);
    try {
      auto *tv=dynamic_cast<TreeVar<Type>*>(data.at(fullname).get());
      if (!tv) throw cms::Exception("InvalidArgument") << "[TreeData::get] Wrong type: " << fullname;
      return tv->get();
    } catch (const std::out_of_range& e){
      throw cms::Exception("InvalidArgument") << "[TreeData::get] Variable " << fullname << " does not exist!";
    }
  }

  template<typename Type>
  const std::vector<Type>& getMulti(std::string name) const {
    auto fullname = fullName(name);
    try {
      auto *tv=dynamic_cast<TreeMultiVar<Type>*>(data.at(fullname).get());
      if (!tv) throw cms::Exception("InvalidArgument") << "[TreeData::getMulti] Wrong type: " << fullname;
      return tv->get();
    } catch (const std::out_of_range& e){
      throw cms::Exception("InvalidArgument") << "[TreeData::getMulti] Variable " << name << " does not exist!";
    }
  }


  void book(){
    isBooked = true;
  }

  void reset(){
    for(const auto &d : data) d.second->reset();
  }


protected:
  std::string fullName(std::string name) const {
    return prefix == "" ? name : (prefix + "_" + name);
  }

  bool isBooked = false;
  std::string prefix;
  std::map<std::string, std::unique_ptr<AbstractTreeVar>> data;

};


} /* namespace deepntuples */

#endif /* NTUPLECOMMONS_INTERFACE_TREEDATA_H_ */
