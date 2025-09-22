### the directory name
set(directory source/APPLICATIONS/TOPP)

### list all filenames of the directory here
set(TOPP_executables
AccurateMassSearch
AssayGeneratorMetabo
AssayGeneratorMetaboSirius
BaselineFilter
ClusterMassTraces
ClusterMassTracesByPrecursor
CometAdapter
ConsensusID
ConsensusMapNormalizer
CVInspector
DatabaseFilter
DatabaseSuitability
DecoyDatabase
Decharger
DeMeanderize
Digestor
DigestorMotif
DTAExtractor
EICExtractor
Epifany
ExternalCalibration
FalseDiscoveryRate
FeatureFinderCentroided
FeatureFinderIdentification
FeatureFinderMetabo
FeatureFinderMetaboIdent
FeatureFinderMultiplex
FeatureLinkerLabeled
FeatureLinkerUnlabeled
FeatureLinkerUnlabeledKD
FeatureLinkerUnlabeledQT
FileConverter
FileFilter
FileInfo
FileMerger
FLASHDeconv
FuzzyDiff
GenericWrapper
GNPSExport
HighResPrecursorMassCorrector
IDConflictResolver
IDDecoyProbability
IDExtractor
IDFileConverter
IDFilter
IDMapper
IDMerger
IDPosteriorErrorProbability
IDRipper
IDRTCalibration
IDScoreSwitcher
IDSplitter
InternalCalibration
IonMobilityBinning
IsobaricAnalyzer
JSONExporter
LuciphorAdapter
MapAlignerIdentification
MapAlignerPoseClustering
MapAlignerTreeGuided
MapNormalizer
MapRTTransformer
MapStatistics
MaRaClusterAdapter
MassCalculator
MassTraceExtractor
MetaProSIP
MetaboliteAdductDecharger
MetaboliteSpectralMatcher
MRMMapper
MRMPairFinder
MSGFPlusAdapter
MSFraggerAdapter
MSstatsConverter
MultiplexResolver
MzMLSplitter
MzTabExporter
NucleicAcidSearchEngine
NoiseFilterGaussian
NoiseFilterSGolay
NovorAdapter
OpenMSDatabasesInfo
OpenMSInfo
OpenNuXL
OpenPepXL
OpenPepXLLF
OpenSwathAnalyzer
OpenSwathAssayGenerator
OpenSwathChromatogramExtractor
OpenSwathConfidenceScoring
OpenSwathDecoyGenerator
OpenSwathFeatureXMLToTSV
OpenSwathRTNormalizer
PeakPickerHiRes
PeakPickerIterative
PeptideIndexer
PeptideDataBaseSearchFI
PercolatorAdapter
PhosphoScoring
ProteinInference
ProteinQuantifier
ProteomicsLFQ
PSMFeatureExtractor
QCCalculator
QCEmbedder
QCExporter
QCExtractor
QCImporter
QCMerger
QCShrinker
QualityControl
RNADigestor
RNAMassCalculator
RNPxlXICFilter
SageAdapter
SeedListGenerator
SemanticValidator
SequenceCoverageCalculator
SimpleSearchEngine
SiriusExport
SpectraFilterNLargest
SpectraFilterNormalizer
SpectraFilterThresholdMower
SpectraFilterWindowMower
SpectraSTSearchAdapter
SpectraMerger
StaticModification
TextExporter
TICCalculator
TriqlerConverter
XFDR
XMLValidator
)

if(NOT DISABLE_OPENSWATH)
  set(TOPP_executables
    ${TOPP_executables}
    TargetedFileConverter
    OpenSwathDIAPreScoring
    OpenSwathMzMLFileCacher
    OpenSwathWorkflow
    OpenSwathFileSplitter
    OpenSwathRewriteToFeatureXML
    MRMTransitionGroupPicker
  )
endif(NOT DISABLE_OPENSWATH)

if(WITH_PARQUET)
  set(TOPP_executables
    ${TOPP_executables}
    QuantmsIOConverter
  )
endif(WITH_PARQUET)

## all targets requiring OpenMS_GUI
set(TOPP_executables_with_GUIlib
ExecutePipeline
Resampler
# util category
ImageCreator
INIUpdater
)

# Following commit "remove from most TOPP tools", most TOPP tools no longer require Qt GUI.
# Keep GUI linkage only for true GUI tools (ExecutePipeline, Resampler, ImageCreator, INIUpdater)
# via TOPP_executables_with_GUIlib defined above. No additional gating needed here.
### add filenames to Visual Studio solution tree
set(sources_VS)
foreach(i ${TOPP_executables} ${TOPP_executables_with_GUIlib})
	list(APPEND sources_VS "${i}.cpp")
endforeach(i)

source_group("" FILES ${sources_VS})
