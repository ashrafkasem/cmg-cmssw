/*
 * DeepNtuplizerAK8AK8.cc
 *
 *  Created on: May 24, 2017
 *      Author: hqu
 */

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "DataFormats/PatCandidates/interface/Jet.h"

#include <fstream>
#include <memory>
#include "NNKit/FatJetNN/interface/FatJetNN.h"
#include "NNKit/FatJetNN/interface/FatJetNNDecorrelator.h"
#include "NNKit/FatJetNN/interface/FatJetNNHelper.h"
using namespace deepntuples;

class FatJetNNTestAnalyzer : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
  explicit FatJetNNTestAnalyzer(const edm::ParameterSet&);
  ~FatJetNNTestAnalyzer();

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  virtual void beginJob() override;
  virtual void analyze(const edm::Event&, const edm::EventSetup&) override;
  virtual void endJob() override;

  edm::EDGetTokenT<edm::View<pat::Jet>> jetToken_;
  double jetR_ = 0.8;
  bool has_puppi_weighted_daughters_;
  std::string datapath_;
  std::string output_;
  int decorrMode = 0; // 0: from the jet inputs; 1: from the scores


  std::unique_ptr<FatJetNN> fatjetNN_;
  std::unique_ptr<FatJetNN> decorrNN_;
  std::unique_ptr<FatJetNNDecorrelator> fatjetNNDecorr_;
  std::ofstream fout_;
};

FatJetNNTestAnalyzer::FatJetNNTestAnalyzer(const edm::ParameterSet& iConfig):
    jetToken_(consumes<edm::View<pat::Jet> >(iConfig.getUntrackedParameter<edm::InputTag>("jets", edm::InputTag("slimmedJetsAK8")))),
    jetR_(iConfig.getUntrackedParameter<double>("jetR", 0.8)),
    has_puppi_weighted_daughters_(iConfig.getParameter<bool>("hasPuppiWeightedDaughters")),
    datapath_(iConfig.getUntrackedParameter<std::string>("datapath", "NNKit/data/ak8")),
    output_(iConfig.getUntrackedParameter<std::string>("output", "output.md")),
    decorrMode(iConfig.getUntrackedParameter<int>("decorrMode", 0))
{
  // initialize the FatJetNN class in the constructor
  auto cc = consumesCollector();
  fatjetNN_ = std::make_unique<FatJetNN>(iConfig, cc, jetR_);
  // load json for input variable transformation
  fatjetNN_->load_json(edm::FileInPath(datapath_+"/full/preprocessing.json").fullPath());
  // load DNN model and parameter files
  fatjetNN_->load_model(edm::FileInPath(datapath_+"/full/resnet-symbol.json").fullPath(),
      edm::FileInPath(datapath_+"/full/resnet.params").fullPath());

  std::cout << "====== Decorrelation mode = " << decorrMode << " ======" << std::endl;
  if (decorrMode==0){
    decorrNN_ = std::make_unique<FatJetNN>(iConfig, cc, jetR_);
    // load json for input variable transformation
    decorrNN_->load_json(edm::FileInPath(datapath_+"/decorrelated/preprocessing.json").fullPath());
    // load DNN model and parameter files
    decorrNN_->load_model(edm::FileInPath(datapath_+"/decorrelated/resnet-symbol.json").fullPath(),
        edm::FileInPath(datapath_+"/decorrelated/resnet.params").fullPath());
  }else if (decorrMode==1){
    fatjetNNDecorr_ = std::make_unique<FatJetNNDecorrelator>();
    fatjetNNDecorr_->load_model(edm::FileInPath(datapath_+"/decorrelated/decorr-symbol.json").fullPath(),
        edm::FileInPath(datapath_+"/decorrelated/decorr.params").fullPath());
  }

  auto printHeaderMD = [&](){
    fout_ << "| event | jet # | pt | eta | phi | nn version | `binarized_score_top` | `binarized_score_w` | `flavor_score_bb_no_gluon` | `raw_score_qcd` |";
    for (unsigned i=0; i<FatJetNNHelper::NUM_CATEGORIES; ++i){
      fout_ << " `" << FatJetNNHelper::categoryName(static_cast<FatJetNNHelper::FatJetNNCategory>(i)) << "` |";
    }
    fout_ << std::endl;
    fout_ << "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |";
    for (unsigned i=0; i<FatJetNNHelper::NUM_CATEGORIES; ++i){
      fout_ << " --- |";
    }
    fout_ << std::endl;
  };

  auto printHeaderCSV = [&](){
    fout_ << "run,lumi,event,jet_no,pt,eta,phi,nn_version,binarized_score_top,binarized_score_w,flavor_score_bb_no_gluon,raw_score_qcd";
    for (unsigned i=0; i<FatJetNNHelper::NUM_CATEGORIES; ++i){
      fout_ << "," << FatJetNNHelper::categoryName(static_cast<FatJetNNHelper::FatJetNNCategory>(i));
    }
    fout_ << std::endl;
  };

  if(!output_.empty()){
    fout_.open(output_);
    // print header
    if (output_.find(".md") != std::string::npos){
      printHeaderMD();
    } else {
      printHeaderCSV();
    }
  }
}

