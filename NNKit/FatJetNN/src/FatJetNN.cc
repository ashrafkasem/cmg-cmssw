#include "NNKit/FatJetNN/interface/FatJetNN.h"
#include "FWCore/Utilities/interface/Exception.h"

namespace deepntuples {

void FatJetNN::load_json(std::string filepath) {
  std::ifstream infile;
  infile.open(filepath);
  infile >> infos_;
  infile.close();

  input_shapes_.clear();
  var_names_.clear();

  input_names_ = infos_["input_names"].get<std::vector<std::string>>();
  for (const auto &group_name : input_names_){
    input_shapes_.push_back(infos_["input_shapes"][group_name].get<std::vector<unsigned int>>());
    var_names_[group_name] = infos_["var_names"][group_name].get<std::vector<std::string>>();

    if (debug_) {
      std::cerr << group_name << "\nshapes: ";
      for (const auto &x : input_shapes_.back()){
        std::cerr << x << ", ";
      }
      std::cerr << "\nvariables: ";
      for (const auto &x : var_names_[group_name]){ //
        std::cerr << x << ", "; //
      } //
      std::cerr << "\n";
    }

  }


}

void FatJetNN::load_model(std::string model_file, std::string param_file) {
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

const std::vector<float>& FatJetNN::predict(const JetHelper &jet_helper) {
  if (jet_helper.getJetConstituents().empty()){
    // no need to run the NN if fatjet has no constituents saved (e.g., low pt AK8 jets in MiniAOD)
    pred_ = std::vector<float>(FatJetNNHelper::NUM_CATEGORIES, 0);
  }else{
    make_inputs(jet_helper);
    pred_ = predictor_->predict(data_);
  }
  return pred_;
}

//void FatJetNN::make_inputs(const pat::Jet& jet) {
//  data_.clear();
//  for (const auto &group_name : input_names_) {
//    // initiate with an empty vector
//    data_.push_back({});
//    auto &group_values = data_.back();
//    // transform/pad
//    for (const auto &varname : var_names_.at(group_name)){
//      const std::vector<float> raw_value;
//      const auto &info = infos_["var_info"][varname];
//      unsigned size = info["size"].get<unsigned>();
//      float pad = 0;
//      const auto val = center_norm_pad(raw_value, 0, 1, size, pad, -5, 5);
//      group_values.insert(group_values.end(), val.begin(), val.end());
//    }
//  }
//}

void FatJetNN::make_inputs(const JetHelper &jet_helper) {
  data_.clear();
  const auto jet = jet_helper.jet().correctedJet("Uncorrected");
  for (const auto &group_name : input_names_) {
    auto *filler = group_fillers_.at(group_name).get();
    // initiate with an empty vector
    data_.push_back({});
    auto &group_values = data_.back();
    // compute the variables
    filler->fillBranches(jet, 0, jet_helper);
    // transform/pad
    for (const auto &varname : var_names_.at(group_name)){
      const auto &raw_value = filler->treeData().getMulti<float>(varname);
      const auto &info = infos_["var_info"][varname];
      float center = info["median"].get<float>();
      float scale = info["upper"].get<float>() - center;
      if (scale==0) { scale = 1; }
      unsigned size = info["size"].get<unsigned>();
//      float pad = (info["min"].get<float>() - scale - center)/scale; // pad with min_scaled - 1
      float pad = 0; // pad w/ zero
      const auto val = center_norm_pad(raw_value, center, scale, size, pad, -5, 5);
      group_values.insert(group_values.end(), val.begin(), val.end());

      if (debug_){
        std::cerr << " -- var=" << varname << ", center=" << center << ", scale=" << scale << ", pad=" << pad << std::endl;
        std::cerr << "values (first 3 and last 3): " << val.at(0) << ", " << val.at(1) << ", " << val.at(2) << " ... "
            << val.at(size-3) << ", " << val.at(size-2) << ", " << val.at(size-1) << std::endl;
      }

    }
  }
}

std::vector<float> FatJetNN::center_norm_pad(
    const std::vector<float>& input, float center, float scale,
    unsigned target_length, float pad_value, float min, float max) {
  // do variable shifting/scaling/padding/clipping in one go

  auto clip = [](float value, float low, float high){
    if (low >= high) throw cms::Exception("InvalidArgument") << "Error in clip: low >= high!";
    if (value < low) return low;
    if (value > high) return high;
    return value;
  };

  pad_value = clip(pad_value, min, max);
  std::vector<float> out(target_length, pad_value);
  for (unsigned i=0; i<input.size() && i<target_length; ++i){
    out.at(i) = (input.at(i) - center) / scale;
    if (min < max) out.at(i) = clip(out.at(i), min, max);
  }
  return out;
}

} /* namespace deepntuples */

