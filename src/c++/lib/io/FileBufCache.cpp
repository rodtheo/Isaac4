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
 ** \file FileBufCache.hh
 **
 ** Vector of file buffers which are kept open to reduce the cost of closing/opening files.
 **
 ** \author Roman Petrovski
 **/

#include "io/FileBufCache.hh"

namespace isaac
{
namespace io
{

//const boost::filesystem::path FileBufWithReopenHolderBase::emptyPath_;

} // namespace io
} // namespace isaac
