// -*- C++ -*-
//
// Package:    NNKit/FatJetNN
// Class:      MyStuffProducer
// 
/**\class MyStuffProducer MyStuffProducer.cc NNKit/FatJetNN/plugins/MyStuffProducer.cc

 Description: [one line class summary]

 Implementation:
     [Notes on implementation]
*/
//
// Original Author:  Pantelis Kontaxakis
//         Created:  Mon, 05 Mar 2018 14:00:49 GMT
//
//


// system include files
#include <memory>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/stream/EDProducer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/StreamID.h"
#include <vector>
#include "FWCore/Utilities/interface/InputTag.h"
#include <fstream>
#include "NNKit/FatJetNN/interface/FatJetNN.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "DataFormats/PatCandidates/interface/Jet.h"
#include "FWCore/Utilities/interface/EDMException.h"
#include "DataFormats/PatCandidates/interface/CompositeCandidate.h"
#include "DataFormats/Common/interface/Handle.h"

#include "NNKit/FatJetNN/interface/FatJetNNDecorrelator.h"
#include "NNKit/FatJetNN/interface/FatJetNNHelper.h"

using namespace deepntuples;
using namespace std;
using namespace edm;


class MyStuffProducer_94X : public edm::stream::EDProducer<> {
   public:
      explicit MyStuffProducer_94X(const edm::ParameterSet&);
      ~MyStuffProducer_94X();


   private:
      virtual void beginStream(edm::StreamID) override;
      virtual void produce(edm::Event&, const edm::EventSetup&) override;
      virtual void endStream() override;
     
      edm::EDGetTokenT<edm::View<pat::Jet>> jetToken_;
      edm::EDGetTokenT<pat::JetCollection> sdjetToken_;
      double jetR_ = 0.8;
      bool has_puppi_weighted_daughters_;
      //bool useReclusteredJets_ = false;
      std::string datapath_;
      std::string output_;
      int decorrMode = 0; //0: from the jet inputs; 1: from the scores


      std::unique_ptr<FatJetNN> fatjetNN_;
      std::unique_ptr<FatJetNN> decorrNN_;
      std::unique_ptr<FatJetNNDecorrelator> fatjetNNDecorr_;

      std::ofstream fout_;


      //virtual void beginRun(edm::Run const&, edm::EventSetup const&) override;
      //virtual void endRun(edm::Run const&, edm::EventSetup const&) override;
      //virtual void beginLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&) override;
      //virtual void endLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&) override;

      // ----------member data ---------------------------
};

//
// constants, enums and typedefs
//


//
// static data member definitions
//

//
// constructors and destructor
//
MyStuffProducer_94X::MyStuffProducer_94X(const edm::ParameterSet& iConfig):

    jetToken_(consumes<edm::View<pat::Jet> >(iConfig.getUntrackedParameter<edm::InputTag>("jets", edm::InputTag("slimmedJetsAK8")))),
    jetR_(iConfig.getUntrackedParameter<double>("jetR",0.8)), 
    has_puppi_weighted_daughters_(iConfig.getParameter<bool>("hasPuppiWeightedDaughters")),     
    datapath_(iConfig.getUntrackedParameter<std::string>("datapath", "NNKit/data/ak8")),
    output_(iConfig.getUntrackedParameter<std::string>("output", "output.md")),
    decorrMode(iConfig.getUntrackedParameter<int>("decorrMode", 0))

   // output_(iConfig.getUntrackedParameter<std::string>("output", "output.txt"))
{
   // initialize the FatJetNN class in the constructor 
   auto cc = consumesCollector();
   fatjetNN_ = std::make_unique<FatJetNN>(iConfig, cc, jetR_);
   fatjetNN_->load_json(edm::FileInPath(datapath_+"/full/preprocessing.json").fullPath());
   // load DNN model and parameter files
   fatjetNN_->load_model(edm::FileInPath(datapath_+"/full/resnet-symbol.json").fullPath(),
       edm::FileInPath(datapath_+"/full/resnet.params").fullPath());

   std::cout << "====== Decorrelation mode = " << decorrMode << " ======" << std::endl;
   if (decorrMode==0){

       decorrNN_ = std::make_unique<FatJetNN>(iConfig, cc, jetR_);

      //load json for input variable transformation
      decorrNN_->load_json(edm::FileInPath(datapath_+"/decorrelated/preprocessing.json").fullPath());
      //load DNN model and parameter files
      decorrNN_->load_model(edm::FileInPath(datapath_+"/decorrelated/resnet-symbol.json").fullPath(),
        edm::FileInPath(datapath_+"/decorrelated/resnet.params").fullPath());
   }else if (decorrMode==1){
       fatjetNNDecorr_ = std::make_unique<FatJetNNDecorrelator>();

       fatjetNNDecorr_->load_model(edm::FileInPath(datapath_+"/decorrelated/decorr-symbol.json").fullPath(),
          edm::FileInPath(datapath_+"/decorrelated/decorr.params").fullPath());
   }
 

//  if(!output_.empty()){
 //   fout_.open(output_);
 // }
  //produces<vector<pat::Jet>> ();
  produces<vector<pat::Jet>> ();
}

//{
 //  produces<vector<pat::Jet>>();
//}
   //register your products
