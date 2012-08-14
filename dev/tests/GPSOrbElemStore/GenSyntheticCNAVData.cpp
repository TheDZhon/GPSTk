#pragma ident "$Id:$"
/**
*
*  This program reads an FIC file, filters it down to the 
*  Block 109 data, and generates pseudo-CNAV (or CNAV-2) data
*  in "as broadcast binary" format.  The original purpose 
*  was to test the OrbElemICE/OrbElemCNAV/OrbElemCNAV2 
*  classes.  See the corresponding test reader program.  
*  (That program hasn't been written when this comment was
*  generated.)
*
*  Command line
*    -i : input file
*    -o : output file
*    -t : ObsType to output - <TBD Add examples>
*
*
*  Output format
*  
*  Any line beginning with '!' in considered a comment.
*
* For CNAV (TBD - need three messages)
*  Gpp ooo wwww ssssss   
*  0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX
*  0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX
*  0xYYYYYYYY 0xYYYYYYYY 0xYYYYYYYY 0xYYYYYYYY 0xYYYYYYYY
*  0xYYYYYYYY 0xYYYYYYYY 0xYYYYYYYY 0xYYYYYYYY 0xYYYYYYYY
*  0xZZZZZZZZ 0xZZZZZZZZ 0xZZZZZZZZ 0xZZZZZZZZ 0xZZZZZZZZ
*  0xZZZZZZZZ 0xZZZZZZZZ 0xZZZZZZZZ 0xZZZZZZZZ 0xZZZZZZZZ
*
*  G = GPS
*  pp = PRN ID
*  ooo = ObsID string
*  xMit time = in week # (wwww) and SOW (ssssss)
*  XXXXXXXX = Message Type 10, lef-justified, 32 bits per word
*             (300 bits - 9.38 32-bit words)
*  YYYYYYYY = Message Type 11, lef-justified, 32 bits per word
*             (300 bits - 9.38 32-bit words)
*  ZZZZZZZZ = One of Message Type 30-37, lef-justified, 32 bits per word
*             (300 bits - 9.38 32-bit words)
*
* For CNAV-2 
*  Gpp ooo wwww ssssss   0xYYYY
*  0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX
*  0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX
*  0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX
*  0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX
*  0xXXXXXXXX 0xXXXXXXXX 0xXXXXXX00 
*
*  G = GPS
*  pp = PRN ID
*  ooo = ObsID string
*  xMit time = in week # (wwww) and SOW (ssssss)
*  YYYY = Subframe 1
*  XXXXXXXX = Subframe 2, left-justified, 32 bits per word
*             (600 bits - 18.75 32-bit words)
* 
*  Note that one can (must) infer CNAV or CNAV-2 from the ObsID.
* 
*/
// System
#include <iostream>
#include <fstream>
#include <cstdlib>

// gpstk
#include "FileFilterFrame.hpp"
#include "BasicFramework.hpp"
#include "StringUtils.hpp"
#include "gps_constants.hpp"
#include "PackedNavBits.hpp"
#include "SatID.hpp"
#include "ObsID.hpp"
#include "OrbElemFIC109.hpp"
#include "TimeString.hpp"
#include "TimeConstants.hpp"
#include "GNSSconstants.hpp"

// fic
#include "FICStream.hpp"
#include "FICHeader.hpp"
#include "FICData.hpp"
#include "FICFilterOperators.hpp"

// Project

using namespace std;
using namespace gpstk;

class GenSyntheticCNAVData : public gpstk::BasicFramework
{
public:
   GenSyntheticCNAVData(const std::string& applName,
              const std::string& applDesc) throw();
   ~GenSyntheticCNAVData() {}
   virtual bool initialize(int argc, char *argv[]) throw();
   
protected:
   virtual void process();
   gpstk::CommandOptionWithAnyArg inputOption;
   gpstk::CommandOptionWithAnyArg outputOption;
   gpstk::CommandOptionWithAnyArg obsTypeOption;

   std::list<long> blockList;

   void convertCNAV(ofstream& out, const OrbElemFIC109& oe);
   void convertCNAV2(ofstream& out, const OrbElemFIC109& oe);

   bool outputCNAV; 

   //ofstream out;
};

int main( int argc, char*argv[] )
{
   try
   {
      GenSyntheticCNAVData fc("GenSyntheticCNAVData", "");
      if (!fc.initialize(argc, argv)) return(false);
      fc.run();
   }
   catch(gpstk::Exception& exc)
   {
      cout << exc << endl;
      return 1;
   }
   catch(...)
   {
      cout << "Caught an unnamed exception. Exiting." << endl;
      return 1;
   }
   return 0;
}

