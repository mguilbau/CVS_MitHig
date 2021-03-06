#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "SimDataFormats/TrackingAnalysis/interface/TrackingParticle.h"
#include "SimDataFormats/TrackingAnalysis/interface/TrackingParticleFwd.h"

#include "DataFormats/TrackReco/interface/TrackFwd.h"
#include "DataFormats/TrackReco/interface/Track.h"

#include "MitHig/MatchedTrack/interface/MatchedTrack.h"

#include "SimTracker/Records/interface/TrackAssociatorRecord.h"
#include "SimTracker/TrackAssociation/interface/TrackAssociatorByHits.h"

#include "FWCore/Utilities/interface/Exception.h"

#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"

#include "Geometry/CommonDetUnit/interface/GeomDetType.h"
#include "Geometry/CommonDetUnit/interface/GeomDetUnit.h"

#include "DataFormats/SiStripDetId/interface/StripSubdetector.h"
#include "DataFormats/SiPixelDetId/interface/PixelSubdetector.h"
#include "DataFormats/SiStripDetId/interface/TECDetId.h"
#include "DataFormats/SiStripDetId/interface/TIBDetId.h"
#include "DataFormats/SiStripDetId/interface/TIDDetId.h"
#include "DataFormats/SiStripDetId/interface/TOBDetId.h"
#include "DataFormats/SiPixelDetId/interface/PXBDetId.h"
#include "DataFormats/SiPixelDetId/interface/PXFDetId.h"

#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"

#include "TrackingTools/TransientTrack/interface/TransientTrackBuilder.h"
#include "TrackingTools/TransientTrack/interface/TransientTrack.h"
#include "TrackingTools/Records/interface/TransientTrackRecord.h"

/*
 class TransientTrackFromFTSFactory {
 public:
 
 reco::TransientTrack build (const FreeTrajectoryState & fts) const;
 reco::TransientTrack build (const FreeTrajectoryState & fts,
 const edm::ESHandle<GlobalTrackingGeometry>& trackingGeometry);
 };
 */

#include "RecoVertex/KalmanVertexFit/interface/SingleTrackVertexConstraint.h"

#include "TROOT.h"
#include "TFile.h"
#include "TNtuple.h"
#include "TClonesArray.h"

#include <fstream>
using namespace std;
using namespace reco;
using namespace edm;

/*****************************************************************************/
class HighPtTrackAnalyzer : public edm::EDAnalyzer
{
public:
	explicit HighPtTrackAnalyzer(const edm::ParameterSet& pset);
	~HighPtTrackAnalyzer();
	virtual void beginJob(const edm::EventSetup& es);
	virtual void analyze(const edm::Event& ev, const edm::EventSetup& es);
	virtual void endJob();
	
private:
	int layerFromDetid(const DetId& detId);
	int getNumberOfSimHits(const TrackingParticle& simTrack);
	int getDetLayerId(const PSimHit& simHit);
	float getSimHitRadialPosition(const PSimHit& simHit);
	int getNumberOfPixelHits(const TrackingParticle& simTrack,float *);
	int getNumberOfRecPixelHits(const reco::Track & recTrack, float *);
	void getRecHitLayerPatterns(const reco::Track & recTrack, vector<int> &,vector<int> &,vector<int> &,vector<int> &,vector<int> &,vector<int> &, float &, float &);	
	void getSimHitLayerPatterns(const TrackingParticle & simTrack, vector<int> &,vector<int> &,vector<int> &,vector<int> &,vector<int> &,vector<int> &, float &, float &);	
	void checkSimTracks (edm::Handle<TrackingParticleCollection>& simCollection,reco::SimToRecoCollection& q);

	pair<float,float> refitWithVertex(const reco::Track & recTrack,const reco::VertexCollection* vertices);
	int getParticleId(edm::RefToBase<reco::Track>& recTrack, int & ptype);
	void checkRecTracks(edm::Handle<edm::View<reco::Track> >& recCollection,const reco::VertexCollection* vertices,reco::RecoToSimCollection& p);
	
	const TrackerGeometry * theTracker;
	const TrackAssociatorByHits * theAssociatorByHits;
	const TransientTrackBuilder * theTTBuilder;
	TrackerHitAssociator * theHitAssociator;
	
	vector<string> trackCollectionLabels;
	string resultFileLabel;
	bool useAbsoluteNumberOfHits, keepLowPtSimTracks, infoHiEventTopology;
	int proc;
	Int_t iTrkSim,iTrkReco,iVtx;
	Int_t iEvent,iRun;
	Int_t Npart, Ncoll, Nhard;
	Float_t Phi0;
	Float_t ImpactPara; 
	Float_t RecVtx, SimVtx;
	Float_t fSimPxlLayerHit,dMinSimPt,fRecPxlLayerHit;
	vector<int> vPXBHits, vPXFHits, vTIBHits, vTOBHits, vTIDHits, vTECHits;
	vector<int> vPXBSimHits, vPXFSimHits, vTIBSimHits, vTOBSimHits, vTIDSimHits, vTECSimHits;
	float fSimHitLayers, fRecHitLayers, fPxlSimHitLayers, fPxlRecHitLayers;
	
	TFile * resultFile; 
	TTree *recInfoTree;
	TClonesArray *CAReco,*CASim;
	
};

/*****************************************************************************/
HighPtTrackAnalyzer::HighPtTrackAnalyzer(const edm::ParameterSet& pset)
{
	trackCollectionLabels = pset.getParameter<vector<string> >("trackCollection");
	resultFileLabel       = pset.getParameter<string>("resultFile");
	useAbsoluteNumberOfHits = pset.getUntrackedParameter<bool>("useAbsoluteNumberOfHits",false);
	keepLowPtSimTracks = pset.getUntrackedParameter<bool>("keepLowPtSimTracks",false);
	infoHiEventTopology = pset.getUntrackedParameter<bool>("infoHiEventTopology",false); }

