########### author : Pantelis Kontaxakis ##########
########### institute : National and Kapodistrian University of Athens #######################
########### Email : pantelis.kontaxakis@cern.ch #########
########### Date : November 2018 #######################

import FWCore.ParameterSet.Config as cms

# ---------------------------------------------------------
from FWCore.ParameterSet.VarParsing import VarParsing
options = VarParsing('analysis')
options.inputFiles = '/store/mc/RunIIFall17MiniAOD/ZprimeToWWToWlepWhad_narrow_M-3000_TuneCP5_13TeV-madgraph/MINIAODSIM/94X_mc2017_realistic_v10-v1/20000/3E25D208-8205-E811-8858-3417EBE64426.root'
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
process.GlobalTag = GlobalTag(process.GlobalTag, '94X_mc2017_realistic_v13', '')
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

# ---------------------------------------------------------
process.deepntuplizer = cms.EDProducer('MyStuffProducer_94X',
                                jets=cms.untracked.InputTag('slimmedJetsAK8'),
                                hasPuppiWeightedDaughters=cms.bool(False),
                                jetR=cms.untracked.double(0.8),
                                datapath=cms.untracked.string('NNKit/data/ak8'),
                                #output=cms.untracked.string('output_94X.md'),
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

