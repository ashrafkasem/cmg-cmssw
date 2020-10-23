/*
 * JetHelper.hh
 *
 *  Created on: Jan 27, 2017
 *      Author: hqu
 */

#ifndef NTUPLECOMMONS_INTERFACE_JETHELPER_H_
#define NTUPLECOMMONS_INTERFACE_JETHELPER_H_

#include "FWCore/Utilities/interface/Exception.h"
#include "DataFormats/PatCandidates/interface/Jet.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"

namespace deepntuples {

class JetHelper {
public:
  JetHelper() {}
  JetHelper(const pat::Jet *jet, bool has_puppi_weighted_daughters);

  virtual ~JetHelper() {}

  // ------

  // return jet constituents (PF candidates)
  const std::vector<const pat::PackedCandidate*>& getJetConstituents() const { return daughters_; }
  unsigned int numberOfDaughters() const { return daughters_.size(); }

  const pat::Jet& jet() const { return *jet_; }
  const std::vector<const pat::Jet*>& getSubJets() const {
    return subjets_;
  }
  bool hasPuppiWeightedDaughters() const { return has_puppi_weighted_daughters_; }

  std::pair<double, double> getCorrectedPuppiSoftDropMass(const std::vector<const pat::Jet*> &puppisubjets) const;

private:
  void initializeConstituents();


private:
  // data members
  const pat::Jet *jet_ = nullptr;
  bool has_puppi_weighted_daughters_ = false;
  std::vector<const pat::Jet*> subjets_;
  std::vector<const pat::PackedCandidate*> daughters_;

};

} /* namespace deepntuples */

#endif /* NTUPLECOMMONS_INTERFACE_JETHELPER_H_ */
