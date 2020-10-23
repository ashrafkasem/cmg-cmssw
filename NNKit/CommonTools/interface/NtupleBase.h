/*
 * NtupleBase.h
 *
 *  Created on: May 23, 2017
 *      Author: hqu
 */

#ifndef NTUPLECOMMONS_INTERFACE_NTUPLEBASE_H_
#define NTUPLECOMMONS_INTERFACE_NTUPLEBASE_H_

#include <cassert>

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"

#include "NNKit/CommonTools/interface/TreeData.h"
#include "NNKit/CommonTools/interface/JetHelper.h"
#include "NNKit/CommonTools/interface/InfinityCatcher.h"

namespace deepntuples {

class NtupleBase {
public:
  NtupleBase() {}
  NtupleBase(std::string branchName, double jetR) :
    branchName_(branchName), jetR_(jetR), data(branchName_) {}
  virtual ~NtupleBase() {}

  // initilize the modules (read configuration and get the tokens)
  virtual void readConfig(const edm::ParameterSet& iConfig, edm::ConsumesCollector & cc) {}

  // read event content or event setup for each event
  virtual void readEvent(const edm::Event& iEvent, const edm::EventSetup& iSetup) {}

  // register the data branches to the tree
  void initBranches() {
    book();
    data.book();
  }

  // fill the branches
  bool fillBranches(const pat::Jet &jet, size_t jetidx, const JetHelper &jet_helper) {
    data.reset();
    return fill(jet, jetidx, jet_helper);
  }

  // return tree data for reading
  const TreeData& treeData() const { return data; }

protected:
  // declare the data branches (name, type, default values)
  virtual void book() = 0;
  // fill the data branches to the tree
  virtual bool fill(const pat::Jet &jet, size_t jetidx, const JetHelper &jet_helper) = 0;

protected:
  std::string branchName_;
  double jetR_ = -1;
  TreeData data;

};

} /* namespace deepntuples */

#endif /* NTUPLECOMMONS_INTERFACE_NTUPLEBASE_H_ */
