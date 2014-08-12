#ifndef GAUSHITFINDERANA_H
#define GAUSHITFINDERANA_H
////////////////////////////////////////////////////////////////////////
//
// Gaus(s)HitFinder class designed to analyze signal on a wire in the TPC
//
// jaasaadi@syr.edu
//
// Note: This is a rework of the original hit finder ana module
// 	 The only histograms that are saved are ones that can be used
//	 to make sure the hit finder is functioning...the rest is 
// 	 outputted to a TTree for offline analysis.
////////////////////////////////////////////////////////////////////////


// LArSoft includes
#include "Geometry/Geometry.h"
#include "Geometry/PlaneGeo.h"
#include "MCCheater/BackTracker.h"
#include "RecoBase/Hit.h"
#include "Utilities/LArProperties.h"
#include "Utilities/DetectorProperties.h"

// ROOT includes
#include <TMath.h>
#include <TH1F.h>
#include <TGraph.h>
#include <TFile.h>
#include "TComplex.h"
#include "TString.h"
#include "TH1.h"
#include "TH2.h"
#include "TProfile.h"
#include "TTree.h"

// C++ includes
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iostream>
#include <bitset>
#include <vector>
#include <string>

// Framework includes
#include "art/Framework/Principal/Event.h"
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Persistency/Common/Ptr.h" 
#include "art/Persistency/Common/PtrVector.h" 
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Framework/Services/Optional/TFileDirectory.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "art/Framework/Core/ModuleMacros.h" 
#include "art/Framework/Core/EDAnalyzer.h"

constexpr int kMaxHits       = 20000; //maximum number of hits;

namespace geo { class Geometry;   }
namespace sim { class SimChannel; }

namespace hit{

  /// Base class for creation of raw signals on wires. 
  class GausHitFinderAna : public art::EDAnalyzer {
    
  public:
        
    explicit GausHitFinderAna(fhicl::ParameterSet const& pset); 
    virtual ~GausHitFinderAna();
    
    /// read/write access to event
    void analyze (const art::Event& evt);
    void beginJob();
    void reconfigure(fhicl::ParameterSet const& p);

  private:

    std::string            fHitFinderModuleLabel; ///
    std::string            fLArG4ModuleLabel;
    std::string            fCalDataModuleLabel;
      


      
    TH1F* fHitResidualAll;
    TH1F* fHitResidualAllAlt;
    TH1F* fNumberOfHitsPerEvent;
    TH2F* fPeakTimeVsWire;
    
    

    // ### TTree for offline analysis ###
    TTree* fHTree;
    // === Event Information ===
    Int_t fRun;			// Run Number
    Int_t fEvt;			// Event Number
    
    
    // === Wire Information ====
    Float_t fWireTotalCharge;	// Charge on all wires
    
    
    // === Hit Information ===
    Int_t fnHits; 			// Number of Hits in the Event
    Int_t fWire[kMaxHits];		// Wire Number
    Float_t fStartTime[kMaxHits];	// Start Time
    Float_t fStartTimeUncert[kMaxHits];	// Start Time Uncertainty
    Float_t fEndTime[kMaxHits];		// End Time
    Float_t fEndTimeUncert[kMaxHits];	// End Time Uncertainty
    Float_t fPeakTime[kMaxHits];	// Peak Time
    Float_t fPeakTimeUncert[kMaxHits];	// Peak Time Uncertainty
    Float_t fCharge[kMaxHits];		// Charge of this hit
    Float_t fChargeUncert[kMaxHits];	// Charge Uncertainty of this hit
    Int_t fMultiplicity[kMaxHits];	// Hit pulse multiplicity
    Float_t fGOF[kMaxHits];		// Goodness of Fit (Chi2/NDF)
    
    // === Total Hit Information ===
    Float_t fTotalHitChargePerEvent;	//Total charge recorded in each event
    
    // === Truth Hit Info from BackTracker ===
    Float_t fTruePeakPos[kMaxHits];	// Truth Time Tick info from BackTracker

    
      
      

      
  }; // class GausHitFinderAna