/* Examples
   produces<ExampleData2>();

   //if do put with a label
   produces<ExampleData2>("label");
 
   //if you want to put into the Run
   produces<ExampleData2,InRun>();
*/
   //now do what ever other initialization is needed
  



MyStuffProducer_94X::~MyStuffProducer_94X()
{ 
//    if(fout_.is_open()){
//    fout_.close();
  }

 
   // do anything here that needs to be done at destruction time
   // (e.g. close files, deallocate resources etc.)

//}


//
// member functions
//

// ------------ method called to produce the data  ------------
void
MyStuffProducer_94X::produce(edm::Event& iEvent, const edm::EventSetup& iSetup)
{
 //  using namespace edm;
/* This is an event example
   //Read 'ExampleData' from the Event
   Handle<ExampleData> pIn;
   iEvent.getByLabel("example",pIn);

   //Use the ExampleData to create an ExampleData2 which 
   // is put into the Event
   std::unique_ptr<ExampleData2> pOut(new ExampleData2(*pIn));
   iEvent.put(std::move(pOut));
*/

/* this is an EventSetup example
   //Read SetupData from the SetupRecord in the EventSetup
   ESHandle<SetupData> pSetup;
   iSetup.get<SetupRecord>().get(pSetup);
*/


  // need to access event info
  fatjetNN_->readEvent(iEvent, iSetup);
  if (decorrMode==0) decorrNN_->readEvent(iEvent, iSetup);

  unique_ptr<vector<pat::Jet>> result (new vector<pat::Jet> () );  

  // loop over the jets
  edm::Handle<edm::View<pat::Jet>> jets;
  iEvent.getByToken(jetToken_, jets);

  //edm::Handle<pat::JetCollection> sdjetsHandle;
  //if (useReclusteredJets_) iEvent.getByToken(sdjetToken_, sdjetsHandle);

  for (unsigned idx=0; idx<jets->size(); ++idx){
    const auto &jet = jets->at(idx);
    JetHelper jet_helper(&jet, has_puppi_weighted_daughters_);
    // reclustered fatjets do not have subjets linked to them, so need to be set manually
   // if (useReclusteredJets_) jet_helper.setSubjets(*sdjetsHandle, jetR_);

    // fout_ << iEvent.id().event() << " "<< idx << " " << jet.pt() << " " << jet.eta() << " " << jet.phi() << " -- ";

    // get the NN predictions
    const auto& nnpreds = fatjetNN_->predict(jet_helper);
    FatJetNNHelper nn(nnpreds);
     nn.get_binarized_score_top();     
//     std::cout<< "binarized_score_top: " << fatjetNN_->get_binarized_score_top()<<std::endl;
     nn.get_binarized_score_w();
     nn.get_binarized_score_z();
     nn.get_binarized_score_zbb();
     nn.get_binarized_score_hbb();
     nn.get_binarized_score_h4q();
     nn.get_raw_score_top();
     nn.get_raw_score_w();
     nn.get_raw_score_z();
     nn.get_raw_score_zbb();
     nn.get_raw_score_hbb();
     nn.get_raw_score_h4q();
     nn.get_raw_score_qcd();


     ////////////////////////////////////
     const pat::Jet & z=jet;
  //   pat::Jet *jet_extended = jet.clone();
     pat::Jet jet_extended(z);
     jet_extended.addUserFloat("binarized_score_top_PUPPI",nn.get_binarized_score_top()); 
     jet_extended.addUserFloat("raw_score_top_PUPPI",nn.get_raw_score_top());
     jet_extended.addUserFloat("binarized_score_w_PUPPI",nn.get_binarized_score_w());
     jet_extended.addUserFloat("raw_score_w_PUPPI",nn.get_raw_score_w());


     

     result->push_back(jet_extended);

     //////////////////////////////////////////////     

 //   for (const auto &v : nnpreds){
 //     fout_ << v << " ";
  //  }
  //  fout_ << std::endl;

  }

  iEvent.put(std::move(result));

 
}

// ------------ method called once each stream before processing any runs, lumis or events  ------------
void
MyStuffProducer_94X::beginStream(edm::StreamID)
{
}

// ------------ method called once each stream after processing all runs, lumis and events  ------------
void
MyStuffProducer_94X::endStream() {
}

// ------------ method called when starting to processes a run  ------------
/*
void
MyStuffProducer::beginRun(edm::Run const&, edm::EventSetup const&)
{
}
*/
 
// ------------ method called when ending the processing of a run  ------------
/*
void
MyStuffProducer::endRun(edm::Run const&, edm::EventSetup const&)
{
}
*/
 
// ------------ method called when starting to processes a luminosity block  ------------
/*
void
MyStuffProducer::beginLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&)
{
}
*/
 
// ------------ method called when ending the processing of a luminosity block  ------------
/*
void
MyStuffProducer::endLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&)
{
}
*/
 
// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------

/*void
MyStuffProducer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  //The following says we do not know what parameters are allowed so do no validation
  // Please change this to state exactly what you do use, even if it is no parameters
  edm::ParameterSetDescription desc;
  desc.setUnknown();
  descriptions.addDefault(desc);
}*/

//define this as a plug-in
DEFINE_FWK_MODULE(MyStuffProducer_94X);
