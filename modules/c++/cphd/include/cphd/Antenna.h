/******************************************************************************
 * (C) Copyright 2004 - 2014, MDA Information Systems LLC
 *
 * Developed in the performance of one or more U.S. Government contracts.
 * The U.S. Government has Unlimited Rights in this computer software as set
 * forth in the Rights in Technical Data and Compute Software: Noncommercial
 * Items clauses.
 ******************************************************************************/


#ifndef __CPHD_ANTENNA_H__
#define __CPHD_ANTENNA_H__

#include <ostream>
#include <vector>

#include <six/sicd/Antenna.h>

namespace cphd
{
/*!
 *  \struct ElectricalBoresight
 *  \brief CPHD EB Parameter
 *
 *  Electrical boresight steering directions
 *  the DCXPoly is the EB steering x-axis direction
 *  cosine (DCX) as a function of slow time
 */
typedef six::sicd::ElectricalBoresight ElectricalBoresight;

/*!
 *  \struct HalfPowerBeamwidths
 *  \brief CPHD HPBW Parameter
 *
 *  Half-power beamwidths.  For electronically steered
 *  arrays, the EB is steered to DCX = 0, DCY = 0
 */
typedef six::sicd::HalfPowerBeamwidths HalfPowerBeamwidths;

/*!
 *  \struct GainAndPhasePolys
 *  \brief CPHD Array & Elem Parameter
 *
 *  Mainlobe array pattern polynomials.  For
 *  electronically steered arrays, the EB is steered to
 *  DCX = 0, DCY = 0.
 */
typedef six::sicd::GainAndPhasePolys GainAndPhasePolys;

/*!
 *  \struct AntennaParameters
 *  \brief CPHD Tx/Rcv, etc Antenna parameter
 *
 */
typedef six::sicd::AntennaParameters AntennaParameters;

/*!
 *  \struct Antenna
 *  \brief CPHD Antenna parameter
 *
 *  Parameters that describe the antenna(s) during collection.
 *  Parameters describe the antenna orientation, pointing direction
 *  and beamshape during the collection.
 *
 *  There can be a as many sets of antenna parameters as there are
 *  channels in the collection
 *  Parameters may be provided separably for the transmit (Tx) antenna
 *  and the receieve (Rcv) antenna.  A single set of prarameters may
 *  be provided for a combined two-way pattern (as appropriate)
 */
 struct Antenna
{
    //!  Constructor
    Antenna();

    bool operator==(const Antenna& other) const
    {
        return numTxAnt == other.numTxAnt &&
               numRcvAnt == other.numRcvAnt &&
               numTWAnt == other.numTWAnt &&
               tx == other.tx &&
               rcv == other.rcv &&
               twoWay == other.twoWay;
    }

    bool operator!=(const Antenna& other) const
    {
        return !((*this) == other);
    }

    //! Number of Transmit Antennae - up to one per channel
    size_t numTxAnt;

    //! Number of Receive Antennae - up to one per channel
    size_t numRcvAnt;

    //! Number of Two-Way Antennae - up to one per channel
    size_t numTWAnt;

    //! Transmit parameters
    std::vector<AntennaParameters> tx;

    //! Receive parameters
    std::vector<AntennaParameters> rcv;

    //! Two way parameters
    std::vector<AntennaParameters> twoWay;
};

std::ostream& operator<< (std::ostream& os, const Antenna& d);
}

#endif