/*****************************************************************************/
HighPtTrackAnalyzer::~HighPtTrackAnalyzer()
{
}

/*****************************************************************************/
void HighPtTrackAnalyzer::beginJob(const edm::EventSetup& es)
{
	// Get tracker geometry
	edm::ESHandle<TrackerGeometry> tracker;
	es.get<TrackerDigiGeometryRecord>().get(tracker);
	theTracker = tracker.product();
	
	// Get associator
	edm::ESHandle<TrackAssociatorBase> theHitsAssociator;
	es.get<TrackAssociatorRecord>().get("TrackAssociatorByHits",
										theHitsAssociator);
	theAssociatorByHits =
	(const TrackAssociatorByHits*)theHitsAssociator.product();
	
	// Get transient track builder
	edm::ESHandle<TransientTrackBuilder> builder;
	es.get<TransientTrackRecord>().get("TransientTrackBuilder", builder);
	theTTBuilder = builder.product();
	
	// Root
	resultFile = new TFile(resultFileLabel.c_str(),"recreate");
	resultFile->cd();
	
	dMinSimPt=1.0;
	
	CAReco=new TClonesArray("MatchedTrack",10000);
	CASim=new TClonesArray("MatchedTrack",10000);
	
	recInfoTree=new TTree("RecoStudyTree","RecoStudyTree");
	recInfoTree->Branch("RunNo",&iRun,"RunNumber/I");
	recInfoTree->Branch("EventNo",&iEvent,"EventNumber/I");
	recInfoTree->Branch("RecTracks",&CAReco);
	recInfoTree->Branch("SimTracks",&CASim);
	recInfoTree->Branch("TotalRecoTracks",&iTrkReco,"TotalRecTracks/I");
	recInfoTree->Branch("TotalSimTracks",&iTrkSim,"TotalSimTracks/I");
	recInfoTree->Branch("TotalVtx",&iVtx,"TotalVtx/I");
	recInfoTree->Branch("RecVertex",&RecVtx,"RecVertex/F");
	recInfoTree->Branch("SimVertex",&SimVtx,"SimVertex/F");
	if(infoHiEventTopology){
	   recInfoTree->Branch("ImpactParameter",&ImpactPara,"ImpactParameter/F");
	   recInfoTree->Branch("Npart",&Npart,"Npart/I");
	   recInfoTree->Branch("Ncoll",&Ncoll,"Ncoll/I");
	   recInfoTree->Branch("Nhard",&Nhard,"Nhard/I");
	   recInfoTree->Branch("ReactionPlaneAngle",&Phi0,"ReactionPlaneAngle/F");
	}
}

/*****************************************************************************/
void HighPtTrackAnalyzer::endJob()
{
	resultFile->cd();
	recInfoTree->Write();
	resultFile->Close();
}

/*****************************************************************************/
int HighPtTrackAnalyzer::layerFromDetid(const DetId& detId)
{
	int layerNumber=0;
	unsigned int subdetId = static_cast<unsigned int>(detId.subdetId());
	if ( subdetId == StripSubdetector::TIB)
    {
		TIBDetId tibid(detId.rawId());
		layerNumber = tibid.layer();
    }
	else if ( subdetId ==  StripSubdetector::TOB )
    {
		TOBDetId tobid(detId.rawId());
		layerNumber = tobid.layer();
    }
	else if ( subdetId ==  StripSubdetector::TID)
    {
		TIDDetId tidid(detId.rawId());
		layerNumber = tidid.wheel();
    }
	else if ( subdetId ==  StripSubdetector::TEC )
    {
		TECDetId tecid(detId.rawId());
		layerNumber = tecid.wheel();
    }
	else if ( subdetId ==  PixelSubdetector::PixelBarrel )
    {
		PXBDetId pxbid(detId.rawId());
		layerNumber = pxbid.layer();
    }
	else if ( subdetId ==  PixelSubdetector::PixelEndcap )
    {
		PXFDetId pxfid(detId.rawId());
		layerNumber = pxfid.disk();
    }
	else
		edm::LogVerbatim("TrackValidator") << "Unknown subdetid: " << subdetId;
	
	return layerNumber;
}

/*****************************************************************************/
int HighPtTrackAnalyzer::getNumberOfSimHits(const TrackingParticle& simTrack)
{
	int oldlay = 0; int newlay = 0;
	int olddet = 0; int newdet = 0;
	
	int nhit = 0;
	
	for(std::vector<PSimHit>::const_iterator
		simHit = simTrack.pSimHit_begin();
		simHit!= simTrack.pSimHit_end(); simHit++)
	{
		const DetId detId = DetId(simHit->detUnitId());
		oldlay = newlay; newlay = layerFromDetid(detId);
		olddet = newdet; newdet = detId.subdetId();
		if(oldlay != newlay || (oldlay == newlay && olddet != newdet) ) nhit++;
	}
	
	return nhit;
}

/*****************************************************************************/
int HighPtTrackAnalyzer::getDetLayerId(const PSimHit& simHit)
{
	int layerId;
	
	DetId id = DetId(simHit.detUnitId());
	LocalPoint lpos = simHit.localPosition();
	GlobalPoint gpos = theTracker->idToDetUnit(id)->toGlobal(lpos);
	
	if(theTracker->idToDetUnit(id)->subDetector() ==
	   GeomDetEnumerators::PixelBarrel)
	{ // barrel
		if(gpos.perp2() < 6 * 6) layerId = 0;
		else
		{
			if(gpos.perp2() < 9 * 9) layerId = 1;
			else layerId = 2;
		}
	}
	else
	{ // endcap
		if(fabsf(gpos.z()) < 40) layerId = 3;
		else layerId = 4;
	}
	
	return layerId;
}

