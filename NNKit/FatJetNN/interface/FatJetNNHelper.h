/*
 * FatJetNNHelper.h
 *
 *  Created on: Feb 8, 2018
 *      Author: hqu
 */

#ifndef FATJETNN_INTERFACE_FATJETNNHELPER_H_
#define FATJETNN_INTERFACE_FATJETNNHELPER_H_

#include <vector>
#include <string>
#include "FWCore/Utilities/interface/Exception.h"

namespace deepntuples {

class FatJetNNHelper {
public:
  FatJetNNHelper() {}
  FatJetNNHelper(const std::vector<float>& nn) : nn(nn) {
    if (nn.size() != FatJetNNCategory::NUM_CATEGORIES)
      throw cms::Exception("RuntimeError") << "[FatJetNNHelper] Prediction array should have size " << std::to_string(FatJetNNCategory::NUM_CATEGORIES) << " but got " << std::to_string(nn.size());
  }
  ~FatJetNNHelper() {}

  enum FatJetNNCategory {
    label_Top_bcq,
    label_Top_bqq,
    label_Top_bc,
    label_Top_bq,
    label_W_cq,
    label_W_qq,
    label_Z_bb,
    label_Z_cc,
    label_Z_qq,
    label_H_bb,
    label_H_cc,
    label_H_qqqq,
    label_QCD_bb,
    label_QCD_cc,
    label_QCD_b,
    label_QCD_c,
    label_QCD_others,
    NUM_CATEGORIES,
  };

  static std::string categoryName(FatJetNNCategory category){
    std::vector<std::string> catnames {
      "Top_bcq",
      "Top_bqq",
      "Top_bc",
      "Top_bq",
      "W_cq",
      "W_qq",
      "Z_bb",
      "Z_cc",
      "Z_qq",
      "H_bb",
      "H_cc",
      "H_qqqq",
      "QCD_bb",
      "QCD_cc",
      "QCD_b",
      "QCD_c",
      "QCD_others",
      "",
    };
    return catnames.at(category);
  }

  float get(FatJetNNCategory category) {
    return nn.at(category);
  }

  float get_raw_score_qcd() {
    return nn.at(label_QCD_b)+nn.at(label_QCD_bb)+nn.at(label_QCD_c)+nn.at(label_QCD_cc)+nn.at(label_QCD_others);
  }

  float get_raw_score_top() {
    return nn.at(label_Top_bcq)+nn.at(label_Top_bqq);
  }

  float get_raw_score_w() {
    return nn.at(label_W_cq)+nn.at(label_W_qq);
  }

  float get_raw_score_z() {
    return nn.at(label_Z_bb)+nn.at(label_Z_cc)+nn.at(label_Z_qq);
  }

  float get_raw_score_zbb() {
    return nn.at(label_Z_bb);
  }

  float get_raw_score_zcc() {
    return nn.at(label_Z_cc);
  }

  float get_raw_score_hbb() {
    return nn.at(label_H_bb);
  }

  float get_raw_score_hcc() {
    return nn.at(label_H_cc);
  }

  float get_raw_score_h4q() {
    return nn.at(label_H_qqqq);
  }

  float get_binarized_score_top() {
    return divide_filter_zero(get_raw_score_top(), get_raw_score_top()+get_raw_score_qcd());
  }

  float get_binarized_score_w() {
    return divide_filter_zero(get_raw_score_w(), get_raw_score_w()+get_raw_score_qcd());
  }

  float get_binarized_score_z() {
    return divide_filter_zero(get_raw_score_z(), get_raw_score_z()+get_raw_score_qcd());
  }

  float get_binarized_score_zbb() {
    return divide_filter_zero(get_raw_score_zbb(), get_raw_score_zbb()+get_raw_score_qcd());
  }

  float get_binarized_score_zcc() {
    return divide_filter_zero(get_raw_score_zcc(), get_raw_score_zcc()+get_raw_score_qcd());
  }

  float get_binarized_score_hbb() {
    return divide_filter_zero(get_raw_score_hbb(), get_raw_score_hbb()+get_raw_score_qcd());
  }

  float get_binarized_score_hcc() {
    return divide_filter_zero(get_raw_score_hcc(), get_raw_score_hcc()+get_raw_score_qcd());
  }

  float get_binarized_score_h4q() {
    return divide_filter_zero(get_raw_score_h4q(), get_raw_score_h4q()+get_raw_score_qcd());
  }

  // flavor score: bb vs cc vs others
  float get_flavor_score_bb() {
    return divide_filter_zero(
        nn.at(label_H_bb)+nn.at(label_Z_bb)+nn.at(label_QCD_bb),
        nn.at(label_H_bb)+nn.at(label_Z_bb)+nn.at(label_H_cc)+nn.at(label_Z_cc) + get_raw_score_qcd()
      );
  }

  float get_flavor_score_cc() {
    return divide_filter_zero(
        nn.at(label_H_cc)+nn.at(label_Z_cc)+nn.at(label_QCD_cc),
        nn.at(label_H_bb)+nn.at(label_Z_bb)+nn.at(label_H_cc)+nn.at(label_Z_cc) + get_raw_score_qcd()
      );
  }

  float get_flavor_score_bb_no_gluon() {
    return divide_filter_zero(
        nn.at(label_H_bb)+nn.at(label_Z_bb),
        nn.at(label_H_bb)+nn.at(label_Z_bb)+nn.at(label_H_cc)+nn.at(label_Z_cc) + get_raw_score_qcd()
      );
  }

  float get_flavor_score_cc_no_gluon() {
    return divide_filter_zero(
        nn.at(label_H_cc)+nn.at(label_Z_cc),
        nn.at(label_H_bb)+nn.at(label_Z_bb)+nn.at(label_H_cc)+nn.at(label_Z_cc) + get_raw_score_qcd()
      );
  }

private:
  static float divide_filter_zero(float numerator, float denominator, float fallback=-10.) {
    return (denominator != 0 && numerator >= 0) ? numerator / denominator : fallback;
  }

  std::vector<float> nn;
};


} /* namespace deepntuples */

#endif /* FATJETNN_INTERFACE_FATJETNNHELPER_H_ */
