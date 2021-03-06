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
 ** \file SemialignedEndsClipper.hh
 **
 ** \brief Utility classes for detecting and removing fragment parts that contain
 **        sequences of the adapters
 ** 
 ** \author Roman Petrovski
 **/

#ifndef iSAAC_ALIGNMENT_SEMIALIGNED_ENDS_CLIPPER_HH
#define iSAAC_ALIGNMENT_SEMIALIGNED_ENDS_CLIPPER_HH

#include "alignment/BamTemplate.hh"
#include "build/PackedFragmentBuffer.hh"
#include "io/Fragment.hh"
#include "reference/Contig.hh"

namespace isaac
{
namespace build
{

class SemialignedEndsClipper : boost::noncopyable
{
    static const unsigned CONSECUTIVE_MATCHES_MIN = 5;
public:
    SemialignedEndsClipper(alignment::Cigar &cigarBuffer) : cigarBuffer_(cigarBuffer)
    {
    }

    void clip(
        const reference::ContigList &contigs,
        const reference::ReferencePosition binEndPos,
        const io::FragmentAccessor &fragment,
        PackedFragmentBuffer::Index &index,
        reference::ReferencePosition &newRStrandPosition,
        unsigned short &newEditDistance);

private:
    alignment::Cigar &cigarBuffer_;

    bool clipLeftSide(
        const reference::ContigList &contigList,
        const reference::ReferencePosition binEndPos,
        const unsigned char *sequenceBegin,
        PackedFragmentBuffer::Index &index,
        unsigned short &newEditDistance);

    bool clipRightSide(
        const reference::ContigList &contigList,
        const unsigned char *sequenceEnd,
        PackedFragmentBuffer::Index &index,
        reference::ReferencePosition &newRStrandPosition,
        unsigned short &newEditDistance);

};

} // namespace build
} // namespace isaac

#endif // #ifndef iSAAC_ALIGNMENT_SEMIALIGNED_ENDS_CLIPPER_HH