/*****************************************************************************/
float HighPtTrackAnalyzer::getSimHitRadialPosition(const PSimHit& simHit)
{
	
	unsigned int id = simHit.detUnitId();

	if (id > 490000000)  return -999.9; //exclude simHits from outside the tracker
	
	DetId detId = DetId(id);
	LocalPoint lpos = simHit.localPosition();
	GlobalPoint gpos = theTracker->idToDetUnit(detId)->toGlobal(lpos);
	
	return gpos.perp();
	
}

/*****************************************************************************/
int HighPtTrackAnalyzer::getNumberOfPixelHits(const TrackingParticle& simTrack,float *fSimPxlLayerHit)
{
	// How many pixel hits?
	const int nLayers = 5;
	vector<bool> filled(nLayers,false);
	
	int numberOfPixelHits = 0;
	
	for(std::vector<PSimHit>::const_iterator simHit = simTrack.pSimHit_begin();simHit!= simTrack.pSimHit_end();simHit++){
		//DetId id = DetId(simHit->detUnitId());
		unsigned int id = simHit->detUnitId();
		
		if (id > 490000000){  //exclude simHits from outside the tracker
			//LogWarning("TrackAnalyzer") << "-----------------id " << id;
		} else {
			
			DetId detId(id);
			if(detId.subdetId() ==(int)PixelSubdetector::PixelBarrel||detId.subdetId() ==(int)PixelSubdetector::PixelEndcap){
				filled[getDetLayerId(*simHit)] = true;
				numberOfPixelHits++;
			}
		}
	}
	
	// Count the number of filled pixel layers
	int fLayers = 0;
	for(int i=0; i<nLayers; i++)
		if(filled[i] == true) fLayers++;
	
	*fSimPxlLayerHit=(float)fLayers;
	
	return numberOfPixelHits;
}



int HighPtTrackAnalyzer::getNumberOfRecPixelHits(const reco::Track& recTrack,float *fRecPxlLayerHit){
	
	const int nLayers = 5;
	vector<bool> filled(nLayers,false);
	
	int numberOfPixelHits = 0;
	
	for(trackingRecHit_iterator recHit = recTrack.recHitsBegin();recHit!= recTrack.recHitsEnd(); recHit++){
		if((*recHit)->isValid()){
			
			DetId id = (*recHit)->geographicalId();
			if(!theTracker->idToDet(id))
				continue;
			
			if(theTracker->idToDet(id)->subDetector() ==GeomDetEnumerators::PixelBarrel ||theTracker->idToDet(id)->subDetector() ==GeomDetEnumerators::PixelEndcap){
				int layerId;
				
				GlobalPoint gpos = theTracker->idToDet((*recHit)->geographicalId())->toGlobal((*recHit)->localPosition());
				
				if(theTracker->idToDet(id)->subDetector()==GeomDetEnumerators::PixelBarrel)
				{ // barrel
					if(gpos.perp2() < 6 * 6) layerId = 0;
					else
					{
						if(gpos.perp2() < 9 * 9) layerId = 1;
						else layerId = 2;
					}
				}
				else
				{ // endcap
					if(fabsf(gpos.z()) < 40) layerId = 3;
					else layerId = 4;
				}
				filled[layerId] = true;
				numberOfPixelHits++;
			}
		}
		
	}
	int fLayers = 0;
	for(int i=0; i<nLayers; i++)
		if(filled[i] == true) fLayers++;
	
	*fRecPxlLayerHit=(float)fLayers;
	
	return numberOfPixelHits;
}

/******************************************************************************************/