  //-------------------------------------------------
  GausHitFinderAna::GausHitFinderAna(fhicl::ParameterSet const& pset) 
    : EDAnalyzer(pset)
  {
    this->reconfigure(pset);
  }

  //-------------------------------------------------
  GausHitFinderAna::~GausHitFinderAna()
  {
  }

  void GausHitFinderAna::reconfigure(fhicl::ParameterSet const& p)
  {
    fHitFinderModuleLabel 	= p.get< std::string >("HitsModuleLabel");
    fLArG4ModuleLabel        	= p.get< std::string >("LArGeantModuleLabel");
    fCalDataModuleLabel	 	= p.get< std::string  >("CalDataModuleLabel");
    return;
  }
  //-------------------------------------------------
  void GausHitFinderAna::beginJob() 
  {
    // get access to the TFile service
    art::ServiceHandle<art::TFileService> tfs;
   
    
    // ======================================
    // === Hit Information for Histograms ===
    fHitResidualAll	= tfs->make<TH1F>("fHitResidualAll", "Hit Residual All", 1600, -400, 400);
    fHitResidualAllAlt  = tfs->make<TH1F>("fHitResidualAllAlt", "Hit Residual All", 1600, -400, 400);
    fNumberOfHitsPerEvent= tfs->make<TH1F>("fNumberOfHitsPerEvent", "Number of Hits in Each Event", 10000, 0, 10000);
    fPeakTimeVsWire     = tfs->make<TH2F>("fPeakTimeVsWire", "Peak Time vs Wire Number", 3200, 0, 3200, 9500, 0, 9500);

    
    
    
    // ##############
    // ### TTree ####
    fHTree = tfs->make<TTree>("HTree","HTree");

    // === Event Info ====
    fHTree->Branch("Evt", &fEvt, "Evt/I");
    fHTree->Branch("Run", &fRun, "Run/I");
    
    // === Wire Info ===
    fHTree->Branch("WireTotalCharge", &fWireTotalCharge, "WireTotalCharge/F");
    
    // === Hit Info ===
    fHTree->Branch("nHits", &fnHits, "nHits/I");
    fHTree->Branch("Wire", &fWire, "Wire[nHits]/I");
    fHTree->Branch("StartTime", &fStartTime, "fStartTime[nHits]/F");
    fHTree->Branch("StartTimeUncert", &fStartTimeUncert, "fStartTimeUncert[nHits]/F");
    fHTree->Branch("EndTime", &fEndTime, "fEndTime[nHits]/F");
    fHTree->Branch("EndTimeUncert", &fEndTimeUncert, "fEndTimeUncert[nHits]/F");
    fHTree->Branch("PeakTime", &fPeakTime, "fPeakTime[nHits]/F");
    fHTree->Branch("PeakTimeUncert", &fPeakTimeUncert, "fPeakTimeUncert[nHits]/F");
    fHTree->Branch("Charge", &fCharge, "fCharge[nHits]/F");
    fHTree->Branch("ChargeUncert", &fChargeUncert, "fChargeUncert[nHits]/F");
    fHTree->Branch("Multiplicity", &fMultiplicity, "fMultiplicity[nHits]/I");
    fHTree->Branch("GOF", &fGOF, "fGOF[nHits]/F");
    
    // === Total Hit Information ===
    fHTree->Branch("TotalHitChargePerEvent", &fTotalHitChargePerEvent, "TotalHitChargePerEvent/F");
    
    // === Truth Hit Information from BackTracker ===
    fHTree->Branch("TruePeakPos", &fTruePeakPos, "fTruePeakPos[nHits]/F");

    return;

  }

