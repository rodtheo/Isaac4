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
 ** \file Build.hh
 **
 ** Reorders alingments and stores them in bam file.
 ** 
 ** \author Roman Petrovski
 **/

#ifndef iSAAC_BUILD_BUILD_HH
#define iSAAC_BUILD_BUILD_HH

#include <boost/filesystem.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/thread.hpp>

#include "demultiplexing/BarcodePathMap.hh"
#include "alignment/BinMetadata.hh"
#include "alignment/TemplateLengthStatistics.hh"
#include "build/BinSorter.hh"
#include "build/BuildStats.hh"
#include "build/BuildContigMap.hh"
#include "common/Threads.hpp"
#include "flowcell/BarcodeMetadata.hh"
#include "flowcell/Layout.hh"
#include "flowcell/TileMetadata.hh"
#include "io/FileSinkWithMd5.hh"
#include "reference/ReferenceMetadata.hh"
#include "reference/SortedReferenceMetadata.hh"


namespace isaac
{
namespace build
{

class Build
{
    const std::vector<std::string> &argv_;
    const std::string &description_;
    const flowcell::FlowcellLayoutList &flowcellLayoutList_;
    const flowcell::TileMetadataList &tileMetadataList_;
    const flowcell::BarcodeMetadataList &barcodeMetadataList_;
    alignment::BinMetadataList bins_;
    alignment::BinMetadataCRefList binRefs_;
    const reference::SortedReferenceMetadataList &sortedReferenceMetadataList_;
    const BuildContigMap contigMap_;
    const boost::filesystem::path outputDirectory_;
    unsigned maxLoaders_;
    unsigned maxComputers_;
    unsigned allocatedBins_;
    std::vector<unsigned> computeSlotWaitingBins_;
    const unsigned maxSavers_;
    const int bamGzipLevel_;
    const std::string &bamPuFormat_;
    const bool bamProduceMd5_;
    const std::vector<std::string> &bamHeaderTags_;
    // forcedDodgyAlignmentScore_ gets assigned to reads that have their scores at ushort -1
    const unsigned char forcedDodgyAlignmentScore_;
    const bool singleLibrarySamples_;
    const bool keepDuplicates_;
    const bool markDuplicates_;
    const bool anchorMate_;
    const unsigned realignedGapsPerFragment_;
    const alignment::AlignmentCfg &alignmentCfg_;
    const build::GapRealignerMode realignGaps_;
    const unsigned realignMapqMin_;
    const unsigned expectedCoverage_;
    const uint64_t targetBinSize_;
    const double expectedBgzfCompressionRatio_;
    const unsigned maxReadLength_;
    const IncludeTags includeTags_;
    const bool pessimisticMapQ_;

    boost::mutex stateMutex_;
    boost::condition_variable stateChangedCondition_;
    bool forceTermination_;

    common::ThreadVector threads_;

    const reference::ContigLists &contigLists_;

    //pair<[barcode], [output file]>, first maps barcode indexes to unique paths in second
    demultiplexing::BarcodePathMap barcodeBamMapping_;
    //[output file], one stream per bam file path
    boost::ptr_vector<bam::BamIndex> bamIndexes_;
    std::vector<boost::shared_ptr<boost::iostreams::filtering_ostream> > bamFileStreams_;

    BuildStats stats_;

    //[thread][bam file][byte]
    typedef std::vector<bam::BgzfBuffer> BgzfBuffers;
    typedef std::vector<BgzfBuffers> ThreadBgzfBuffers;
    ThreadBgzfBuffers threadBgzfBuffers_;
    // Geometry: [thread][bam file]. Streams for compressing bam data into threadBgzfBuffers_
    boost::ptr_vector<boost::ptr_vector<boost::iostreams::filtering_ostream> > threadBgzfStreams_;
    boost::ptr_vector<boost::ptr_vector<bam::BamIndexPart> > threadBamIndexParts_;

    const build::gapRealigner::Gaps knownIndels_;
    ParallelGapRealigner gapRealigner_;
    BinSorter binSorter_;