void HighPtTrackAnalyzer::getRecHitLayerPatterns(const reco::Track & recTrack, vector<int> &vPXBHits,vector<int> &vPXFHits,vector<int> &vTIBHits,vector<int> &vTOBHits,vector<int> &vTIDHits,vector<int> &vTECHits, float &fRecHitLayers, float &fPxlRecHitLayers) {	
	
	// re-initialize vectors
	vPXBHits.assign(3,0);
	vPXFHits.assign(3,0);
	vTIBHits.assign(4,0);
	vTOBHits.assign(6,0);
	vTIDHits.assign(3,0);
	vTECHits.assign(9,0);
	fRecHitLayers=0.0;
	fPxlRecHitLayers=0.0;
	
	// loop over recHits on track
	for(trackingRecHit_iterator recHit = recTrack.recHitsBegin();recHit!= recTrack.recHitsEnd(); recHit++){
		if((*recHit)->isValid()){
			
			DetId detId = (*recHit)->geographicalId();
			if(!theTracker->idToDet(detId))
				continue;
			
			Int_t layerNumber=0;
			unsigned int subdetId = static_cast<unsigned int>(detId.subdetId());
			
			if ( subdetId ==  PixelSubdetector::PixelBarrel )
			{
				PXBDetId pxbid(detId.rawId());
				layerNumber = pxbid.layer();
				vPXBHits[layerNumber-1]++;
				LogTrace("TrackAnalyzer") << "PXB layer " << layerNumber << ": " << vPXBHits[layerNumber-1] << " hit(s)";
			}
			else if ( subdetId ==  PixelSubdetector::PixelEndcap )
			{
				PXFDetId pxfid(detId.rawId());
				layerNumber = pxfid.disk();
				vPXFHits[layerNumber-1]++;
				LogTrace("TrackAnalyzer") << "PXF layer " << layerNumber << ": " << vPXFHits[layerNumber-1] << " hit(s)";
			}
			else if ( subdetId == StripSubdetector::TIB)
			{
				TIBDetId tibid(detId.rawId());
				layerNumber = tibid.layer();
				vTIBHits[layerNumber-1]++;
				LogTrace("TrackAnalyzer") << "TIB layer " << layerNumber << ": " << vTIBHits[layerNumber-1] << " hit(s)";
			}
			else if ( subdetId ==  StripSubdetector::TOB )
			{
				TOBDetId tobid(detId.rawId());
				layerNumber = tobid.layer();
				vTOBHits[layerNumber-1]++;
				LogTrace("TrackAnalyzer") << "TOB layer " << layerNumber << ": " << vTOBHits[layerNumber-1] << " hit(s)";
			}
			else if ( subdetId ==  StripSubdetector::TID)
			{
				TIDDetId tidid(detId.rawId());
				layerNumber = tidid.wheel();
				vTIDHits[layerNumber-1]++;
				LogTrace("TrackAnalyzer") << "TID layer " << layerNumber << ": " << vTIDHits[layerNumber-1] << " hit(s)";
			}
			else if ( subdetId ==  StripSubdetector::TEC )
			{
				TECDetId tecid(detId.rawId());
				layerNumber = tecid.wheel();
				vTECHits[layerNumber-1]++;
				LogTrace("TrackAnalyzer") << "TEC layer " << layerNumber << ": " << vTECHits[layerNumber-1] << " hit(s)";
			}
			
		}// end isValid
	}//end loop over recHits
	
	
	for(Int_t i=0; i<9; i++) {
		if(i<3) fPxlRecHitLayers += (vPXBHits[i]>0) + (vPXFHits[i]>0);
		if(i<3) fRecHitLayers += (vPXBHits[i]>0) + (vPXFHits[i]>0) + (vTIDHits[i]>0);
		if(i<4) fRecHitLayers += (vTIBHits[i]>0);
		if(i<6) fRecHitLayers += (vTOBHits[i]>0);
		fRecHitLayers += (vTECHits[i]>0);
	}
	
	LogTrace("TrackAnalyzer") << "Total # of layers hit by RecTrack: " << fRecHitLayers;
	LogTrace("TrackAnalyzer") << "Total # of pixel layers hit by RecTrack: " << fPxlRecHitLayers;
	
	LogTrace("TrackAnalyzer") << "-------------------------------------------------\n";
	
}

void HighPtTrackAnalyzer::getSimHitLayerPatterns(const TrackingParticle & simTrack, vector<int> &vPXBSimHits,vector<int> &vPXFSimHits,vector<int> &vTIBSimHits,vector<int> &vTOBSimHits,vector<int> &vTIDSimHits,vector<int> &vTECSimHits,float &fSimHitLayers, float &fPxlSimHitLayers) {	

	// re-initialize vectors
	vPXBSimHits.assign(3,0);
	vPXFSimHits.assign(3,0);
	vTIBSimHits.assign(4,0);
	vTOBSimHits.assign(6,0);
	vTIDSimHits.assign(3,0);
	vTECSimHits.assign(9,0);
	fSimHitLayers=0.0;
	fPxlSimHitLayers=0.0;
	
	Float_t lastr=0.0; //radial position of last hit
	Float_t newr=0.0; //radial position of new hit
	
	for(std::vector<PSimHit>::const_iterator simHit = simTrack.pSimHit_begin();simHit!= simTrack.pSimHit_end();simHit++){

		newr = getSimHitRadialPosition(*simHit);
		if(newr < lastr) {
			LogTrace("TrackAnalyzer") << "TRUNCATING SIMTRACK at r=" << newr << "\tLast hit was at r=" << lastr;
			break;
		} else {
			lastr=newr;
		}
		
		DetId detId(simHit->detUnitId());
		if(!theTracker->idToDet(detId))
			continue;
		
		Int_t layerNumber=0;
		unsigned int subdetId = static_cast<unsigned int>(detId.subdetId());
		
		if ( subdetId ==  PixelSubdetector::PixelBarrel )
		{
			PXBDetId pxbid(detId.rawId());
			layerNumber = pxbid.layer();
			vPXBSimHits[layerNumber-1]++;
			LogTrace("TrackAnalyzer") << "PXB layer " << layerNumber << ": " << vPXBSimHits[layerNumber-1] << " hit(s)";
		}
		else if ( subdetId ==  PixelSubdetector::PixelEndcap )
		{
			PXFDetId pxfid(detId.rawId());
			layerNumber = pxfid.disk();
			vPXFSimHits[layerNumber-1]++;
			LogTrace("TrackAnalyzer") << "PXF layer " << layerNumber << ": " << vPXFSimHits[layerNumber-1] << " hit(s)";
		}
		else if ( subdetId == StripSubdetector::TIB)		{
			TIBDetId tibid(detId.rawId());
			layerNumber = tibid.layer();
			vTIBSimHits[layerNumber-1]++;
			LogTrace("TrackAnalyzer") << "TIB layer " << layerNumber << ": " << vTIBSimHits[layerNumber-1] << " hit(s)";
		}
		else if ( subdetId ==  StripSubdetector::TOB )
		{
			TOBDetId tobid(detId.rawId());
			layerNumber = tobid.layer();
			vTOBSimHits[layerNumber-1]++;
			LogTrace("TrackAnalyzer") << "TOB layer " << layerNumber << ": " << vTOBSimHits[layerNumber-1] << " hit(s)";
		}
		else if ( subdetId ==  StripSubdetector::TID)
		{
			TIDDetId tidid(detId.rawId());
			layerNumber = tidid.wheel();
			vTIDSimHits[layerNumber-1]++;
			LogTrace("TrackAnalyzer") << "TID layer " << layerNumber << ": " << vTIDSimHits[layerNumber-1] << " hit(s)";
		}
		else if ( subdetId ==  StripSubdetector::TEC )
		{
			TECDetId tecid(detId.rawId());
			layerNumber = tecid.wheel();
			vTECSimHits[layerNumber-1]++;
			LogTrace("TrackAnalyzer") << "TEC layer " << layerNumber << ": " << vTECSimHits[layerNumber-1] << " hit(s)";
		}
			
		
	}// end loop over simHits
	
	for(Int_t i=0; i<9; i++) {
		if(i<3) fPxlSimHitLayers += (vPXBSimHits[i]>0) + (vPXFSimHits[i]>0);
		if(i<3) fSimHitLayers += (vPXBSimHits[i]>0) + (vPXFSimHits[i]>0) + (vTIDSimHits[i]>0);
		if(i<4) fSimHitLayers += (vTIBSimHits[i]>0);
		if(i<6) fSimHitLayers += (vTOBSimHits[i]>0);
		fSimHitLayers += (vTECSimHits[i]>0);
	}
	
	LogTrace("TrackAnalyzer") << "Total # of layers hit by SimTrack: " << fSimHitLayers;
	LogTrace("TrackAnalyzer") << "Total # of pixel layers hit by SimTrack: " << fPxlSimHitLayers;
	
	LogTrace("TrackAnalyzer") << "-------------------------------------------------\n";
	
}