  //-------------------------------------------------
  void GausHitFinderAna::analyze(const art::Event& evt)
  {

    // ##############################################
    // ### Outputting Run Number and Event Number ###
    // ##############################################
    //std::cout << "run    : " << evt.run() <<" event  : "<<evt.id().event() << std::endl;
    
    // ### TTree Run/Event ###
    fEvt = evt.id().event();
    fRun = evt.run();
    
    // ####################################
    // ### Getting Geometry Information ###
    // ####################################
    art::ServiceHandle<geo::Geometry> geom;
  
    // #######################################
    // ### Getting Liquid Argon Properites ###
    // #######################################
    art::ServiceHandle<util::LArProperties> larp;
  
    // ###################################
    // ### Getting Detector Properties ###
    // ###################################
    art::ServiceHandle<util::DetectorProperties> detp;
    
    // ##########################################
    // ### Reading in the Wire List object(s) ###
    // ##########################################
    art::Handle< std::vector<recob::Wire> > wireVecHandle;
    evt.getByLabel(fCalDataModuleLabel,wireVecHandle);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////        CHARGE DIRECTLY FROM WIRE INFO   /////////////////////////////////////////////////
    
    float TotWireCharge = 0;
    
    //##############################
    //### Looping over the wires ###
    //############################## 
    for(size_t wireIter = 0; wireIter < wireVecHandle->size(); wireIter++)
    	{
	art::Ptr<recob::Wire> wire(wireVecHandle, wireIter);
      	// #################################################
      	// ### Getting a vector of signals for this wire ###
      	// #################################################
      	std::vector<float> signal(wire->Signal());
      
      	// ##########################################################
      	// ### Making an iterator for the time ticks of this wire ###
      	// ##########################################################
      	std::vector<float>::iterator timeIter;  	    // iterator for time bins
	
	
      
      	// ##################################
      	// ### Looping over Signal Vector ###
      	// ##################################
      	for(timeIter = signal.begin(); timeIter+2<signal.end(); timeIter++)
      	   {
	 
	   // ###########################################################
	   // ### If the ADC value is less than 2 skip this time tick ###
	   // ###########################################################
	   if(*timeIter < 2) {continue;}
	   
	   // ### Filling Total Wire Charge ###
	   TotWireCharge += *timeIter;
	   
	 
	   }//<---End timeIter loop
      
      
	}//<---End wireIter loop
    
    // ~~~ Filling the total charge in the event ~~~
    //std::cout<<"Total Real Charge = "<<TotWireCharge<<std::endl;
    fWireTotalCharge = TotWireCharge;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////        RECONSTRUCTED HIT INFORMATION   /////////////////////////////////////////////////  
  
    // ##################################################
    // ### Getting the Reconstructed Hits (hitHandle) ###
    // ##################################################
    art::Handle< std::vector<recob::Hit> > hitHandle;
    evt.getByLabel(fHitFinderModuleLabel,hitHandle);
  
    // #########################################
    // ### Putting Hits into a vector (hits) ###
    // #########################################
    std::vector< art::Ptr<recob::Hit> > hits;
    art::fill_ptr_vector(hits, hitHandle);
    
    float TotCharge = 0;
    int hitCount = 0;
    // ### Number of Hits in the event ###
    fnHits = hitHandle->size();
    fNumberOfHitsPerEvent->Fill(hitHandle->size());
    // #########################
    // ### Looping over Hits ###
    // #########################
    for(size_t numHit = 0; numHit < hitHandle->size(); ++numHit)
       {
       // === Finding Channel associated with the hit ===
       art::Ptr<recob::Hit> hit(hitHandle, numHit);
       
       fWire[hitCount] 		= hit->WireID().Wire;
       fStartTime[hitCount]       = hit->StartTime();
       fStartTimeUncert[hitCount] = hit->SigmaStartTime();
       fEndTime[hitCount]         = hit->EndTime();
       fEndTimeUncert[hitCount]   = hit->SigmaEndTime();
       fPeakTime[hitCount]	= hit->PeakTime();
       fPeakTimeUncert[hitCount]= hit->SigmaPeakTime();
       fCharge[hitCount]	= hit->Charge();
       fChargeUncert[hitCount]	= hit->SigmaCharge();
       fMultiplicity[hitCount]	= hit->Multiplicity();
       fGOF[hitCount]		= hit->GoodnessOfFit();
       //std::cout<<"Hit Charge = "<<hit->Charge()<<std::endl;
       //std::cout<<"StartTime = "<<hit->StartTime()<<std::endl;
       
       hitCount++;
       TotCharge += hit->Charge();
       
       fPeakTimeVsWire->Fill(hit->WireID().Wire, hit->PeakTime());
       }//<---End numHit
    //std::cout<<"Total Reco Charge = "<<TotCharge<<std::endl;
    fTotalHitChargePerEvent = TotCharge;
    
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////        TRUTH HIT INFO FROM BACKTRACKER   ///////////////////////////////////////////////// 
    
    // ###############################################################
    // ### Integers used for setting Channel, TPC, Plane, and Wire ###
    // ###############################################################
    unsigned int plane = 0;
    
    // ############################################
    // ### Variables used for Truth Calculation ###
    // ############################################
    Float_t TruthHitTime = 0 , TruthHitCalculated = 0;
    int count = 0;

    // ================================================
    // === Calculating Time Tick and Drift Velocity ===
    // ================================================
    double time_tick      = detp->SamplingRate()/1000.;
    double drift_velocity = larp->DriftVelocity(larp->Efield(),larp->Temperature());
    
    for(size_t nh = 0; nh < hitHandle->size(); nh++)
       {
       // === Finding Channel associated with the hit ===
       art::Ptr<recob::Hit> hitPoint(hitHandle, nh);
       plane = hitPoint->WireID().Plane;
       //wire = hitPoint->WireID().Wire;    	
      
      
       // ===================================================================
       // Using Track IDE's to locate the XYZ location from truth information
       // ===================================================================
       std::vector<sim::TrackIDE> trackides;
       std::vector<double> xyz;
       try
          {
          // ####################################
          // ### Using BackTracker HitCheater ###
          // ####################################
          art::ServiceHandle<cheat::BackTracker> bt;

          trackides = bt->HitToTrackID(hitPoint);
          xyz = bt->HitToXYZ(hitPoint);
          }
       catch(cet::exception e)
          {mf::LogWarning("GausHitFinderAna") << "BackTracker Failed";
           continue;}

      
       // ==============================================================
       // Calculating the truth tick position of the hit using 2 methods
       // Method 1: ConvertXtoTicks from the detector properties package
       // Method 2: Actually do the calculation myself to double check things
       // ==============================================================
      
       // ### Method 1 ###
       TruthHitTime = detp->ConvertXToTicks(xyz[0], plane, hitPoint->WireID().TPC, hitPoint->WireID().Cryostat);
      
       // ### Method 2 ###
       // ================================================
       // Establishing the x-position of the current plane
       // ================================================ 
       const double origin[3] = {0.};
       double pos[3];
       geom->Plane(plane).LocalToWorld(origin, pos);
       double planePos_timeCorr = (pos[0]/drift_velocity)*(1./time_tick)+60; 
       //<---x position of plane / drift velocity + 60 (Trigger offset)
      
       TruthHitCalculated = ( (xyz[0]) / (drift_velocity * time_tick) ) + planePos_timeCorr;
      
       fTruePeakPos[count] = TruthHitTime;
       count++;
       double hitresid = ( ( TruthHitTime - hitPoint->PeakTime() ) / hitPoint->SigmaPeakTime() );
       fHitResidualAll->Fill( hitresid );
       
       double hitresidAlt = ( ( TruthHitCalculated - hitPoint->PeakTime() ) / hitPoint->SigmaPeakTime() );
       fHitResidualAllAlt->Fill(hitresidAlt);
       
       }//<---End nh loop
    
    fHTree->Fill();
    return;
    
  }//end analyze method
   
  // --------------------------------------------------------
  DEFINE_ART_MODULE(GausHitFinderAna)

} // end of hit namespace



#endif // GAUSHITFINDERANA_H