    struct Task
    {
        std::size_t maxThreads_;
        /// lower priority values indicate higher priority tasks
        std::size_t priority_;
        bool complete_;
        std::size_t threadsIn_;
        Task(const std::size_t maxThreads, const std::size_t priority):
            maxThreads_(maxThreads), priority_(priority), complete_(false), threadsIn_(0){}
        bool busy() const {return maxThreads_ == threadsIn_;}
        virtual ~Task(){}
        virtual void execute(boost::unique_lock<boost::mutex> &lock, const unsigned threadNumber) = 0;
    };

    typedef std::vector<Task*> Tasks;
    Tasks tasks_;

public:
    Build(const std::vector<std::string> &argv,
          const std::string &description,
          const flowcell::FlowcellLayoutList &flowcellLayoutList,
          const flowcell::TileMetadataList &tileMetadataList,
          const flowcell::BarcodeMetadataList &barcodeMetadataList,
          const alignment::BinMetadataList &bins,
          const reference::ReferenceMetadataList &referenceMetadataList,
          const std::vector<alignment::TemplateLengthStatistics> &barcodeTemplateLengthStatistics,
          const reference::SortedReferenceMetadataList &sortedReferenceMetadataList,
          const reference::ContigLists &contigLists,
          const boost::filesystem::path outputDirectory,
          const unsigned maxLoaders,
          const unsigned maxComputers,
          const unsigned maxSavers,
          const build::GapRealignerMode realignGaps,
          const unsigned realignMapqMin,
          const boost::filesystem::path &knownIndelsPath,
          const int bamGzipLevel,
          const std::string &bamPuFormat,
          const bool bamProduceMd5,
          const std::vector<std::string> &bamHeaderTags,
          const unsigned expectedCoverage,
          const uint64_t targetBinSize,
          const double expectedBgzfCompressionRatio,
          const bool singleLibrarySamples,
          const bool keepDuplicates,
          const bool markDuplicates,
          const bool anchorMate,
          const bool realignGapsVigorously,
          const bool realignDodgyFragments,
          const unsigned realignedGapsPerFragment,
          const bool clipSemialigned,
          const alignment::AlignmentCfg &alignmentCfg,
          const bool loadAllContigs,
          const std::string &binRegexString,
          const unsigned char forcedDodgyAlignmentScore,
          const bool keepUnaligned,
          const bool putUnalignedInTheBack,
          const IncludeTags includeTags,
          const bool pessimisticMapQ);

    void run(common::ScopedMallocBlock &mallocBlock);

    void dumpStats(const boost::filesystem::path &statsXmlPath);

    static uint64_t estimateOptimumFragmentsPerBin(
        const unsigned int estimatedFragmentSize,
        const uint64_t availableMemory,
        const double expectedBgzfCompressionRatio,
        const unsigned computeThreads);

    const demultiplexing::BarcodePathMap &getBarcodeBamMapping() const {return barcodeBamMapping_;}
private:
    std::vector<boost::shared_ptr<boost::iostreams::filtering_ostream> >  createOutputFileStreams(
        const flowcell::TileMetadataList &tileMetadataList,
        const flowcell::BarcodeMetadataList &barcodeMetadataList,
        boost::ptr_vector<bam::BamIndex> &bamIndexes) const;

    void reserveBuffers(
        boost::unique_lock<boost::mutex> &lock,
        const alignment::BinMetadataCRefList::const_iterator thisThreadBinIt,
        const alignment::BinMetadata &bin,
        common::ScopedMallocBlock &mallocBlock,
        const std::size_t threadNumber,
        boost::shared_ptr<BinData> &binDataPtr);

    void reserveBuffers(
        const alignment::BinMetadata &bin,
        const unsigned binStatsIndex,
        const reference::ContigLists &contigLists,
        boost::ptr_vector<boost::iostreams::filtering_ostream> &bgzfStreams,
        boost::ptr_vector<bam::BamIndexPart> &bamIndexParts,
        BgzfBuffers &bgzfBuffers,
        boost::shared_ptr<BinData> &binDataPtr);