/*****************************************************************************/
void HighPtTrackAnalyzer::checkSimTracks(edm::Handle<TrackingParticleCollection>& simCollection,reco::SimToRecoCollection& q){
	Int_t iSimCount=-1;
	TClonesArray &CASimTemp = *((TClonesArray*)CASim);
		
	for(TrackingParticleCollection::size_type i=0;i < simCollection.product()->size(); ++i){
		const TrackingParticleRef simTrack(simCollection, i);
		
		if(simTrack->charge() != 0){
			if(simTrack->pt()<2.0 && !keepLowPtSimTracks)
				continue;
			
			
			iSimCount++;
			MatchedTrack *TrackTemp=new(CASimTemp[iSimCount]) MatchedTrack();
						
			LogTrace("TrackAnalyzer") << "\n=================================================";
			LogTrace("TrackAnalyzer") << "SimTrack #" << i;
			LogTrace("TrackAnalyzer") << "pT = " << simTrack->pt() << "\t eta = " << simTrack->eta();
			LogTrace("TrackAnalyzer") << "-------------------------------------------------";
			
			// sim
			TrackTemp->iPID=simTrack->pdgId();
			//result.push_back(simTrack->parentVertex()->position().T()); // ?
			TrackTemp->fPt=simTrack->pt();
			TrackTemp->fPx=simTrack->px();
			TrackTemp->fPy=simTrack->py();
			TrackTemp->fPz=simTrack->pz();
			TrackTemp->fPhi=simTrack->phi();
			TrackTemp->fEta=simTrack->eta();
			TrackTemp->fPxlHit=getNumberOfPixelHits(*simTrack,(float *)&fSimPxlLayerHit);
			TrackTemp->fHit=getNumberOfSimHits(*simTrack);
			TrackTemp->fD0=simTrack->vertex().Rho();
			TrackTemp->iq=simTrack->charge();
			
			getSimHitLayerPatterns(*simTrack,vPXBSimHits,vPXFSimHits,vTIBSimHits,vTOBSimHits,vTIDSimHits,vTECSimHits,fSimHitLayers,fPxlSimHitLayers);
			TrackTemp->PXBLayerHits=vPXBSimHits;
			TrackTemp->PXFLayerHits=vPXFSimHits;
			TrackTemp->TIBLayerHits=vTIBSimHits;
			TrackTemp->TOBLayerHits=vTOBSimHits;
			TrackTemp->TIDLayerHits=vTIDSimHits;
			TrackTemp->TECLayerHits=vTECSimHits;
			TrackTemp->fHitLayers=fSimHitLayers;
			TrackTemp->fPxlLayerHit=fPxlSimHitLayers;

			// reco
			edm::RefToBase<reco::Track> matchedRecTrack;
			int nRec=0;
			int nSharedT=0;
			int nShared=0;
			
			if(q.find(simTrack)!=q.end()){
				try{
					
					vector<pair<edm::RefToBase<reco::Track>, double> > recTracks = q[simTrack];
					for(vector<pair<edm::RefToBase<reco::Track>,double> >::const_iterator it = recTracks.begin(); it != recTracks.end(); ++it){
						edm::RefToBase<reco::Track> recTrack = it->first;
						if(!useAbsoluteNumberOfHits) {
							nShared=(int)(it->second * TrackTemp->fHit + 0.5); //quality = # of shared hits/total # of sim hits
						} else {
							nShared=(int)(it->second+0.5); //quality = # of shared hits
						}
						if(nSharedT<nShared){
							nSharedT=nShared;
							matchedRecTrack=recTrack; 
						}
						nRec++;
					}
				}
				catch (cms::Exception& event){
				}
				
				TrackTemp->iMatches=nRec;
				
				if(nSharedT > 0){
					
					LogTrace("TrackAnalyzer") << "Matched Rec Track";
					LogTrace("TrackAnalyzer") << "pT = " << matchedRecTrack->pt() << "\t eta = " << matchedRecTrack->eta();
					LogTrace("TrackAnalyzer") << "-------------------------------------------------";
					
					TrackTemp->fMPt=matchedRecTrack->pt();
					TrackTemp->fMPx=matchedRecTrack->px();
					TrackTemp->fMPy=matchedRecTrack->py();
					TrackTemp->fMPz=matchedRecTrack->pz();
					TrackTemp->fMPhi=matchedRecTrack->phi();
					TrackTemp->fMEta=matchedRecTrack->eta();
					TrackTemp->fMD0=matchedRecTrack->d0();
					TrackTemp->fMD0Err=matchedRecTrack->d0Error();
					TrackTemp->iMq=matchedRecTrack->charge();
					TrackTemp->fMHit=matchedRecTrack->recHitsSize();
					TrackTemp->fMPxlHit=getNumberOfRecPixelHits(*matchedRecTrack,(float *)&fRecPxlLayerHit);
					TrackTemp->fMChi2=matchedRecTrack->chi2();
					TrackTemp->fMChi2Norm=matchedRecTrack->normalizedChi2();
					TrackTemp->fHitMatched=(Float_t)nSharedT;
					TrackTemp->fMZ=matchedRecTrack->dz();
					TrackTemp->fMZErr=matchedRecTrack->dzError();
					/* TrackTemp->fMZ=matchedRecTrack->dx();
					 TrackTemp->fMZErr=matchedRecTrack->dxError();
					 TrackTemp->fMZ=matchedRecTrack->dy();
					 TrackTemp->fMZErr=matchedRecTrack->dyError();*/
					TrackTemp->fMValidHits=matchedRecTrack->numberOfValidHits();
					
					getRecHitLayerPatterns(*matchedRecTrack,vPXBHits,vPXFHits,vTIBHits,vTOBHits,vTIDHits,vTECHits,fRecHitLayers,fPxlRecHitLayers);
					TrackTemp->PXBLayerMHits=vPXBHits;
					TrackTemp->PXFLayerMHits=vPXFHits;
					TrackTemp->TIBLayerMHits=vTIBHits;
					TrackTemp->TOBLayerMHits=vTOBHits;
					TrackTemp->TIDLayerMHits=vTIDHits;
					TrackTemp->TECLayerMHits=vTECHits;
					TrackTemp->fMHitLayers=fRecHitLayers;
					TrackTemp->fMPxlLayerHit=fPxlRecHitLayers;
					
					LogTrace("TrackAnalyzer") << "-------------------------------------------------";
					
				}else{
					
				}
			}else{
				TrackTemp->iMatches=0;
			}
		}//end if charge!=0;
	}
}

