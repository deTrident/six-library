/* =========================================================================
 * This file is part of six-c++ 
 * =========================================================================
 * 
 * (C) Copyright 2004 - 2009, General Dynamics - Advanced Information Systems
 *
 * six-c++ is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program; If not, 
 * see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef __SIX_NITF_IMAGE_INFO_H__
#define __SIX_NITF_IMAGE_INFO_H__

#include "six/Data.h"
#include "six/Utilities.h"
#include "six/NITFSegmentInfo.h"

namespace six
{

/*!
 *  \class NITFImageInfo
 *  \brief Internal book-keeping for NITF imagery
 *
 *  This class has the logic that calculates the dimensions of
 *  each image segment.  It also can tell you practical information
 *  about the image itself.
 *
 *  This class is used for both complex and derived products
 *
 *
 */
class NITFImageInfo
{
public:

    NITFImageInfo(Data* d, unsigned long maxRows = Constants::ILOC_MAX,
            sys::Uint64_T maxSize = Constants::IS_SIZE_MAX,
            bool computeSegments = false) :
        data(d), startIndex(0), numRowsLimit(maxRows), maxProductSize(maxSize)
    {
        productSize = (sys::Uint64_T) data->getNumBytesPerPixel()
                * (sys::Uint64_T) data->getNumRows()
                * (sys::Uint64_T) data->getNumCols();
        if (computeSegments)
            compute();
    }

    unsigned int getNumBitsPerPixel() const
    {
        return data->getNumBytesPerPixel() / data->getNumChannels() * 8;
    }

    std::string getPixelValueType() const
    {
        std::string type;
        switch (data->getPixelType())
        {
        case RE32F_IM32F:
            type = "R";
            break;

        default:
            type = "INT";

        }
        return type;
    }

    std::string getRepresentation() const
    {

        std::string irep;
        switch (data->getPixelType())
        {
        case MONO8LU:
        case MONO8I:
        case MONO16I:
            irep = "MONO";
            break;
        case RGB8LU:
            irep = "RGB/LUT";
            break;
        case RGB24I:
            irep = "RGB";
            break;

        default:
            irep = "NODISPLY";
        }
        return irep;

    }

    std::string getMode() const
    {
        std::string imode;
        switch (data->getPixelType())
        {
        case RGB8LU:
        case MONO8LU:
        case MONO8I:
        case MONO16I:
            imode = "B";
            break;
        default:
            imode = "P";
        }
        return imode;
    }

    Data* getData() const
    {
        return data;
    }

    std::vector<NITFSegmentInfo> getImageSegments() const
    {
        return imageSegments;
    }

    void addSegment(NITFSegmentInfo info)
    {
        imageSegments.push_back(info);
    }

    unsigned long getStartIndex() const
    {
        return startIndex;
    }
    void setStartIndex(unsigned long index)
    {
        startIndex = index;
    }

    //! Number of bytes in the product
    sys::Uint64_T getProductSize() const
    {
        return productSize;
    }

    //! This is the total number of rows we can have in a NITF segment
    unsigned long getNumRowsLimit() const
    {
        return numRowsLimit;
    }

    //! This is the total size that each product seg can be
    sys::Uint64_T getMaxProductSize() const
    {
        return maxProductSize;
    }

    // Currently punts on LU
    std::vector<nitf::BandInfo> getBandInfo();

    //! TODO dont forget me!!
    static PixelType getPixelTypeFromNITF(nitf::ImageSubheader& subheader);

protected:
    Data* data;

    unsigned long startIndex;

    //! Number of bytes in the product
    sys::Uint64_T productSize;

    //! This is the total number of rows we can have in a NITF segment
    unsigned long numRowsLimit;

    //! This is the total size that each product seg can be
    sys::Uint64_T maxProductSize;

    /*!
     *  This is a vector of segment information that is used to get
     *  the per-segment info for populating/reading from a NITF
     *
     *  Note that the number of segments has a hard limit of 999
     */
    std::vector<NITFSegmentInfo> imageSegments;

    void computeImageInfo();

    /*!
     *  This function figures out the parameters for each segment
     *  The algorithm follows the document.  The document does not
     *  state that they mean us to use WGS84 for LLA, but we assume
     *  that that is the intention
     *
     */
    void computeSegmentInfo();

    /*!
     *  This function transforms the LLA corners
     *  for the first row first column,
     *  and the first row last column, and turns those
     *  into ECEF, uses linear interpolation
     *
     */
    void computeSegmentCorners();

    /*!
     *  This function is called by the container to determine
     *  what the properties of the image segments will be.
     *
     */
    void compute();

    /*     NITFImageInfo() : data(NULL), startIndex(0), */
    /*         numRowsLimit(Constants::ILOC_MAX), */
    /*             maxProductSize(Constants::IS_SIZE_MAX)  {} */

    /*!
     *  By default, we use the IS_SIZE_MAX to determine the max product
     *  size for an image segment, and if we have to segment, we
     *  use the ILOC_MAX to determine the segment size (if that is
     *  smaller than the product size).  These calls give the 
     *  user access to these limits and allows them to be overridden.
     *
     *  This method would typically only be used during product
     *  prototyping and testing.  It should not be used to artificially
     *  constrain actual products.
     *
     *  Do not attempt to use this method unless you understand the
     *  segmentation rules.
     *
     */
    /*      void setLimits(sys::Uint64_T maxSize, */
    /*                     unsigned long maxRows) */
    /*      { */

    /*          if (maxSize > Constants::IS_SIZE_MAX) */
    /*              throw except::Exception(Ctxt("You cannot exceed the IS_SIZE_MAX")); */

    /*          if (maxRows > Constants::ILOC_MAX) */
    /*              throw except::Exception(Ctxt("You cannot exceed the ILOC_MAX")); */

    /*          maxProductSize = maxSize; */
    /*          numRowsLimit = maxRows; */
    /*      } */

};

}

#endif
