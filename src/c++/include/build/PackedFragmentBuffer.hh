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
 ** \file PackedFragmentBuffer.hh
 **
 ** Helper.
 ** 
 ** \author Roman Petrovski
 **/

#ifndef iSAAC_BUILD_PACKED_FRAGMENT_BUFFER_HH
#define iSAAC_BUILD_PACKED_FRAGMENT_BUFFER_HH

#include "alignment/BinMetadata.hh"
#include "build/FragmentIndex.hh"

namespace isaac
{
namespace build
{

/**
 * \brief Helper to access fragments stored in a contiguous byte vector
 */
class PackedFragmentBuffer : std::vector<char, common::NumaAllocator<char, common::numa::defaultNodeLocal> >
{
    typedef std::vector<char, common::NumaAllocator<char, common::numa::defaultNodeLocal> > BaseT;
public:
    struct Index
    {
        Index(
            const isaac::reference::ReferencePosition pos,
            uint64_t dataOffset,
            uint64_t mateDataOffset,
            const unsigned *cigarBegin,
            const unsigned *cigarEnd,
            const bool reverse):
                pos_(pos), dataOffset_(dataOffset), mateDataOffset_(mateDataOffset),
                cigarBegin_(cigarBegin), cigarEnd_(cigarEnd), reverse_(reverse),
                splitInfoOffset_(0), splitInfoCount_(0)
        {}

        Index(const FStrandFragmentIndex &idx, const io::FragmentAccessor &fragment) :
            Index(idx.fStrandPos_, idx.dataOffset_, idx.mateDataOffset_,
                  fragment.cigarBegin(), fragment.cigarEnd(), fragment.isReverse()){}
        Index(const RStrandOrShadowFragmentIndex &idx, const io::FragmentAccessor &fragment) :
            Index(idx.fStrandPos_, idx.dataOffset_, idx.mateDataOffset_,
                  fragment.cigarBegin(), fragment.cigarEnd(), fragment.isReverse()){}
        Index(const SeFragmentIndex &idx, const io::FragmentAccessor &fragment) :
            Index(idx.fStrandPos_, idx.dataOffset_, idx.dataOffset_,
                  fragment.cigarBegin(), fragment.cigarEnd(), fragment.isReverse()){}

        bool hasMate() const
        {
            return mateDataOffset_ != dataOffset_;
        }

        unsigned getBeginClippedLength() const
        {
            ISAAC_ASSERT_MSG(cigarBegin_ != cigarEnd_, "Unexpected empty CIGAR");
            std::pair<unsigned, alignment::Cigar::OpCode> operation = alignment::Cigar::decode(*cigarBegin_);
            if (alignment::Cigar::SOFT_CLIP == operation.second)
            {
                return operation.first;
            }
            return 0;
        }

        /**
         * \brief Returns unadjusted position if it is adjusted due to a soft clipping
         */
        isaac::reference::ReferencePosition getUnclippedPosition() const
        {
            return pos_ - getBeginClippedLength();
        }

        // temporary storage for fragment.fStrandPosition_. Not guaranteed to be up to date.
        // currently gap realigner updates shadow fragment.fStrandPosition_ without updating its index.
        // Ensure it is synchronized with fragment.fStrandPosition_ before using it
        isaac::reference::ReferencePosition pos_;
        std::size_t dataOffset_;
        // same as dataOffset_ for single-ended
        std::size_t mateDataOffset_;

        typedef const uint32_t * CigarIterator;
        CigarIterator cigarBegin_;
        CigarIterator cigarEnd_;
        bool reverse_;
        // offset into list of SplitInfo structures
        unsigned splitInfoOffset_;
        unsigned splitInfoCount_;


        friend std::ostream & operator <<(std::ostream &os, const PackedFragmentBuffer::Index &index)
        {
            os << "PackedFragmentBuffer::Index(" <<
                index.pos_ << "," << index.dataOffset_ << "do " << index.mateDataOffset_ << "mdo, ";
            return alignment::Cigar::toStream(index.cigarBegin_, index.cigarEnd_, os) << ")";
        }

