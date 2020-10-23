/*
 * FatJetNNDecorrelator.h
 *
 *  Created on: Feb 8, 2018
 *      Author: hqu
 */

#ifndef FATJETNN_INTERFACE_FATJETNNDECORRELATOR_H_
#define FATJETNN_INTERFACE_FATJETNNDECORRELATOR_H_

#include <vector>
#include <unordered_map>
#include "NNKit/CommonTools/interface/MXNetCppPredictor.h"

namespace deepntuples {

class JetHelper;

class FatJetNNDecorrelator {
public:
  FatJetNNDecorrelator();
  FatJetNNDecorrelator(const std::string &input_name, unsigned num_vars);
  virtual ~FatJetNNDecorrelator();

  // initialize DNN model
  void load_model(std::string model_file, std::string param_file);

  // run the decorrelator
  const std::vector<float>& decorrelate(const std::vector<float>& scores, const JetHelper &jet_helper);

  // run the decorrelator
  // pt: uncorrected pt
  // mass: corrected Puppi softdrop mass
  const std::vector<float>& decorrelate(const std::vector<float>& scores, float pt, float mass);

protected:
  void make_inputs(const std::vector<float>& scores, float pt, float mass);

  std::vector<std::string> input_names_; // names of each input group :: the ordering is important!
  std::vector<std::vector<unsigned int>> input_shapes_; // shapes of each input group
  std::vector<std::vector<float>> data_;

  std::unique_ptr<mxnet::cpp::Block> block_;
  std::unique_ptr<mxnet::cpp::MXNetCppPredictor> predictor_;
  std::vector<float> pred_;

  bool debug_ = false;

};

} /* namespace deepntuples */

#endif /* FATJETNN_INTERFACE_FATJETNNDECORRELATOR_H_ */