GenSyntheticCNAVData::GenSyntheticCNAVData(const std::string& applName, 
                       const std::string& applDesc) throw()
          :BasicFramework(applName, applDesc),
           inputOption('i', "input-file", "The name of the FIC file to be read.", true),
           outputOption('o', "output-file", "The name of the output file to write.", true),
           obsTypeOption('t',"obs type","obs type to simulate: CNAV or CNAV-2.",true),
           outputCNAV(false)
{
   inputOption.setMaxCount(1); 
   outputOption.setMaxCount(1);
   obsTypeOption.setMaxCount(1);
}

bool GenSyntheticCNAVData::initialize(int argc, char *argv[])
   throw()
{
   if (!BasicFramework::initialize(argc, argv)) return false;
   
   if (debugLevel)
   {
      cout << "Output File: " << outputOption.getValue().front() << endl;
   }
   
      // Set up the FIC data filter
   blockList.push_back(109);

   string typeCheck = obsTypeOption.getValue().front();
   if       (typeCheck.compare("CNAV-2")==0) outputCNAV = false;
    else if (typeCheck.compare("CNAV")==0) outputCNAV = true;
    else 
    {
       cerr << "Type must be 'CNAV' or 'CNAV-2'" << endl;
       return false;
    }

                
   return true;   
}

void GenSyntheticCNAVData::process()
{

      // Open the output stream
   char ofn[100];
   strcpy( ofn, outputOption.getValue().front().c_str());
   ofstream out( ofn, ios::out );
   if (!out) 
   {
      cerr << "Failed to open output file. Exiting." << endl;
      exit(1);
   }
 
   string fn = inputOption.getValue().front();
   cout << "Attempting to read from file '" << fn << "'" << endl;
   FileFilterFrame<FICStream, FICData> input(fn);
   int recordCount = 0; 


      // Put descriptive header into the output file
   if (outputCNAV) 
   {
      out << "!  Synthetic CNAV data generated from Legcay Nav Data" << endl;
      out << "!  Input File: " << fn << endl; 
      out << "!  " << endl; 
      out << "!  Gpp ooo wwww ssssss" << endl;
      out << "!  0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX" << endl;
      out << "!  0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX" << endl;
      out << "!  0xYYYYYYYY 0xYYYYYYYY 0xYYYYYYYY 0xYYYYYYYY 0xYYYYYYYY" << endl;
      out << "!  0xYYYYYYYY 0xYYYYYYYY 0xYYYYYYYY 0xYYYYYYYY 0xYYYYYYYY" << endl;
      out << "!  0xZZZZZZZZ 0xZZZZZZZZ 0xZZZZZZZZ 0xZZZZZZZZ 0xZZZZZZZZ" << endl;
      out << "!  0xZZZZZZZZ 0xZZZZZZZZ 0xZZZZZZZZ 0xZZZZZZZZ 0xZZZZZZZZ" << endl;
      out << "!  " << endl; 
      out << "!  G = GPS" << endl;
      out << "!  pp = PRN ID" << endl;
      out << "!  ooo = ObsID string" << endl;
      out << "!  xMit time = in week # (wwww) and SOW (ssssss)" << endl;
      out << "!  XXXXXXXX = Message Type 10, lef-justified, 32 bits per word" << endl;
      out << "!             (300 bits - 9.38 32-bit words)" << endl;
      out << "!  YYYYYYYY = Message Type 11, lef-justified, 32 bits per word" << endl;
      out << "!             (300 bits - 9.38 32-bit words)" << endl;
      out << "!  ZZZZZZZZ = One of Message Type 30-37, lef-justified, 32 bits per word" << endl;
      out << "!             (300 bits - 9.38 32-bit words)" << endl;
      out << "!" << endl; 
   }
   else
   {
      out << "!  Synthetic CNAV-2 data generated from Legcay Nav Data" << endl;
      out << "!  Input File: " << fn << endl; 
      out << "!  " << endl; 
      out << "!  Gpp ooo wwww ssssss   0xYYYY" << endl;
      out << "!  0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX" << endl;
      out << "!  0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX" << endl;
      out << "!  0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX" << endl;
      out << "!  0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX" << endl;
      out << "!  0xXXXXXXXX 0xXXXXXXXX 0xXXXXXX00 " << endl;
      out << "!  " << endl; 
      out << "!  G = GPS" << endl;
      out << "!  pp = PRN ID" << endl;
      out << "!  ooo = ObsID string" << endl;
      out << "!  xMit time = in week # (wwww) and SOW (ssssss)" << endl;
      out << "!  YYYY = Subframe 1" << endl;
      out << "!  XXXXXXXX = Subframe 2, left-justified, 32 bits per word" << endl;
      out << "!             (600 bits - 18.75 32-bit words)" << endl;
      out << "!" << endl; 
   
   }

      // Filter the FIC data for the requested vlock(s)
   input.filter(FICDataFilterBlock(blockList));

   list<FICData>& ficList = input.getData();
   cout << "Read " << input.size() << " records from input file" << endl;
   list<FICData>::iterator itr = ficList.begin();
   
   while (itr != ficList.end())
   {
      recordCount++;
         
      FICData& r = *itr;
      OrbElemFIC109 oe(r);

      if (outputCNAV) convertCNAV( out, oe );
       else convertCNAV2( out, oe );
      
      itr++;
   }  // End of FIC Block loop

}