/*****************************************************************************/
pair<float,float> HighPtTrackAnalyzer::refitWithVertex(const reco::Track & recTrack,const reco::VertexCollection* vertices){
	TransientTrack theTransientTrack = theTTBuilder->build(recTrack);
	
	// If there are vertices found
	if(vertices->size() > 0)
	{
		float dzmin = -1.;
		const reco::Vertex * closestVertex = 0;
		
		// Look for the closest vertex in z
		for(reco::VertexCollection::const_iterator
			vertex = vertices->begin(); vertex!= vertices->end(); vertex++)
		{
			float dz = fabs(recTrack.vertex().z() - vertex->position().z());
			if(vertex == vertices->begin() || dz < dzmin)
			{ dzmin = dz ; closestVertex = &(*vertex); }
		}
		
		// Get vertex position and error matrix
		GlobalPoint vertexPosition(closestVertex->position().x(),
								   closestVertex->position().y(),
								   closestVertex->position().z());
		
		float beamSize = 15e-4; // 15 um
		GlobalError vertexError(beamSize*beamSize, 0,
								beamSize*beamSize, 0,
								0,closestVertex->covariance(2,2));
		
		// sometimes the median vertex producer gives very large errors that cause a crash on matrix inversion
		if(closestVertex->covariance(2,2)>1.0) {
			//LogWarning("TrackAnalyzer") << "vtx position = (" << closestVertex->position().x() << "," << closestVertex->position().y() << "," << closestVertex->position().z() << ")";
			//LogWarning << "vertex cov(2,2) = " << closestVertex->covariance(2,2);
			return pair<float,float>(recTrack.pt(), -9999);
		} else {
			// Refit track with vertex constraint
			SingleTrackVertexConstraint stvc;
			pair<TransientTrack, float> result =
			stvc.constrain(theTransientTrack, vertexPosition, vertexError);
			
			//LogDebug("TrackAnalyzer") << "Track refitted with vertex constraint: pT = " << result.first.impactPointTSCP().pt();
			//LogDebug("TrackAnalyzer") << ", chi2 = " << result.second;
			
			return pair<float,float>(result.first.impactPointTSCP().pt(),
									 result.second);
		}
	}
	else
		return pair<float,float>(recTrack.pt(), -9999);
	
}

/*****************************************************************************/
int HighPtTrackAnalyzer::getParticleId(edm::RefToBase<reco::Track>& recTrack, int & ptype)
{
	int pid = 0;
	ptype = 0;
	double tmin = 0.;
	
	for(trackingRecHit_iterator recHit = recTrack->recHitsBegin();
		recHit!= recTrack->recHitsEnd(); recHit++)
	{
		vector<PSimHit> simHits = theHitAssociator->associateHit(**recHit);
		
		for(vector<PSimHit>::const_iterator simHit = simHits.begin(); 
			simHit!= simHits.end(); simHit++)
			if(simHit == simHits.begin() || simHit->tof() < tmin )
			{
				pid   = simHit->particleType();
				ptype = simHit->processType();
				tmin  = simHit->tof();
			}
	}  
	
	return pid;
}