    void allocateThreadData(const std::size_t threadNumber);

    template <typename ExceptionType, typename ExceptionDataT>
    bool handleBinAllocationFailure(
        bool warningTraced,
        const alignment::BinMetadata &bin,
        const ExceptionType  &e,
        const ExceptionDataT &errorData);

    boost::shared_ptr<BinData> allocateBin(
        boost::unique_lock<boost::mutex> &lock,
        alignment::BinMetadataCRefList::iterator &thisThreadBinIt,
        alignment::BinMetadataCRefList::iterator &nextUnprocessedBinIt,
        alignment::BinMetadataCRefList::const_iterator &nextUnallocatedBinIt,
        const alignment::BinMetadataCRefList::const_iterator binsEnd,
        common::ScopedMallocBlock &mallocBlock,
        const std::size_t threadNumber);

    void waitForLoadSlot(
        boost::unique_lock<boost::mutex> &lock,
        const alignment::BinMetadataCRefList::const_iterator thisThreadBinIt,
        const alignment::BinMetadataCRefList::const_iterator binsEnd,
        alignment::BinMetadataCRefList::const_iterator &nextUnloadedBinIt);

    void returnLoadSlot(const bool exceptionUnwinding);

    bool yieldIfPossible(
        boost::unique_lock<boost::mutex>& lock,
        const std::size_t threadNumber,
        Task *task);

    bool processMostUrgent(boost::unique_lock<boost::mutex> &lock, const unsigned threadNumber, Task *task);

    template <typename OperationT>
    void preemptComputeSlot(
        boost::unique_lock<boost::mutex> &lock,
        const std::size_t maxThreads,
        const std::size_t priority,
        OperationT operation,
        const unsigned threadNumber);

    void returnComputeSlot(const bool exceptionUnwinding);

    void waitForSaveSlot(
        boost::unique_lock<boost::mutex> &lock,
        const alignment::BinMetadataCRefList::const_iterator thisThreadBinIt,
        alignment::BinMetadataCRefList::const_iterator &nextUnserializedBinIt);

    void returnSaveSlot(
        alignment::BinMetadataCRefList::const_iterator &nextUnserializedBinIt,
        const alignment::BinMetadataCRefList::const_iterator thisThreadBinEndIt,
        const bool exceptionUnwinding);

    void sortBinParallel(alignment::BinMetadataCRefList::iterator &nextUnprocessedBinIt,
                         alignment::BinMetadataCRefList::const_iterator &nextUnallocatedBinIt,
                         alignment::BinMetadataCRefList::const_iterator &nextUnloadedBinIt,
                         alignment::BinMetadataCRefList::const_iterator &nextUnserializedBinIt,
                         common::ScopedMallocBlock &mallocBlock,
                         const std::size_t threadNumber);

    void saveAndReleaseBuffers(
        boost::unique_lock<boost::mutex> &lock,
        const boost::filesystem::path &filePath,
        const std::size_t threadNumber);

    void saveBuffer(
        const bam::BgzfBuffer &bgzfBuffer,
        std::ostream &bamStream,
        const bam::BamIndexPart &bamIndexPart,
        bam::BamIndex &bamIndex,
        const boost::filesystem::path &filePath);

    uint64_t estimateBinCompressedDataRequirements(
        const alignment::BinMetadata & binMetadata,
        const unsigned outputFileIndex) const;

    bool executePreemptTask(
        boost::unique_lock<boost::mutex>& lock,
        Task& task,
        const unsigned threadNumber) const;

    void cleanupBinAllocationFailure(
        const alignment::BinMetadata& bin,
        boost::ptr_vector<boost::iostreams::filtering_ostream>& bgzfStreams,
        boost::ptr_vector<bam::BamIndexPart>& bamIndexParts,
        boost::shared_ptr<BinData>& binDataPtr, BgzfBuffers& bgzfBuffers);
};

} // namespace build
} // namespace isaac

#endif // #ifndef iSAAC_BUILD_BUILD_HH
