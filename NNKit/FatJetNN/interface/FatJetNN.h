/*
 * FatJetNN.h
 *
 *  Created on: Jun 22, 2017
 *      Author: hqu
 */

#ifndef NNKIT_FATJETNN_INTERFACE_FATJETNN_H_
#define NNKIT_FATJETNN_INTERFACE_FATJETNN_H_

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "NNKit/CommonTools/interface/json.hpp"
#include "NNKit/CommonTools/interface/MXNetCppPredictor.h"

#include "NNKit/FatJetInputs/interface/PFCandidateFiller.h"
#include "NNKit/FatJetInputs/interface/SVFiller.h"

#include "NNKit/FatJetNN/interface/FatJetNNHelper.h"

namespace deepntuples {

class FatJetNN {
public:
  FatJetNN(const edm::ParameterSet& iConfig, edm::ConsumesCollector & cc, double jetR=0.8) : jetR_(jetR) {
    group_fillers_["pfcand"] = std::make_unique<PFCandidateFiller>("", jetR_);
    group_fillers_["sv"] = std::make_unique<SVFiller>("", jetR_);

    // read config, register variables
    for (const auto &p : group_fillers_) {
      p.second->readConfig(iConfig, cc);
      p.second->initBranches();
    }
  }
  virtual ~FatJetNN() {}

  static std::vector<float> center_norm_pad(const std::vector<float>& input,
      float center, float scale,
      unsigned target_length, float pad_value=0,
      float min=0, float max=-1);


  // read event content or event setup for each event
  void readEvent(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
    for (const auto &p : group_fillers_) p.second->readEvent(iEvent, iSetup);
  }

  // process input json
  void load_json(std::string filepath);

  // initialize DNN model
  void load_model(std::string model_file, std::string param_file);

  // run predictor
  const std::vector<float>& predict(const JetHelper &jet_helper);

  // get raw variable values of the current jet (needs to call predict first!)
  const std::vector<float>& getRawInputs(const std::string &group_name, const std::string &var_name) {
    auto* filler = group_fillers_.at(group_name).get();
    return filler->treeData().getMulti<float>(var_name);
  }

  // get the leading value
  float getOneRawInput(const std::string &group_name, const std::string &var_name) {
    const auto &v = getRawInputs(group_name, var_name);
    if (v.empty()) return 0;
    else return v.front();
  }

protected:
  // get and transform variables
  void make_inputs(const JetHelper &jet_helper);

  std::vector<std::string> input_names_; // names of each input group :: the ordering is important!
  std::vector<std::vector<unsigned int>> input_shapes_; // shapes of each input group
  std::unordered_map<std::string, std::vector<std::string>> var_names_; // group name -> variable names
  std::unordered_map<std::string, std::unique_ptr<NtupleBase>> group_fillers_; // group name -> variable filler
  std::vector<std::vector<float>> data_;
  nlohmann::json infos_;

  std::unique_ptr<mxnet::cpp::Block> block_;
  std::unique_ptr<mxnet::cpp::MXNetCppPredictor> predictor_;
  std::vector<float> pred_;
  double jetR_ = -1;

  bool debug_ = false;
};

// ---------------------------------------------------------------------------

} /* namespace deepntuples */

#endif /* NNKIT_FATJETNN_INTERFACE_FATJETNN_H_ */
