/*
 * FatJetNNDecorrelator.cc
 *
 *  Created on: Feb 8, 2018
 *      Author: hqu
 */

#include <iostream>
#include <cmath>
#include <memory>
#include "NNKit/CommonTools/interface/JetHelper.h"
#include "NNKit/FatJetNN/interface/FatJetNNDecorrelator.h"

namespace deepntuples {

FatJetNNDecorrelator::FatJetNNDecorrelator() : FatJetNNDecorrelator("data", 20)
{
}

FatJetNNDecorrelator::FatJetNNDecorrelator(const std::string& input_name, unsigned num_vars) :
    input_names_({input_name}),
    input_shapes_({{1, num_vars}}) // (n_batch, n_vars)
{
}

FatJetNNDecorrelator::~FatJetNNDecorrelator() {
}

void FatJetNNDecorrelator::load_model(std::string model_file, std::string param_file) {
  if (debug_) {
    std::cerr << "Start loading model:\n..." << model_file << "\n..." << param_file << std::endl;
  }
  block_ = std::make_unique<mxnet::cpp::Block>(model_file, param_file);
  predictor_ = std::make_unique<mxnet::cpp::MXNetCppPredictor>(*block_);
  predictor_->set_input_shapes(input_names_, input_shapes_);
  if (debug_) {
    std::cerr << "Successfully loaded model." << std::endl;
  }

}

const std::vector<float>& FatJetNNDecorrelator::decorrelate(const std::vector<float>& scores, const JetHelper &jet_helper) {
  float uncorrpt = jet_helper.jet().correctedJet(0).pt();
  float msdcorr = jet_helper.getCorrectedPuppiSoftDropMass(jet_helper.getSubJets()).second;
  return decorrelate(scores, uncorrpt, msdcorr);
}

const std::vector<float>& FatJetNNDecorrelator::decorrelate(const std::vector<float>& scores, float pt, float mass) {
  make_inputs(scores, pt, mass);
  pred_ = predictor_->predict(data_);
  return pred_;
}

void FatJetNNDecorrelator::make_inputs(const std::vector<float>& scores, float pt, float mass) {
  data_.clear();
  data_.push_back({});
  auto &group_values = data_.back();

  // 0.1 * log(score)
  for (const auto &s : scores){
    group_values.push_back(0.1 * std::log(s));
  }

  // 0.001 * mass
  group_values.push_back(0.001 * mass);

  // 0.1 * log(pt)
  group_values.push_back(0.1 * std::log(pt));

  auto clip = [](float value, float low, float high){
    if (value < low) return low;
    if (value > high) return high;
    return value;
  };

  // 0.1 * rho (:= log(m^2/pt^2))
  // i.e., 0.2 * log ( clip(m,1,1000) / pt )
  group_values.push_back(0.2 * std::log( clip(mass, 1, 1000) / pt));

  if (debug_){
    std::cerr << "[Raw inputs] pt = " << pt << ", mass = " << mass << ", score = ";
    for (const auto &s : scores) std::cerr << s << ", ";
    std::cerr << std::endl;

    std::cerr << "[Processed inputs] (scores, mass, pt, rho): ";
    for (const auto &v : data_.back()) std::cerr << v << ", ";
    std::cerr << std::endl;
  }

}

} /* namespace deepntuples */