void GenSyntheticCNAVData::convertCNAV(ofstream& out, const OrbElemFIC109& oe)
{
   out << "G" << setw(2) << setfill('0') <<  dec << oe.satID.id;
   out << setfill(' ') << " L2C ";

      //Debug
   cout << "G" << setw(2) << setfill('0') << oe.satID.id;
   cout << setfill(' ') << " L2C " << 
             printTime(oe.transmitTime,"%04F %06.0g") << endl; 

      // Synthesize an ObsID such that this data APPEARS
      // to have come from L2C M+L.
   ObsID obsID( ObsID::otNavMsg, ObsID::cbL2, ObsID::tcC2LM);

      // Instantiate a PackedNavBits object to contain the Subframe 2 data
   PackedNavBits pnb( oe.satID, obsID, oe.transmitTime ); 

      // Timing considerations.  See IS-GPS-705 Section 20.3.2 and 20.3.4.1
      // (Table 20-XII, Message Broadcast Intervals.)
      // For purposes of synthesizing data, the program will assume a 24 sec
      // repeat interval for Message Types 10/11/(30-37).  The time tags in
      // the messages refer to the time at the beginning of the NEXT message.
      //
      // oe.transmitTime represents the beginning time of the tranmission of
      // this message.  The legacy navigation message is based on 30-second
      // subframes.  Since we are assuming a 24-second repeat for CNAV some 
      // adjustments need to be made. 
   double SOWBeginLegacyMsg = (static_cast<GPSWeekSecond>(oe.transmitTime)).sow;

      // Round BACK to nearest 24 second interval.  
   double adjSOWBegin = (double) (( (long)SOWBeginLegacyMsg / 24 ) * 24);
   long msg10TOW = (long) adjSOWBegin + 6;
   long msg11TOW = msg10TOW + 6;
   long msgClkTOW = msg11TOW + 6;
   cout << "msg 10, 11, Clk: " << msg10TOW << ", " << msg11TOW << ", "  << msgClkTOW << endl;

      // ---End Timing considerations

      // Output the week number
   int weekNum = (static_cast<GPSWeekSecond>(oe.transmitTime)).week;
   out << setw(4) << weekNum << "  ";

      // output the transmit SOW (as adjusted)
   out << setw(6) << setfill('0') << (unsigned long)adjSOWBegin << endl;

      // Set up transmission times
   CommonTime ctMsg10  = GPSWeekSecond(weekNum, msg10TOW);
   CommonTime ctMsg11  = GPSWeekSecond(weekNum, msg11TOW);
   CommonTime ctMsgClk = GPSWeekSecond(weekNum, msgClkTOW);

      // Message Type 10
   unsigned long PREAMBLEMsg10 = 139;   // 0x8B by another base
   int n_PREAMBLEMsg10         = 8;
   int s_PREAMBLEMsg10         = 1;

   unsigned long PRNIDMsg10    = oe.satID.id;
   int n_PRNIDMsg10            = 6;
   int s_PRNIDMsg10            = 1;

   unsigned long Msg10ID       = 10;
   int n_Msg10ID               = 6;
   int s_Msg10ID               = 1;

   unsigned long TOWMsg10      = msg10TOW;
   int n_TOWMsg10              = 17;
   int s_TOWMsg10              = 6;

   unsigned long AlertMsg10    = 0;
   int n_AlertMsg10            = 1;
   int s_AlertMsg10            = 1;

   unsigned long TOWWeek       = (static_cast<GPSWeekSecond>(oe.transmitTime)).week;
   int n_TOWWeek               = 13;
   int s_TOWWeek               = 1;

   unsigned long healthTrans   = 0;
   if (!oe.healthy) healthTrans = 1;
   unsigned long L1Health      = healthTrans;
   int n_L1Health              = 1;
   int s_L1Health              = 1;

   unsigned long L2Health      = 0;
   int n_L2Health              = 1;
   int s_L2Health              = 1;

   unsigned long L5Health      = 0;
   int n_L5Health              = 1;
   int s_L5Health              = 1;
   
      // Form estimate of Top based on Toe and AODO
   double dTop                 = (static_cast<GPSWeekSecond>(oe.ctToe)).sow;
   dTop                       -= oe.AODO;
   if (dTop<0.0) dTop += FULLWEEK;
   unsigned long Top           = (unsigned long) dTop;
   int n_Top                   = 11;
   int s_Top                   = 300;

   long URAed                  = oe.accFlag;
   int n_URAed                 = 5;
   int s_URAed                 = 1;

   unsigned long rToeMsg10     = (static_cast<GPSWeekSecond>(oe.ctToe)).sow;
   int n_rToeMsg10             = 11;
   int s_rToeMsg10             = 300;

   double deltaA               = oe.A - A_REF_GPS;
   int n_deltaA                = 26;
   int s_deltaA                = -9;

   double Adot                 = 0;
   int n_Adot                  = 25;
   int s_Adot                  = -21;

   double rdn                  = oe.dn;
   int n_rdn                   = 17;
   int s_rdn                   = -44;

   double dndot                = 0;
   int n_dndot                 = 23;
   int s_dndot                 = -57;

   double rM0                  = oe.M0;
   int n_rM0                   = 33;
   int s_rM0                   = -32;

   double recc                 = oe.ecc;
   int n_recc                  = 33;
   int s_recc                  = -34;

   double rw                   = oe.w;
   int n_rw                    = 33;
   int s_rw                    = -32;

   unsigned long sflag         = 0;
   int n_sflag                 = 1;
   int s_sflag                 = 1;

   unsigned long L2CPhasing    = 0;
   int n_L2CPhasing            = 1;
   int s_L2CPhasing            = 1;

   unsigned long reservedBitsMsg10 = 0;
   int n_reservedBitsMsg10         = 3;
   int s_reservedBitsMsg10         = 1;

   unsigned long CRCMsg10      = 0;
   int n_CRCMsg10              = 24;
   int s_CRCMsg10              = 1;

   // Message Type 11
   unsigned long PREAMBLEMsg11 = 139;
   int n_PREAMBLEMsg11         = 8;
   int s_PREAMBLEMsg11         = 1;

   unsigned long PRNIDMsg11    = oe.satID.id;
   int n_PRNIDMsg11            = 6;
   int s_PRNIDMsg11            = 1;

   unsigned long Msg11ID       = 11;
   int n_Msg11ID               = 6;
   int s_Msg11ID               = 1; 

   unsigned long TOWMsg11      = msg11TOW;
   int n_TOWMsg11              = 17;
   int s_TOWMsg11              = 6;

   unsigned long AlertMsg11    = 0;
   int n_AlertMsg11            = 1;
   int s_AlertMsg11            = 1;

   unsigned long rToeMsg11     = (static_cast<GPSWeekSecond>(oe.ctToe)).sow;
   int n_rToeMsg11             = 11;
   int s_rToeMsg11             = 300;

   double rOMEGA0              = oe.OMEGA0;
   int n_rOMEGA0               = 33;
   int s_rOMEGA0               = -32;

   double ri0                  = oe.i0;
   int n_ri0                   = 33;
   int s_ri0                   = -32;

   double rOMEGAdot            = oe.OMEGAdot;
   int n_rOMEGAdot             = 24;
   int s_rOMEGAdot             = -43;

   double deltaOMEGAdot        = rOMEGAdot - OMEGADOT_REF_GPS;
   int n_deltaOMEGAdot         = 17;
   int s_deltaOMEGAdot         = -44;

   double ridot     = oe.idot;
   int n_ridot      = 15;
   int s_ridot      = -44;

   double rCis      = oe.Cis;
   int n_rCis       = 16;
   int s_rCis       = -30;

   double rCic      = oe.Cic;
   int n_rCic       = 16;
   int s_rCic       = -30;

   double rCrs      = oe.Crs;
   int n_rCrs       = 24;
   int s_rCrs       = -8;

   double rCrc      = oe.Crc;
   int n_rCrc       = 24;
   int s_rCrc       = -8;

   double rCus      = oe.Cus;
   int n_rCus       = 21;
   int s_rCus       = -30;

   double rCuc      = oe.Cuc;
   int n_rCuc       = 21;
   int s_rCuc       = -30;

   unsigned long reservedBitsMsg11 = 0;
   int n_reservedBitsMsg11         = 7;
   int s_reservedBitsMsg11         = 1;

   unsigned long CRCMsg11 = 0;
   int n_CRCMsg11         = 24;
   int s_CRCMsg11         = 1;

   
      // Message Type Clk
   unsigned long PREAMBLE = 139;
   int n_PREAMBLE         = 8;
   int s_PREAMBLE         = 1;

   unsigned long PRNID    = oe.satID.id;
   int n_PRNID            = 6;
   int s_PRNID            = 1;

   unsigned long MsgID    = 30;
   int n_MsgID            = 6;
   int s_MsgID            = 1;

   unsigned long TOWMsg   = msgClkTOW;
   int n_TOWMsg           = 17;
   int s_TOWMsg           = 6;

   unsigned long Alert    = 0;
   int n_Alert            = 1;
   int s_Alert            = 1;

   long URAned0           = oe.accFlag;
   int n_URAned0          = 5;
   int s_URAned0          = 1;

   unsigned long URAned1  = 1;   // No value provided in legacy
   int n_URAned1          = 3;
   int s_URAned1          = 1;

   unsigned long URAned2  = 2;   // No value provided in legacy
   int n_URAned2          = 3;
   int s_URAned2          = 1;

   unsigned long Toc      = (static_cast<GPSWeekSecond>(oe.ctToe)).sow;
   int n_Toc              = 11;
   int s_Toc              = 300;
   
   double af0             = oe.af0;
   int n_af0              = 26;
   int s_af0              = -35;

   double af1             = oe.af1;
   int n_af1              = 20;
   int s_af1              = -48;

   double af2             = oe.af2;
   int n_af2              = 10;
   int s_af2              = -60;

      // Following the clock data, there are 149 bits of "message unique"
      // data.  For our purposes, these need to be padded.  Use alternating
      // 1's and 0's.
   unsigned long padBits  = 0xAAAAAAAA;
   int n_padBits          = 32;           // Need 4 of these
   int s_padBits          = 1;

   unsigned long padBits2 = 0x00015555;   // and one of these
   int n_padBits2         = 21;          
   int s_padBits2         = 1;


   unsigned long CRCMsgClk = 0;
   int n_CRCMsgClk         = 24;
   int s_CRCMsgClk         = 1;

      // First Test Case. Create PNB objects in which to store Message Type 10 and 11 data.
   PackedNavBits pnb10;
   PackedNavBits pnb11;
   
   /* Pack Message Type 10 data */
   pnb10.setSatID(oe.satID);
   pnb10.setObsID(obsID);
   pnb10.setTime(ctMsg10);
   pnb10.addUnsignedLong(PREAMBLEMsg10, n_PREAMBLEMsg10, s_PREAMBLEMsg10);
   pnb10.addUnsignedLong(PRNIDMsg10, n_PRNIDMsg10, s_PRNIDMsg10);
   pnb10.addUnsignedLong(Msg10ID, n_Msg10ID, s_Msg10ID);
   pnb10.addUnsignedLong(TOWMsg10, n_TOWMsg10, s_TOWMsg10);
   pnb10.addUnsignedLong(AlertMsg10, n_AlertMsg10, s_AlertMsg10);
   pnb10.addUnsignedLong(TOWWeek, n_TOWWeek, s_TOWWeek);
   pnb10.addUnsignedLong(L1Health, n_L1Health, s_L1Health);
   pnb10.addUnsignedLong(L2Health, n_L2Health, s_L2Health);
   pnb10.addUnsignedLong(L5Health, n_L5Health, s_L5Health);
   pnb10.addUnsignedLong(Top, n_Top, s_Top);
   pnb10.addLong(URAed, n_URAed, s_URAed);
   pnb10.addUnsignedLong(rToeMsg10, n_rToeMsg10, s_rToeMsg10);
   pnb10.addSignedDouble(deltaA, n_deltaA, s_deltaA);
   pnb10.addSignedDouble(Adot, n_Adot, s_Adot);
   pnb10.addDoubleSemiCircles(rdn, n_rdn, s_rdn);
   pnb10.addDoubleSemiCircles(dndot, n_dndot, s_dndot);
   pnb10.addDoubleSemiCircles(rM0, n_rM0, s_rM0);
   pnb10.addUnsignedDouble(recc, n_recc, s_recc);
   pnb10.addDoubleSemiCircles(rw, n_rw, s_rw);
   pnb10.addUnsignedLong(sflag, n_sflag, s_sflag);
   pnb10.addUnsignedLong(L2CPhasing, n_L2CPhasing, s_L2CPhasing);
   pnb10.addUnsignedLong(reservedBitsMsg10, n_reservedBitsMsg10, s_reservedBitsMsg10);
   pnb10.addUnsignedLong(CRCMsg10, n_CRCMsg10, s_CRCMsg10);

      /* Pack Message Type 11 data */
   pnb11.setSatID(oe.satID);
   pnb11.setObsID(obsID);
   pnb11.setTime(ctMsg11);
   pnb11.addUnsignedLong(PREAMBLEMsg11, n_PREAMBLEMsg11, s_PREAMBLEMsg11);
   pnb11.addUnsignedLong(PRNIDMsg11, n_PRNIDMsg11, s_PRNIDMsg11);
   pnb11.addUnsignedLong(Msg11ID, n_Msg11ID, s_Msg11ID);
   pnb11.addUnsignedLong(TOWMsg11, n_TOWMsg11, s_TOWMsg11);
   pnb11.addUnsignedLong(AlertMsg11, n_AlertMsg11, s_AlertMsg11);
   pnb11.addUnsignedLong(rToeMsg11, n_rToeMsg11, s_rToeMsg11);
   pnb11.addDoubleSemiCircles(rOMEGA0, n_rOMEGA0, s_rOMEGA0);
   pnb11.addDoubleSemiCircles(ri0, n_ri0, s_ri0);
   pnb11.addDoubleSemiCircles(deltaOMEGAdot, n_deltaOMEGAdot, s_deltaOMEGAdot);
   pnb11.addDoubleSemiCircles(ridot, n_ridot, s_ridot); 
   pnb11.addSignedDouble(rCis, n_rCis, s_rCis);
   pnb11.addSignedDouble(rCic, n_rCic, s_rCic);
   pnb11.addSignedDouble(rCrs, n_rCrs, s_rCrs);
   pnb11.addSignedDouble(rCrc, n_rCrc, s_rCrc);
   pnb11.addSignedDouble(rCus, n_rCus, s_rCus);
   pnb11.addSignedDouble(rCuc, n_rCuc, s_rCuc);
   pnb11.addUnsignedLong(reservedBitsMsg11, n_reservedBitsMsg11, s_reservedBitsMsg11);
   pnb11.addUnsignedLong(CRCMsg11, n_CRCMsg11, s_CRCMsg11);

      // Create PNB object in which to store the first 128 bits of Message Types 30-37.
   PackedNavBits pnb3_;

      /* Pack Message Type 30-37 data */
   pnb3_.setSatID(oe.satID);
   pnb3_.setObsID(obsID);
   pnb3_.setTime(ctMsgClk);
   pnb3_.addUnsignedLong(PREAMBLE, n_PREAMBLE, s_PREAMBLE);
   pnb3_.addUnsignedLong(PRNID, n_PRNID, s_PRNID);
   pnb3_.addUnsignedLong(MsgID, n_MsgID, s_MsgID);
   pnb3_.addUnsignedLong(TOWMsg, n_TOWMsg, s_TOWMsg);
   pnb3_.addUnsignedLong(Alert, n_Alert, s_Alert);
   pnb3_.addUnsignedLong(Top, n_Top, s_Top);
   pnb3_.addLong(URAned0, n_URAned0, s_URAned0);
   pnb3_.addUnsignedLong(URAned1, n_URAned1, s_URAned1);
   pnb3_.addUnsignedLong(URAned2, n_URAned2, s_URAned2);
   pnb3_.addUnsignedLong(Toc, n_Toc, s_Toc);
   pnb3_.addSignedDouble(af0, n_af0, s_af0);
   pnb3_.addSignedDouble(af1, n_af1, s_af1);
   pnb3_.addSignedDouble(af2, n_af2, s_af2);
   pnb3_.addUnsignedLong(padBits, n_padBits, s_padBits);
   pnb3_.addUnsignedLong(padBits, n_padBits, s_padBits);
   pnb3_.addUnsignedLong(padBits, n_padBits, s_padBits);
   pnb3_.addUnsignedLong(padBits, n_padBits, s_padBits);
   pnb3_.addUnsignedLong(padBits2, n_padBits2, s_padBits2);
   pnb3_.addUnsignedLong(CRCMsgClk, n_CRCMsgClk, s_CRCMsgClk);

      /* Resize the vectors holding the packed nav message data. */
   pnb10.trimsize();
   pnb11.trimsize();
   pnb3_.trimsize();

      // Output the results 
   pnb10.outputPackedBits(out);
   out << endl;
   pnb11.outputPackedBits(out);
   out << endl;
   pnb3_.outputPackedBits(out);
   out << endl;
}