        bool operator != (const Index &that)
        {
            ISAAC_ASSERT_MSG(that.dataOffset_ == dataOffset_, "Invalid comparison of index of different fragments " << *this << " vs " << that);
            ISAAC_ASSERT_MSG(that.mateDataOffset_ == mateDataOffset_, "Invalid comparison of index of different fragments " << *this << " vs " << that);
            ISAAC_ASSERT_MSG(that.reverse_ == reverse_, "Invalid comparison of index of different fragments " << *this << " vs " << that);

            return that.pos_ != pos_ || std::distance(that.cigarBegin_, that.cigarEnd_) != std::distance(cigarBegin_, cigarEnd_) ||
                cigarEnd_ != std::mismatch(cigarBegin_, cigarEnd_, that.cigarBegin_).first;
        }

    };


    using BaseT::capacity;
    using BaseT::front;
    using BaseT::size;
    using BaseT::begin;
    using BaseT::end;
    using BaseT::const_iterator;
    using BaseT::iterator;
    using BaseT::reserve;

    void resize(const alignment::BinMetadata& bin)
    {
        BaseT::resize(bin.getDataSize());
    }

    void resize(const std::size_t size)
    {
        BaseT::resize(size);
    }

    void unreserve()
    {
        BaseT().swap(*this);
    }

    io::FragmentAccessor &getFragment(const Index& fragmentIndex)
        {return getFragment(fragmentIndex.dataOffset_);}

    io::FragmentAccessor &getMate(const Index& fragmentIndex)
        {return getFragment(fragmentIndex.mateDataOffset_);}

    const io::FragmentAccessor &getFragment(const Index& fragmentIndex) const
        {return getFragment(fragmentIndex.dataOffset_);}

    io::FragmentAccessor &getFragment(const FragmentIndex &fragmentIndex)
        {return getFragment(fragmentIndex.dataOffset_);}

    const io::FragmentAccessor &getFragment(const FragmentIndex &fragmentIndex) const
        {return getFragment(fragmentIndex.dataOffset_);}

    const io::FragmentAccessor &getMate(const FragmentIndex& fragmentIndex) const
        {return getFragment(fragmentIndex.mateDataOffset_);}

    io::FragmentAccessor &getFragment(
        const uint64_t offset)
    {
        io::FragmentAccessor &fragment =
            *reinterpret_cast<io::FragmentAccessor*>(&at(offset));
        return fragment;
    }

    const io::FragmentAccessor &getFragment(
        const uint64_t offset) const
    {
        const io::FragmentAccessor &fragment =
            *reinterpret_cast<const io::FragmentAccessor*>(&at(offset));
        return fragment;
    }

    static uint64_t getMemoryRequirements(const alignment::BinMetadata& bin)
    {
        return bin.getDataSize();
    }

    bool orderForBam(const Index &left, const Index &right) const
    {
        if (left.pos_ < right.pos_)
        {
            return true;
        }
        else if (left.pos_ == right.pos_)
        {
            const io::FragmentAccessor &leftFragment = getFragment(left);
            const io::FragmentAccessor &rightFragment = getFragment(right);
            const uint64_t leftGlobalClusterId = leftFragment.tile_ * INSANELY_HIGH_NUMBER_OF_CLUSTERS_PER_TILE + leftFragment.clusterId_;
            const uint64_t rightGlobalClusterId = rightFragment.tile_ * INSANELY_HIGH_NUMBER_OF_CLUSTERS_PER_TILE + rightFragment.clusterId_;

            return
                leftGlobalClusterId < rightGlobalClusterId ||
                (leftGlobalClusterId == rightGlobalClusterId &&
                    // ensure singleton and shadow stay together and shadow follows the singleton
                    (leftFragment.flags_.unmapped_ < rightFragment.flags_.unmapped_ ||
                        (leftFragment.flags_.unmapped_ == rightFragment.flags_.unmapped_ &&
                            // SAAC-378 ensure second read comes after first to generate consistent bam files between different runs
                            leftFragment.flags_.secondRead_ < rightFragment.flags_.secondRead_
                        )
                    )
                );
        }

        return false;
    }
};


} // namespace build
} // namespace isaac

#endif // #ifndef iSAAC_BUILD_PACKED_FRAGMENT_BUFFER_HH
