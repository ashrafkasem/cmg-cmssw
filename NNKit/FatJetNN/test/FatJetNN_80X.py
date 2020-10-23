########### author : Pantelis Kontaxakis ##########
########### institute : National and Kapodistrian University of Athens #######################
########### Email : pantelis.kontaxakis@cern.ch #########
########### Date : November 2018 #######################

import FWCore.ParameterSet.Config as cms

# ---------------------------------------------------------
from FWCore.ParameterSet.VarParsing import VarParsing
options = VarParsing('analysis')
options.inputFiles = '/store/mc/RunIISummer16MiniAODv2/ZprimeToTT_M-3000_W-30_TuneCUETP8M1_13TeV-madgraphMLM-pythia8/MINIAODSIM/PUMoriond17_80X_mcRun2_asymptotic_2016_TrancheIV_v6-v1/80000/D6D620EF-73BE-E611-8BFB-B499BAA67780.root'
options.maxEvents = 10
#options.parseArguments()

# ---------------------------------------------------------
process = cms.Process("FatJetNN")

process.load('FWCore.MessageService.MessageLogger_cfi')
process.MessageLogger.cerr.FwkReport.reportEvery = 100

process.options = cms.untracked.PSet(
   allowUnscheduled = cms.untracked.bool(True),  
   wantSummary=cms.untracked.bool(False)
)

process.maxEvents = cms.untracked.PSet(input=cms.untracked.int32(options.maxEvents))

process.source = cms.Source('PoolSource',
    fileNames=cms.untracked.vstring(options.inputFiles)
)

# ---------------------------------------------------------
process.load("FWCore.MessageService.MessageLogger_cfi")
process.load("Configuration.EventContent.EventContent_cff")
process.load('Configuration.StandardSequences.Services_cff')
process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
process.load('Configuration.StandardSequences.MagneticField_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, '80X_mcRun2_asymptotic_2016_TrancheIV_v8', '')
print 'Using global tag', process.GlobalTag.globaltag
# ---------------------------------------------------------
# set up TransientTrackBuilder
process.TransientTrackBuilderESProducer = cms.ESProducer("TransientTrackBuilderESProducer",
    ComponentName=cms.string('TransientTrackBuilder')
)
# ---------------------------------------------------------
# recluster Puppi jets
bTagDiscriminators = [
    'pfCombinedInclusiveSecondaryVertexV2BJetTags',
    'pfBoostedDoubleSecondaryVertexAK8BJetTags'
]
JETCorrLevels = ['L2Relative', 'L3Absolute']

from JMEAnalysis.JetToolbox.jetToolbox_cff import jetToolbox
jetToolbox(process, 'ak8', 'jetSequence', 'out', PUMethod='Puppi', JETCorrPayload='AK8PFPuppi', JETCorrLevels=JETCorrLevels, miniAOD=True, runOnMC=True,
           Cut='pt > 170.0 && abs(rapidity()) < 2.4', addNsub=True, maxTau=3,
           addSoftDrop=True, addSoftDropSubjets=True, subJETCorrPayload='AK4PFPuppi', subJETCorrLevels=JETCorrLevels,
           bTagDiscriminators=bTagDiscriminators)
srcJets = cms.untracked.InputTag('packedPatJetsAK8PFPuppiSoftDrop')
# ---------------------------------------------------------
process.deepntuplizer = cms.EDProducer('MyStuffProducer_94X',
                                jets=srcJets,
                                hasPuppiWeightedDaughters=cms.bool(True),
                                jetR=cms.untracked.double(0.8),
                                datapath=cms.untracked.string('NNKit/data/ak8'),
                                #output=cms.untracked.string('output_80X.md'),
                                )
process.p = cms.Path(process.deepntuplizer)


process.MINIAODSIMoutput = cms.OutputModule("PoolOutputModule",
    compressionAlgorithm = cms.untracked.string('LZMA'),
    compressionLevel = cms.untracked.int32(4),
    dataset = cms.untracked.PSet(
        dataTier = cms.untracked.string(''),
        filterName = cms.untracked.string('')
    ),
    dropMetaData = cms.untracked.string('ALL'),
    eventAutoFlushCompressedSize = cms.untracked.int32(15728640),
    fastCloning = cms.untracked.bool(False),
    fileName = cms.untracked.string('DeepAK8Producer.root'),

    outputCommands = cms.untracked.vstring(#'keep *',
                                           #'drop *_deepNNTagInfos*_*_*',
                                           'drop *',
                                           'keep *_deepntuplizer_*_*',
                                           #'keep *_selectedUpdatedPatJets*_*_*',         
                                           ),

 #   overrideInputFileSplitLevels = cms.untracked.bool(True)
)

process.endpath = cms.EndPath(process.MINIAODSIMoutput)