void GenSyntheticCNAVData::convertCNAV2(ofstream& out, const OrbElemFIC109& oe)
{
   out << "G" << setw(2) << setfill('0') <<  dec << oe.satID.id;
   out << setfill(' ') << " L1C ";

      //Debug
   cout << "G" << setw(2) << setfill('0') << oe.satID.id;
   cout << setfill(' ') << " L1C " << 
             printTime(oe.transmitTime,"%04F %06.0g") << endl; 
      
      // Synthesize an ObsID such that this data APPEARS
      // to have come from L1C.
      // Need to add an L1C tracking code to ObsID
   ObsID obsID( ObsID::otNavMsg, ObsID::cbL1, ObsID::tcAny);

      // Instantiate a PackedNavBits object to contain the Subframe 2 data
   PackedNavBits pnb( oe.satID, obsID, oe.transmitTime ); 

      // Timing considerations.  See IS-GPS-800 Section 3.5.2
      // Subframe 1 is a 9-bit value (TOI count) that represents the number of
      // 18-second intervals since the last two-hour epoch that will
      // be complete at the END of this frame.   
      // That is to say, the TOI in the FIRST frame at an even two-hour
      // epoch will be "1".  The TOI in the LAST frame before the two-hour
      // epoch will be "0".
      //
      // oe.transmitTime represents the beginning time of the tranmission of
      // this message.  The legacy navigation message is based on 30-second
      // subframes and CNAV-2 has a 18-second frame rate.  Therefore, some 
      // adjustments need to be made. 
   double SOWBeginLegacyMsg = (static_cast<GPSWeekSecond>(oe.transmitTime)).sow;

      // Round BACK to nearest 18 second interval.  
   double adjSOWBegin = (double) (( (long)SOWBeginLegacyMsg / 18 ) * 18);

   int numTwoHourEpochs = (long) adjSOWBegin / 7200; 
   double secSinceLastTwoHourEpoch =  adjSOWBegin - (numTwoHourEpochs * 7200); 
   int currentNumCNAV2Frames = secSinceLastTwoHourEpoch / 18;
   int nextNumCNAV2Frames = currentNumCNAV2Frames + 1; 
      // ---End Timing considerations

      // Debug
   //cout << "SOWBeginLegacyMsg        : " << SOWBeginLegacyMsg << endl;
   //cout << "adjSOWBegin              : " << adjSOWBegin << endl;
   //cout << "numTwoHourEpochs         : " << numTwoHourEpochs << endl;
   //cout << "secSinceLastTwoHourEpoch : " << secSinceLastTwoHourEpoch << endl;
   //cout << "nextNumCNAV2Frames       : " << nextNumCNAV2Frames << endl; 


      // Output the week number
   int weekNum = (static_cast<GPSWeekSecond>(oe.transmitTime)).week;
   out << setw(4) << weekNum << "  ";

      // output the transmit SOW (as adjusted)
   out << setw(6) << setfill('0') << (unsigned long)adjSOWBegin << "  "; 


      // Output subframe 1 (TOI count)
   out << "0x" << setw(4) << setfill('0') << hex << nextNumCNAV2Frames << endl; 

      // Translate data from FIC 109 legacy format to 
      // CNAV 2 structure.
   unsigned long TOWWeek   = (static_cast<GPSWeekSecond>(oe.transmitTime)).week;
   int n_TOWWeek           = 13;
   int s_TOWWeek           = 1;

   unsigned long ITOW      = numTwoHourEpochs;
   int n_ITOW              = 8;
   int s_ITOW              = 1;

      // Form estimate of Top based on Toe and AODO
   double dTop             = (static_cast<GPSWeekSecond>(oe.ctToe)).sow;
   dTop                   -= oe.AODO;        // Convert from counts to seconds, 
                                             // then subtract from Toe.
   if (dTop<0.0) dTop += FULLWEEK;
   unsigned long Top       = (unsigned long) dTop;
   int n_Top               = 11;
   int s_Top               = 300;

   unsigned long L1CHealth = 0;
   if (!oe.healthy) L1CHealth = 1;
   int n_L1CHealth         = 1;
   int s_L1CHealth         = 1;

   long URAed              = oe.accFlag;
   int n_URAed             = 5;
   int s_URAed             = 1;

   double fullToeSOW       = (static_cast<GPSWeekSecond>(oe.ctToe)).sow;
   unsigned long Toe       = fullToeSOW; 
   int n_Toe               = 11;
   int s_Toe               = 300;

   double deltaA           = oe.A - A_REF_GPS;
   int n_deltaA            = 26;
   int s_deltaA            = -9;

   double Adot             = 0;
   int n_Adot              = 25;
   int s_Adot              = -21;

   double dn               = oe.dn;
   int n_dn                = 17;
   int s_dn                = -44;

   double dndot            = 0;
   int n_dndot             = 23;
   int s_dndot             = -57;

   double M0               = oe.M0;
   int n_M0                = 33;
   int s_M0                = -32;

   double ecc              = oe.ecc;
   int n_ecc               = 33;
   int s_ecc               = -34;

   double w                = oe.w;
   int n_w                 = 33;
   int s_w                 = -32;

   double OMEGA0           = oe.OMEGA0;
   int n_OMEGA0            = 33;
   int s_OMEGA0            = -32;

   double i0               = oe.i0;
   int n_i0                = 33;
   int s_i0                = -32;

   double OMEGAdot         = oe.OMEGAdot;
   int n_OMEGAdot          = 24;
   int s_OMEGAdot          = -43;

   double deltaOMEGAdot    = OMEGAdot - OMEGADOT_REF_GPS;
   int n_deltaOMEGAdot     = 17;
   int s_deltaOMEGAdot     = -44;

   double idot             = oe.idot;
   int n_idot              = 15;
   int s_idot              = -44;

   double Cis              = oe.Cis;
   int n_Cis               = 16;
   int s_Cis               = -30;

   double Cic              = oe.Cic;
   int n_Cic               = 16;
   int s_Cic               = -30;

   double Crs              = oe.Crs;
   int n_Crs               = 24;
   int s_Crs               = -8;

   double Crc              = oe.Crc;
   int n_Crc               = 24;
   int s_Crc               = -8;

   double Cus              = oe.Cus;
   int n_Cus               = 21;
   int s_Cus               = -30;

   double Cuc              = oe.Cuc;
   int n_Cuc               = 21;
   int s_Cuc               = -30;

   long URAned0              = oe.accFlag;
   int n_URAned0             = 5;
   int s_URAned0             = 1;

   unsigned long URAned1    = 1;           // No value provided in legacy
   int n_URAned1            = 3;
   int s_URAned1            = 1;

   unsigned long URAned2    = 2;           // No value provided in legacy
   int n_URAned2            = 3;
   int s_URAned2            = 1;
   
   double af0              = oe.af0;
   int n_af0               = 26;
   int s_af0               = -35;

   double af1              = oe.af1;
   int n_af1               = 20;
   int s_af1               = -48;

   double af2              = oe.af2;
   int n_af2               = 10;
   int s_af2               = -60;

   double Tgd              = oe.Tgd;
   int n_Tgd               = 13;
   int s_Tgd               = -35;

   double ISCL1cp          = 1E-8;         // No value provided in legacy
   int n_ISCL1cp           = 13;
   int s_ISCL1cp           = -35;

   double ISCL1cd          = -1E-8;        // No value provided in legacy
   int n_ISCL1cd           = 13;
   int s_ISCL1cd           = -35;

   unsigned long sflag     = 0;            // Assume integrity status flag OFF
   int n_sflag             = 1;
   int s_sflag             = 1;

   unsigned long reservedBits = 0;
   int n_reservedBits         = 10;
   int s_reservedBits         = 1;

   unsigned long CRC       = 0;
   int n_CRC               = 24;
   int s_CRC               = 1; 


   //cout << "TOWWeek, ITOW, Top, AODO = " << TOWWeek << ", " << ITOW << ", " 
   //     << Top << ", " << oe.AODO << endl;

      // Pack the individual items into the structure
   pnb.addUnsignedLong(TOWWeek, n_TOWWeek, s_TOWWeek);
   pnb.addUnsignedLong(ITOW, n_ITOW, s_ITOW);
   pnb.addUnsignedLong(Top, n_Top, s_Top);
   pnb.addUnsignedLong(L1CHealth, n_L1CHealth, s_L1CHealth);
   pnb.addLong(URAed, n_URAed, s_URAed);
   pnb.addUnsignedLong(Toe, n_Toe, s_Toe);
   pnb.addSignedDouble(deltaA, n_deltaA, s_deltaA);
   pnb.addSignedDouble(Adot, n_Adot, s_Adot);
   pnb.addDoubleSemiCircles(dn, n_dn, s_dn);
   pnb.addDoubleSemiCircles(dndot, n_dndot, s_dndot);
   pnb.addDoubleSemiCircles(M0, n_M0, s_M0);
   pnb.addUnsignedDouble(ecc, n_ecc, s_ecc);
   pnb.addDoubleSemiCircles(w, n_w, s_w);
   pnb.addDoubleSemiCircles(OMEGA0, n_OMEGA0, s_OMEGA0);
   pnb.addDoubleSemiCircles(i0, n_i0, s_i0);
   pnb.addDoubleSemiCircles(deltaOMEGAdot, n_deltaOMEGAdot, s_deltaOMEGAdot);
   pnb.addDoubleSemiCircles(idot, n_idot, s_idot); 
   pnb.addSignedDouble(Cis, n_Cis, s_Cis);
   pnb.addSignedDouble(Cic, n_Cic, s_Cic);
   pnb.addSignedDouble(Crs, n_Crs, s_Crs);
   pnb.addSignedDouble(Crc, n_Crc, s_Crc);
   pnb.addSignedDouble(Cus, n_Cus, s_Cus);
   pnb.addSignedDouble(Cuc, n_Cuc, s_Cuc);
   pnb.addLong(URAned0, n_URAned0, s_URAned0);
   pnb.addUnsignedLong(URAned1, n_URAned1, s_URAned1);
   pnb.addUnsignedLong(URAned2, n_URAned2, s_URAned2);
   pnb.addSignedDouble(af0, n_af0, s_af0);
   pnb.addSignedDouble(af1, n_af1, s_af1);
   pnb.addSignedDouble(af2, n_af2, s_af2);
   pnb.addSignedDouble(Tgd, n_Tgd, s_Tgd);
   pnb.addSignedDouble(ISCL1cp, n_ISCL1cp, s_ISCL1cp);
   pnb.addSignedDouble(ISCL1cd, n_ISCL1cd, s_ISCL1cd);
   pnb.addUnsignedLong(sflag, n_sflag, s_sflag);
   pnb.addUnsignedLong(reservedBits, n_reservedBits, s_reservedBits);
   pnb.addUnsignedLong(CRC, n_CRC, s_CRC);

      // Resize the vector holding the packed nav message data
   pnb.trimsize();

      // Output the results 
   pnb.outputPackedBits(out);
   out << endl;

   return;
}