/*****************************************************************************/
void HighPtTrackAnalyzer::checkRecTracks(edm::Handle<edm::View<reco::Track> >& recCollection,const reco::VertexCollection* vertices,reco::RecoToSimCollection& p){
	
	Int_t iRecCount=-1;
	TClonesArray &CARecTemp = *((TClonesArray*)CAReco);
	
	for(edm::View<reco::Track> ::size_type i=0;i < recCollection.product()->size(); ++i){
		edm::RefToBase<reco::Track> recTrack(recCollection, i);
		iRecCount++;
		MatchedTrack *TrackTemp=new(CARecTemp[iRecCount]) MatchedTrack();
		
		LogTrace("TrackAnalyzer") << "\n=================================================";
		LogTrace("TrackAnalyzer") << "RecTrack #" << i;
		LogTrace("TrackAnalyzer") << "pT = " << recTrack->pt() << "\t eta = " << recTrack->eta();
		LogTrace("TrackAnalyzer") << "-------------------------------------------------";
		
		TrackTemp->fPt=recTrack->pt();
		TrackTemp->fPx=recTrack->px();
		TrackTemp->fPy=recTrack->py();
		TrackTemp->fPz=recTrack->pz();
		TrackTemp->fPhi=recTrack->phi();
		TrackTemp->fEta=recTrack->eta();
		TrackTemp->fD0=recTrack->d0();
		TrackTemp->fD0Err=recTrack->d0Error();
		TrackTemp->iq=recTrack->charge();
		TrackTemp->fHit=recTrack->recHitsSize();
		TrackTemp->fPxlHit=getNumberOfRecPixelHits(*recTrack,(float *)&fRecPxlLayerHit);
		TrackTemp->fChi2=recTrack->chi2();
		TrackTemp->fChi2Norm=recTrack->normalizedChi2();
		TrackTemp->fZ=recTrack->dz();
		TrackTemp->fZErr=recTrack->dzError();
		/* TrackTemp->fMZ=matchedRecTrack->dx();
		 TrackTemp->fMZErr=matchedRecTrack->dxError();
		 TrackTemp->fMZ=matchedRecTrack->dy();
		 TrackTemp->fMZErr=matchedRecTrack->dyError();*/
		TrackTemp->fRefitPt=refitWithVertex(*recTrack,vertices).first;
		TrackTemp->fRefitChi2=refitWithVertex(*recTrack,vertices).second;
		TrackTemp->fValidHits=recTrack->numberOfValidHits();
		
		getRecHitLayerPatterns(*recTrack,vPXBHits,vPXFHits,vTIBHits,vTOBHits,vTIDHits,vTECHits,fRecHitLayers,fPxlRecHitLayers);
		TrackTemp->PXBLayerHits=vPXBHits;
		TrackTemp->PXFLayerHits=vPXFHits;
		TrackTemp->TIBLayerHits=vTIBHits;
		TrackTemp->TOBLayerHits=vTOBHits;
		TrackTemp->TIDLayerHits=vTIDHits;
		TrackTemp->TECLayerHits=vTECHits;
		TrackTemp->fHitLayers=fRecHitLayers;
		TrackTemp->fPxlLayerHit=fPxlRecHitLayers;


		// sim 
		TrackingParticleRef matchedSimTrack;
		int nSim = 0;
		
		Float_t fHitMatch=0.0;
		
		try{
			vector<pair<TrackingParticleRef, double> > simTracks = p[recTrack];
			
			for(vector<pair<TrackingParticleRef, double> >::const_iterator it = simTracks.begin(); it != simTracks.end(); ++it){
				TrackingParticleRef simTrack = it->first;
				float fraction = it->second;
				
				// If more than half is shared
				if(fraction>0.0){
					if(fraction>fHitMatch){ 
						matchedSimTrack = simTrack;
						fHitMatch=fraction;
					}
					nSim++;
				}
				
			}
		}catch (cms::Exception& event){ 
		}
		
		
		if(nSim > 0){
			
			int parentId;
			float T;
			
			int ptype;
			//int ids = getParticleId(recTrack, ptype); // EDIT: gave compiler warning
			getParticleId(recTrack, ptype);
			
			if(matchedSimTrack->parentVertex()->nSourceTracks() == 0){
				// track is primary, has no parent
				// recTrack can be a true primary, or an untracked daughter
				if(ptype == 2) parentId = 0;                        // primary
				else parentId = matchedSimTrack->pdgId(); // hadronic, decay
			}else{
				// track is not primary, has a parent
				TrackingVertex::tp_iterator iv =matchedSimTrack->parentVertex()->sourceTracks_begin();
				parentId = (*iv)->pdgId();
			}
			
			T = matchedSimTrack->parentVertex()->position().T(); // ?
			
			LogTrace("TrackAnalyzer") << "Matched Sim Track";
			LogTrace("TrackAnalyzer") << "pT = " << matchedSimTrack->pt() << "\t eta = " << matchedSimTrack->eta();
			LogTrace("TrackAnalyzer") << "-------------------------------------------------";
			
			TrackTemp->iPID=matchedSimTrack->pdgId();
			TrackTemp->iMParentPID=parentId;
			//result.push_back(simTrack->parentVertex()->position().T()); // ?
			TrackTemp->fMPt=matchedSimTrack->pt();
			TrackTemp->fMPx=matchedSimTrack->px();
			TrackTemp->fMPy=matchedSimTrack->py();
			TrackTemp->fMPz=matchedSimTrack->pz();
			TrackTemp->fMPhi=matchedSimTrack->phi();
			TrackTemp->fMEta=matchedSimTrack->eta();
			TrackTemp->fMPxlHit=getNumberOfPixelHits(*matchedSimTrack,(float *)&fSimPxlLayerHit);
			TrackTemp->fMHit=getNumberOfSimHits(*matchedSimTrack);
			TrackTemp->fMD0=matchedSimTrack->vertex().Rho();
			TrackTemp->iMq=matchedSimTrack->charge();
			TrackTemp->fHitMatched=fHitMatch;
			TrackTemp->iMatches=nSim;
			
			getSimHitLayerPatterns(*matchedSimTrack,vPXBSimHits,vPXFSimHits,vTIBSimHits,vTOBSimHits,vTIDSimHits,vTECSimHits,fSimHitLayers,fPxlSimHitLayers);
			TrackTemp->PXBLayerMHits=vPXBSimHits;
			TrackTemp->PXFLayerMHits=vPXFSimHits;
			TrackTemp->TIBLayerMHits=vTIBSimHits;
			TrackTemp->TOBLayerMHits=vTOBSimHits;
			TrackTemp->TIDLayerMHits=vTIDSimHits;
			TrackTemp->TECLayerMHits=vTECSimHits;
			TrackTemp->fMHitLayers=fSimHitLayers;
			TrackTemp->fMPxlLayerHit=fPxlSimHitLayers;

			
			LogTrace("TrackAnalyzer") << "-------------------------------------------------";

			
		}else {
		}  
	}
}