FatJetNNTestAnalyzer::~FatJetNNTestAnalyzer(){
  if(fout_.is_open()){
    fout_.close();
  }
}

// ------------ method called for each event  ------------
void FatJetNNTestAnalyzer::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {

  // need to access event info
  fatjetNN_->readEvent(iEvent, iSetup);
  if (decorrMode==0) decorrNN_->readEvent(iEvent, iSetup);

  // loop over the jets
  edm::Handle<edm::View<pat::Jet>> jets;
  iEvent.getByToken(jetToken_, jets);

  auto printJetMD = [&](bool decorrVersion, unsigned idx, const pat::Jet &jet, const std::vector<float> &scores){
    if (!decorrVersion){
      if (idx==0){
        fout_ << "| " << iEvent.id().run() << ":" << iEvent.id().luminosityBlock() << ":" << iEvent.id().event() << " | ";
      }else{
        fout_ << "|    | ";
      }
      fout_ << idx << " | " << jet.pt() << " | " << jet.eta() << " | " << jet.phi() << " | " << " Nominal " << " | ";
    }else{
      fout_ << "|  |  |  |  |  | _Decorrelated_ | ";
    }
    FatJetNNHelper nn(scores);
    fout_ << nn.get_binarized_score_top() << " | "
        << nn.get_binarized_score_w() << " | "
        << nn.get_flavor_score_bb_no_gluon() << " | "
        << nn.get_raw_score_qcd() << " | ";
    for (const auto &v : scores){
      fout_ << v << " | ";
    }
    fout_ << std::endl;
  };

  auto printJetCSV = [&](bool decorrVersion, unsigned idx, const pat::Jet &jet, const std::vector<float> &scores){
    fout_ << iEvent.id().run() << "," << iEvent.id().luminosityBlock() << "," << iEvent.id().event() << ","
          << idx << "," << jet.pt() << "," << jet.eta() << "," << jet.phi() << "," << (decorrVersion ? "Decorrelated" : "Nominal") << ",";
    FatJetNNHelper nn(scores);
    fout_ << nn.get_binarized_score_top() << ","
        << nn.get_binarized_score_w() << ","
        << nn.get_flavor_score_bb_no_gluon() << ","
        << nn.get_raw_score_qcd();
    for (const auto &v : scores){
      fout_ << "," << v;
    }
    fout_ << std::endl;
  };

  auto printJet = [&](bool decorrVersion, unsigned idx, const pat::Jet &jet, const std::vector<float> &scores){
    if (output_.find(".md") != std::string::npos){
      printJetMD(decorrVersion, idx, jet, scores);
    } else {
      printJetCSV(decorrVersion, idx, jet, scores);
    }
  };

  for (unsigned idx=0; idx<jets->size(); ++idx){
    const auto &jet = jets->at(idx);
    JetHelper jet_helper(&jet, has_puppi_weighted_daughters_);

    // get the NN predictions (nominal version)
    const auto& nnpreds = fatjetNN_->predict(jet_helper);
    printJet(false, idx, jet, nnpreds);

    // get the NN predictions (decorrelated version)
    if (decorrMode == 0){
      const auto& mdpreds = decorrNN_->predict(jet_helper);
      printJet(true, idx, jet, mdpreds);
    }else if (decorrMode == 1){
      const auto& mdpreds = fatjetNNDecorr_->decorrelate(nnpreds, jet_helper);
      printJet(true, idx, jet, mdpreds);
    }

  }

}


// ------------ method called once each job just before starting event loop  ------------
void FatJetNNTestAnalyzer::beginJob() {
}

// ------------ method called once each job just after ending the event loop  ------------
void FatJetNNTestAnalyzer::endJob() {
}

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void FatJetNNTestAnalyzer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  //The following says we do not know what parameters are allowed so do no validation
  // Please change this to state exactly what you do use, even if it is no parameters
  edm::ParameterSetDescription desc;
  desc.setUnknown();
  descriptions.addDefault(desc);
}

//define this as a plug-in
DEFINE_FWK_MODULE(FatJetNNTestAnalyzer);
