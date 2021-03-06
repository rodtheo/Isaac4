/**
 ** Isaac Genome Alignment Software
 ** Copyright (c) 2010-2017 Illumina, Inc.
 ** All rights reserved.
 **
 ** This software is provided under the terms and conditions of the
 ** GNU GENERAL PUBLIC LICENSE Version 3
 **
 ** You should have received a copy of the GNU GENERAL PUBLIC LICENSE Version 3
 ** along with this program. If not, see
 ** <https://github.com/illumina/licenses/>.
 **
 ** \file AlignOptions.hh
 **
 ** Command line options for 'isaac-align'
 **
 ** \author Come Raczy
 **/

#ifndef iSAAC_OPTIONS_ALIGN_OPTIONS_HH
#define iSAAC_OPTIONS_ALIGN_OPTIONS_HH

#include <string>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

#include "build/GapRealigner.hh"
#include "common/Program.hh"
#include "flowcell/BarcodeMetadata.hh"
#include "flowcell/Layout.hh"
#include "flowcell/ReadMetadata.hh"
#include "alignment/TemplateLengthStatistics.hh"
#include "workflow/AlignWorkflow.hh"

namespace isaac
{
namespace options
{

class AlignOptions : public common::Options
{
public:
    AlignOptions();

    common::Options::Action parse(int argc, char *argv[]);

private:
    std::string usagePrefix() const {return "isaac-align -r <reference> -b <base calls> -m <memory limit> [optional arguments]";}
    void postProcess(boost::program_options::variables_map &vm);
    void verifyMandatoryPaths(boost::program_options::variables_map &vm);
    void parseParallelization();
    build::GapRealignerMode parseGapRealignment();
    void parseExecutionTargets();
    void parseMemoryControl();
    void parseGapScoring();
    void parseSmithWatermanOptions();
    workflow::AlignWorkflow::OptionalFeatures parseBamExcludeTags(std::string strBamExcludeTags);
    void parseDodgyAlignmentScore();
    void parseTemplateLength();
    void parseReferenceGenomes();
    std::vector<boost::filesystem::path> parseSampleSheetPaths() const;
    std::vector<std::pair<flowcell::Layout::Format, bool> > parseBaseCallsFormats();
    void parseStatsImageFormat();
    void parseQScoreBinValues();
    void parseBamExcludeTags();
    void processLegacyOptions(boost::program_options::variables_map &vm);
    void parseHashTableBuckets();


public:
    std::vector<std::string> argv;
    std::string description;
    // have to use string list because boost fails to populate path list with paths containing spaces
    std::vector<std::string> baseCallsStringList;
    std::vector<boost::filesystem::path> baseCallsDirectoryList;
    std::vector<std::string> baseCallsFormatStringList;
    std::vector<std::string> sampleSheetStringList;
    std::vector<std::string> barcodeMismatchesStringList;
    std::vector<std::string> tilesFilterList;
    std::vector<std::string> useBasesMaskList;
    std::size_t hashTableBucketCount;
    std::vector<flowcell::Layout> flowcellLayoutList;
    flowcell::BarcodeMetadataList barcodeMetadataList;
    // another workaround for boost and spaces in paths
    std::string sortedReferenceXmlString;
    boost::filesystem::path sortedReferenceXml;
    std::string referenceName;
    reference::ReferenceMetadataList referenceMetadataList;
    std::string tempDirectoryString;
    boost::filesystem::path tempDirectory;
    std::string outputDirectoryString;
    boost::filesystem::path outputDirectory;
    unsigned seedLength;
    bool allowVariableFastqReadLength;
    bool allowVariableReadLength;
    char fastqQ0;
    unsigned laneNumberMax;
    unsigned readNameLength;
    bool cleanupIntermediary;
    unsigned bclTilesPerChunk;
    bool ignoreMissingBcls;
    bool ignoreMissingFilters;
    // number of seeds to use on the first pass
    unsigned expectedCoverage;
    uint64_t targetBinSizeMB;
    // the list of seed metadata
    unsigned jobs;
    bool enableNuma;
    std::size_t candidateMatchesMax;
    unsigned matchFinderTooManyRepeats;
    unsigned matchFinderWayTooManyRepeats;
    unsigned matchFinderShadowSplitRepeats;
    unsigned seedBaseQualityMin;
    unsigned repeatThreshold;
    int mateDriftRange;
    unsigned neighborhoodSizeThreshold;
    std::string startFromString;
    workflow::AlignWorkflow::State startFrom;
    std::string stopAtString;
    workflow::AlignWorkflow::State stopAt;
    unsigned int verbosity;
    unsigned clustersAtATimeMax;
    bool ignoreNeighbors;
    bool ignoreRepeats;
    int mapqThreshold;
    bool perTileTls;
    bool pfOnly;
    bool allowEmptyFlowcells_;
    unsigned baseQualityCutoff;
    std::string keepUnalignedString;
    bool keepUnaligned;
    bool preSortBins;
    bool preAllocateBins;
    bool putUnalignedInTheBack;
    bool realignGapsVigorously;
    bool realignDodgyFragments;
    unsigned realignedGapsPerFragment;
    bool clipSemialigned;
    bool clipOverlapping;
    bool scatterRepeats;
    bool rescueShadows;
    bool trimPEAdapters;
    unsigned gappedMismatchesMax;
    unsigned smitWatermanGapsMax;
    std::string useSmithWaterman;
    bool smartSmithWaterman;
    unsigned smithWatermanGapSizeMax;
    bool splitAlignments;
    std::string gapScoringString;
    int gapMatchScore;
    int gapMismatchScore;
    int gapOpenScore;
    int gapExtendScore;
    int minGapExtendScore;
    unsigned splitGapLength;
    std::string dodgyAlignmentScoreString;
    alignment::TemplateBuilder::DodgyAlignmentScore dodgyAlignmentScore;
    unsigned anomalousPairHandicap;
    std::string memoryControlString;
    common::ScopedMallocBlock::Mode memoryControl;
    uint64_t memoryLimit;
    static const uint64_t memoryLimitUnlimited = 0;
    unsigned inputLoadersMax;
    unsigned tempSaversMax;
    unsigned tempLoadersMax;
    unsigned outputSaversMax;
    std::string realignGapsString;
    build::GapRealignerMode realignGaps;
    unsigned realignMapqMin;
    std::string knownIndelsPathString;
    boost::filesystem::path knownIndelsPath;
    int bamGzipLevel;
    std::vector<std::string> bamHeaderTags;
    std::string bamPuFormat;
    bool bamProduceMd5;
    double expectedBgzfCompressionRatio;
    bool singleLibrarySamples;
    bool keepDuplicates;
    bool markDuplicates;
    bool anchorMate;
    std::string binRegexString;
    std::string decoyRegexString;
    std::vector<std::size_t> clusterIdList;
    alignment::TemplateLengthStatistics userTemplateLengthStatistics;
    std::string tlsString;
    std::vector<std::string> defaultAdapters;
    std::string statsImageFormatString;
    reports::AlignmentReportGenerator::ImageFileFormat statsImageFormat;
    bool qScoreBin;
    std::string qScoreBinValueString;
    boost::array<char, 256> fullBclQScoreTable;
    std::string bamExcludeTags;
    workflow::AlignWorkflow::OptionalFeatures optionalFeatures;
    bool pessimisticMapQ;
    unsigned detectTemplateBlockSize;
    bool disableResume;
};

} // namespace options
} // namespace isaac

#endif // #ifndef iSAAC_OPTIONS_ALIGN_OPTIONS_HH