/*****************************************************************************/
void HighPtTrackAnalyzer::analyze(const edm::Event& ev, const edm::EventSetup& es){
	
	LogVerbatim("TrackAnalyzer")<<"&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&";
	
	// Get associator
	theHitAssociator = new TrackerHitAssociator::TrackerHitAssociator(ev);
	
	// Get generated info
	edm::Handle<edm::HepMCProduct> hepEv;
	ev.getByLabel("source",hepEv);
	const HepMC::GenEvent * inev = hepEv->GetEvent();
	HepMC::HeavyIon* hi = inev->heavy_ion();
	proc = inev->signal_process_id();

	// the heavy ion event info
	if(infoHiEventTopology) {
	   ImpactPara = hi->impact_parameter();
	   Npart      = hi->Npart_proj() + hi->Npart_targ();
	   Ncoll      = hi->Ncoll();
	   Nhard      = hi->Ncoll_hard();
	   Phi0       = hi->event_plane_angle();
	}
	LogVerbatim("TrackAnalyzer") <<"Event Number : "<<ev.id().event();
	LogVerbatim("TrackAnalyzer") <<"[HighPtTrackAnalyzer] process = "<<proc;
	
	// Get signal process vertex
	HepMC::GenVertex* genvtx = inev->signal_process_vertex();
	HepMC::FourVector* vtx_;

	if(!genvtx){
		HepMC::GenEvent::particle_const_iterator pt=inev->particles_begin();
		HepMC::GenEvent::particle_const_iterator ptend=inev->particles_end();
		while(!genvtx || ( genvtx->particles_in_size() == 1 && pt != ptend ) ){
			++pt;
			genvtx = (*pt)->production_vertex();
		}
	}

	vtx_ = &(genvtx->position());
	SimVtx = 0.1 * vtx_->z(); // hepMC gen vtx is in mm.  everything else is cm so we divide by 10 ;)
	LogVerbatim("TrackAnalyzer") << "[HighPtTrackAnalyzer] vz = " << SimVtx << " cm";
	if(infoHiEventTopology) LogVerbatim("TrackAnalyzer") << "[HighPtTrackAnalyzer] b = " << ImpactPara << " fm\t Npart = " << Npart << "\t RP = " << Phi0;
	
	
	// Get simulated tracks
	edm::Handle<TrackingParticleCollection> simCollection;
	//ev.getByLabel("trackingtruthprod",simCollection); //name changed in trackingParticles_cfi between 2_0_5 and 2_1_7
	ev.getByLabel("mergedtruth",simCollection);
	//  ev.getByType(simCollection);
	
	LogVerbatim("TrackAnalyzer")<<"[HighPtTrackAnalyzer] simTracks = "<<simCollection.product()->size();
	
	// Get reconstructed tracks
	edm::Handle<edm::View<reco::Track> >  recCollection;
	ev.getByLabel(trackCollectionLabels[0], recCollection); // !!
	
	LogVerbatim("TrackAnalyzer")<<"[HighPtTrackAnalyzer] recTracks = "<<recCollection.product()->size();
	
	
	// Get reconstructed vertices
	edm::Handle<reco::VertexCollection> vertexCollection;
	ev.getByLabel("pixelVertices",vertexCollection);
	const reco::VertexCollection * vertices = vertexCollection.product();
	
	iTrkReco = recCollection.product()->size();
	iTrkSim=simCollection.product()->size();
	iVtx = vertexCollection.product()->size();
	if (iVtx>0) RecVtx = vertices->begin()->position().z();
	iEvent=ev.id().event();
	iRun=ev.id().run();
	
	CAReco->Clear();
	CASim->Clear();
	
	
	// Associators
	reco::SimToRecoCollection simToReco=theAssociatorByHits->associateSimToReco(recCollection, simCollection,&ev);
	
	
	reco::RecoToSimCollection recoToSim=theAssociatorByHits->associateRecoToSim(recCollection, simCollection,&ev);
	
	// Analyze
	checkSimTracks(simCollection,simToReco);
	
	checkRecTracks(recCollection, vertices, recoToSim);
	
	recInfoTree->Fill();
	
	
	LogVerbatim("TrackAnalyzer")<<"[HighPtTrackAnalyzer] done, "<<ev.id();
	LogVerbatim("TrackAnalyzer")<<"----------------------------------------------\n";
	
	delete theHitAssociator;
}


DEFINE_FWK_MODULE(HighPtTrackAnalyzer);
