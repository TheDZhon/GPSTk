
/**
 * @file ComputePC.hpp
 * This class eases computing PC combination for GNSS data structures.
 */

#ifndef COMPUTE_PC_GPSTK
#define COMPUTE_PC_GPSTK

//============================================================================
//
//  This file is part of GPSTk, the GPS Toolkit.
//
//  The GPSTk is free software; you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published
//  by the Free Software Foundation; either version 2.1 of the License, or
//  any later version.
//
//  The GPSTk is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with GPSTk; if not, write to the Free Software Foundation,
//  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//  
//  Dagoberto Salazar - gAGE ( http://www.gage.es ). 2007
//
//============================================================================



#include "ComputeCombination.hpp"


namespace gpstk
{

    /** @addtogroup DataStructures */
    //@{


    /** This class eases computing PC combination for GNSS data structures.
     * This class is meant to be used with the GNSS data structures objects
     * found in "DataStructures" class.
     *
     * A typical way to use this class follows:
     *
     * @code
     *   RinexObsStream rin("ebre0300.02o");
     *
     *   gnssRinex gRin;
     *   ComputePC getPC;
     *
     *   while(rin >> gRin) {
     *      gRin >> getPC;
     *   }
     * @endcode
     *
     * The "ComputePC" object will visit every satellite in the GNSS data
     * structure that is "gRin" and will try to compute its PC combination.
     *
     * When used with the ">>" operator, this class returns the same incoming
     * data structure with the PC inserted along their corresponding
     * satellites. Be warned that if a given satellite does not have the 
     * observations required, it will be summarily deleted from the data
     * structure.
     *
     * Sometimes, the Rinex observations file does not have P1, but provides C1
     * instead. In such cases, you must use the useC1() method.
     *
     * @warning When using C1 instead of P1 to compute this combination, be aware
     * that instrumental errors will NOT cancel, introducing a bias that must be
     * taken into account by other means.
     *
     */
    class ComputePC : public ComputeCombination
    {
    public:

        /// Default constructor
        ComputePC() : DEN(0.646944444)
        {
            type1 = TypeID::P1;
            type2 = TypeID::P2;
            resultType = TypeID::PC;
            setIndex();
        };


        /** Returns a satTypeValueMap object, adding the new data generated when calling this object.
         *
         * @param gData     Data object holding the data.
         */
        virtual satTypeValueMap& Process(satTypeValueMap& gData)
        {
            ComputeCombination::Process(gData);

            return gData;
        }


        /// Some Rinex data files provide C1 instead of P1. Use this method in those cases.
        void useC1() { type1 = TypeID::C1; };


        /// Returns an index identifying this object.
        virtual int getIndex(void) const;


        /// Returns a string identifying this object.
        virtual std::string getClassName(void) const;


        /** Sets the index to a given arbitrary value. Use with caution.
         *
         * @param newindex      New integer index to be assigned to current object.
         */
        void setIndex(const int newindex) { (*this).index = newindex; };


        /// Destructor
        virtual ~ComputePC() {};


    protected:

        /// Compute the combination of observables.
        virtual double getCombination(const double& obs1, const double& obs2)
        {
            return ( (GAMMA_GPS*obs1 - obs2)/(DEN) );
        };


    private:

        const double DEN;     // DEN = GAMMA_GPS - 1

        /// Initial index assigned to this class.
        static int classIndex;

        /// Index belonging to this object.
        int index;

        /// Sets the index and increment classIndex.
        void setIndex(void) { (*this).index = classIndex++; }; 

   }; // end class ComputePC
   

   //@}
   
}

#endif
