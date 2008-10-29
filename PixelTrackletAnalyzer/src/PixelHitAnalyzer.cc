// -*- C++ -*-
//
// Package:    PixelHitAnalyzer
// Class:      PixelHitAnalyzer
// 
/**\class PixelHitAnalyzer PixelHitAnalyzer.cc MitHig/PixelHitAnalyzer/src/PixelHitAnalyzer.cc

 Description: <one line class summary>

 Implementation:
     <Notes on implementation>
*/
//
// Original Author:  Yilmaz Yetkin
//         Created:  Tue Sep 30 15:14:28 CEST 2008
// $Id: PixelHitAnalyzer.cc,v 1.7 2008/10/23 16:36:03 yjlee Exp $
//
//


// system include files
#include <memory>
#include <iostream>
#include <vector>
#include <string>
#include <map>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"

#include "FWCore/ServiceRegistry/interface/Service.h"
#include "PhysicsTools/UtilAlgos/interface/TFileService.h"
#include "Geometry/TrackerGeometryBuilder/interface/PixelGeomDetUnit.h"
#include "DataFormats/TrackerRecHit2D/interface/SiPixelRecHitCollection.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"
#include "DataFormats/GeometryVector/interface/GlobalPoint.h"
#include "DataFormats/SiPixelDetId/interface/PXBDetId.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "SimDataFormats/TrackingAnalysis/interface/TrackingVertex.h"
#include "SimDataFormats/TrackingAnalysis/interface/TrackingVertexContainer.h"
#include "SimDataFormats/TrackingAnalysis/interface/TrackingParticle.h"
#include "SimDataFormats/TrackingHit/interface/PSimHit.h"
#include "SimTracker/TrackerHitAssociation/interface/TrackerHitAssociator.h"
#include "SimGeneral/HepPDTRecord/interface/ParticleDataTable.h"

#include "TTree.h"
#include "TNtuple.h"

using namespace std;
using namespace edm;
using namespace reco;

//
// class decleration
//

#define MAXPARTICLES 1000
#define MAXHITS 1000
#define MAXVTX 10

struct PixelEvent{
   int nhits1;
   int nhits2;

   int mult;
   //   int mult2;
   int npart;

   int nv;
   float vz[MAXVTX];

   float eta1[MAXHITS];
   float phi1[MAXHITS];
   float r1[MAXHITS];
   int id1[MAXHITS];
   float cs1[MAXHITS];
   float ch1[MAXHITS];
   int gp1[MAXHITS];
   int type1[MAXHITS];

   float eta2[MAXHITS];
   float phi2[MAXHITS];
   float r2[MAXHITS];
   int id2[MAXHITS];
   float cs2[MAXHITS];
   float ch2[MAXHITS];
   int gp2[MAXHITS];
   int type2[MAXHITS];

   float pt[MAXPARTICLES];
   float eta[MAXPARTICLES];
   float phi[MAXPARTICLES];
   int pdg[MAXPARTICLES];
   int chg[MAXPARTICLES];

};


class PixelHitAnalyzer : public edm::EDAnalyzer {
   public:
      explicit PixelHitAnalyzer(const edm::ParameterSet&);
      ~PixelHitAnalyzer();

   private:
      virtual void beginJob(const edm::EventSetup&) ;
      virtual void analyze(const edm::Event&, const edm::EventSetup&);
      virtual void endJob() ;
   void fillVertices(const edm::Event& iEvent);
   void fillHits(const edm::Event& iEvent);
   void fillParticles(const edm::Event& iEvent);

   int associateSimhitToTrackingparticle(unsigned int trid );
   bool checkprimaryparticle(const TrackingParticle* tp);

      // ----------member data ---------------------------

   //  const char* betafile;
   //  TrackletFinder* finder_;
   //  TrackletCorrections* corrections_;

   bool doMC_;
   vector<string> vertexSrc_;
   double etaMult_;

  const TrackerGeometry* geo_;
  edm::Service<TFileService> fs;           
   edm::ESHandle < ParticleDataTable > pdt;
   edm::Handle<TrackingParticleCollection> trackingParticles;

  map<int,int> tpmap_;

  TTree* pixelTree_;
   TNtuple* nt;
   TNtuple* nt2;

  PixelEvent pev_;

};

//
// constants, enums and typedefs
//

//
// static data member definitions
//

//
// constructors and destructor
//
PixelHitAnalyzer::PixelHitAnalyzer(const edm::ParameterSet& iConfig)

{
   //now do what ever initialization is needed
   doMC_             = iConfig.getUntrackedParameter<bool>  ("doMC",true);
   vertexSrc_ = iConfig.getParameter<vector<string> >("vertexSrc");
   etaMult_ = iConfig.getUntrackedParameter<double>  ("nHitsRegion",1.);
}

PixelHitAnalyzer::~PixelHitAnalyzer()
{
 
   // do anything here that needs to be done at desctruction time
   // (e.g. close files, deallocate resources etc.)

}


//
// member functions
//

// ------------ method called to for each event  ------------
void
PixelHitAnalyzer::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{

   tpmap_.clear();
   pev_.nhits1 = 0;
   pev_.nhits2 = 0;
   pev_.mult = 0;
   pev_.npart = 0;

   pev_.nv = 0;

   fillParticles(iEvent);
   fillVertices(iEvent);
   fillHits(iEvent);

   map<int,int>::iterator begin = tpmap_.begin();
   map<int,int>::iterator end = tpmap_.end();
   for(map<int,int>::iterator it = begin; it != end; ++it){
      cout<<"Barcode : "<<(*it).first<<" Particle : "<<(*it).second<<endl;
   } 

   pixelTree_->Fill();

}

void
PixelHitAnalyzer::fillVertices(const edm::Event& iEvent){

   if(doMC_){
      int daughter = 0;
      int nVertex = 0;
      int greatestvtx = 0;
      Handle<TrackingVertexCollection> vertices;
      iEvent.getByLabel("mergedtruth","MergedTrackTruth", vertices);
      nVertex = vertices->size();
      for (unsigned int i = 0 ; i< vertices->size(); ++i){
	 daughter = (*vertices)[i].nDaughterTracks();
	 if( daughter > (*vertices)[greatestvtx].nDaughterTracks()) greatestvtx = i;
      }
      
      if(vertices->size()>0){
	 pev_.vz[pev_.nv] = (*vertices)[greatestvtx].position().z();
      }else{
	 pev_.vz[pev_.nv] =  -99; 
      }
      pev_.nv++;
   } else {
      // Fill a dummy MC information
      pev_.vz[pev_.nv] = -99;
      pev_.nv++;
   }
   
   for(int iv = 0; iv < vertexSrc_.size(); ++iv){
      const reco::VertexCollection * recoVertices;
      edm::Handle<reco::VertexCollection> vertexCollection;
      iEvent.getByLabel(vertexSrc_[iv],vertexCollection);
      recoVertices = vertexCollection.product();
      int daughter = 0;
      int nVertex = 0;
      int greatestvtx = 0;
      
      nVertex = recoVertices->size();
      for (unsigned int i = 0 ; i< recoVertices->size(); ++i){
	 daughter = (*recoVertices)[i].tracksSize();
	 if( daughter > (*recoVertices)[greatestvtx].tracksSize()) greatestvtx = i;
         cout <<"Vertex: "<< (*recoVertices)[i].position().z()<<" "<<daughter<<endl;
      }
      
      if(recoVertices->size()>0){
	 pev_.vz[pev_.nv] = (*recoVertices)[greatestvtx].position().z();
      }else{
	 pev_.vz[pev_.nv] =  -99;
      }
      cout <<"==>Primary Vertex: "<<pev_.vz[pev_.nv]<<" "<<greatestvtx<<" "<<endl;
      pev_.nv++;
   }

}

void
PixelHitAnalyzer::fillHits(const edm::Event& iEvent){

   TrackerHitAssociator theHitAssociator(iEvent);
   if(doMC_)iEvent.getByLabel("mergedtruth","MergedTrackTruth",trackingParticles);
   
   const SiPixelRecHitCollection* rechits;
   Handle<SiPixelRecHitCollection> rchts;
   iEvent.getByLabel("siPixelRecHits",rchts);
   rechits = rchts.product();

   for(SiPixelRecHitCollection::id_iterator id = rechits->id_begin(); id!= rechits->id_end(); id++){
      if((*id).subdetId() == int(PixelSubdetector::PixelBarrel)){
	 PXBDetId pid(*id);
	 SiPixelRecHitCollection::range range;
	 int layer = pid.layer();
	 if(layer == 1 || layer == 2) range = rechits->get(*id);
	 for(SiPixelRecHitCollection::const_iterator recHit = range.first; recHit!= range.second; recHit++){
	    
	    int ptype = -99;
	    bool isprimary = false;
	    bool issecondary = false;
	    bool isbackground = false;

	    const SiPixelRecHit* recHit1 = &*recHit;

	    cout<<"For the same hit : -------------------"<<endl;

            // GEOMETRY INFO                        

            const PixelGeomDetUnit* pixelLayer = dynamic_cast<const PixelGeomDetUnit*> (geo_->idToDet(recHit1->geographicalId()));
            GlobalPoint gpos = pixelLayer->toGlobal(recHit1->localPosition());

            //Removed by Yen-Jie, we do the calculation in root level.          
            //double vertex = 0;                                                
            //if(pev_.vz[(int)(!doMC_)] != -99) vertex = pev_.vz[(int)(!doMC_); 
	    //	    math::XYZVector rechitPos(gpos.x(),gpos.y(),gpos.z()-pev_.vz[0]);
	    math::XYZVector rechitPos(gpos.x(),gpos.y(),gpos.z()); 
            
	    double eta = rechitPos.eta();
            double phi = rechitPos.phi();
            double r = rechitPos.rho();

	    // SIM INFO
	    int gpid = -9999;
	    int trid = -9999;
	    if (doMC_) {
	       vector<PSimHit> simHits1 = theHitAssociator.associateHit(*recHit1);
	       const PSimHit * bestSimHit1 = 0;
	       int simIdx =0;

	       //gets the primary simhit and its specifications for the rechit   	     
	       for(vector<PSimHit>::const_iterator simHit1 = simHits1.begin(); simHit1!= simHits1.end(); simHit1++){  
		  simIdx++;
		  unsigned int associatedTPID = associateSimhitToTrackingparticle((*simHit1).trackId());
		  ptype = (&(*simHit1))->processType();

                  GlobalPoint simpos = pixelLayer->toGlobal((*simHit1).localPosition());
                  nt2->Fill(eta,phi,simpos.eta(),simpos.phi(),ptype);

		  if (associatedTPID == -1){
		     isbackground = true;
		     continue;    // doesn't match to any Trackingparticle
		  }
		  
		  const TrackingParticle* associatedTP = &(*trackingParticles)[associatedTPID];
		  
		  TrackingParticle::genp_iterator itb = associatedTP->genParticle_begin();
		  TrackingParticle::genp_iterator itend = associatedTP->genParticle_end();

		  isprimary = checkprimaryparticle(associatedTP);
                  issecondary = itb == itend;

		  if(itb == itend){
		     cout<<"This is a secondary particle"<<endl;
		  }

		  cout<<"TP eta : "<<associatedTP->eta()<<" phi : "<<associatedTP->phi()<<endl;

		  for(TrackingParticle::genp_iterator itp = itb; itp != itend; ++itp){
		     gpid = tpmap_[(*itp)->barcode()];
		     cout<<" Particle : "<<gpid<<endl;
		  }
		  
		  if (isprimary && bestSimHit1==0){ 
		     bestSimHit1 = &(*simHit1);
		     break;
		  }
	       } 
	       
	       if(bestSimHit1!=0){
		  trid = bestSimHit1->trackId();  

                  GlobalPoint simpos = pixelLayer->toGlobal((*bestSimHit1).localPosition());
		  nt->Fill(eta,phi,simpos.eta(),simpos.phi(),0);

		  cout<<" trid : "<<trid<<endl;

	       }
	    }
	    
	    int type = -99;
	    if(isbackground) type = 0;
	    if(isprimary) type = 1;
	    if(ptype != 2) type = 2;
            if(issecondary) type = 3;

	    cout<<"Hit eta : "<<eta<<" phi : "<<phi<<endl;
            cout<<"Particle eta : "<<pev_.eta[gpid]<<" phi : "<<pev_.phi[gpid]<<endl;

	    if(layer == 1){ 
	       pev_.eta1[pev_.nhits1] = eta;
	       pev_.phi1[pev_.nhits1] = phi;
	       pev_.r1[pev_.nhits1] = r;
	       pev_.id1[pev_.nhits1] = trid;
	       pev_.cs1[pev_.nhits1] = recHit1->cluster()->size(); //Cluster Size
               pev_.ch1[pev_.nhits1] = recHit1->cluster()->charge(); //Cluster Charge
	       pev_.gp1[pev_.nhits1] = gpid;
	       pev_.type1[pev_.nhits1] = type;
	       pev_.nhits1++;
	       if(fabs(gpos.eta()) < etaMult_ ) pev_.mult++;
	    }
	    if(layer == 2){
	       pev_.eta2[pev_.nhits2] = eta;
	       pev_.phi2[pev_.nhits2] = phi;
	       pev_.r2[pev_.nhits2] = r;
               pev_.id2[pev_.nhits2] = trid;
	       pev_.cs2[pev_.nhits2] = recHit1->cluster()->size(); //Cluster Size
               pev_.ch2[pev_.nhits2] = recHit1->cluster()->charge(); //Cluster Charge
	       pev_.gp2[pev_.nhits2] = gpid;
	       pev_.type2[pev_.nhits2] = type;
	       pev_.nhits2++;
	    } 
	    
	 }
      }
   }
}

void
PixelHitAnalyzer::fillParticles(const edm::Event& iEvent){

   Handle<HepMCProduct> mc;
   iEvent.getByLabel("source",mc);
   const HepMC::GenEvent* evt = mc->GetEvent();

   int all = evt->particles_size();
   HepMC::GenEvent::particle_const_iterator begin = evt->particles_begin();
   HepMC::GenEvent::particle_const_iterator end = evt->particles_end();
   for(HepMC::GenEvent::particle_const_iterator it = begin; it != end; ++it){
      if((*it)->status() != 1) continue;
	 tpmap_[(*it)->barcode()] = pev_.npart;
	 pev_.pdg[pev_.npart] = (*it)->pdg_id();
	 pev_.eta[pev_.npart] = (*it)->momentum().eta();
         pev_.phi[pev_.npart] = (*it)->momentum().phi();
	 pev_.pt[pev_.npart] = (*it)->momentum().perp();
	 const ParticleData * part = pdt->particle(pev_.pdg[pev_.npart]);
	 pev_.chg[pev_.npart] = part->charge();

	 cout<<" Particle "<<pev_.npart<<" eta : "<<pev_.eta[pev_.npart]<<" phi : "<<pev_.phi[pev_.npart]<<" pt : "<<pev_.pt[pev_.npart]<<endl; 

	 pev_.npart++;


   }
}

int PixelHitAnalyzer::associateSimhitToTrackingparticle(unsigned int trid )
{
   int ref=-1;

   const TrackingParticleCollection* TPCProd = trackingParticles.product();
   for (TrackingParticleCollection::size_type i=0; i<TPCProd->size(); i++){
      const TrackingParticle* tp = &(*TPCProd)[i];
      vector <PSimHit> particlesimhits = tp->trackPSimHit();
      for(vector<PSimHit>::const_iterator simhit = particlesimhits.begin(); simhit != particlesimhits.end(); ++simhit)
	 {
	    //cout <<"       matching TP: "<<i<<" TPsimhitid: "<<simhit->trackId()<<" simhitId: "<<trid<<endl;
	    if(simhit->trackId()==trid)//  checkprimaryparticle(tp))
	       {
		  ref=i;
		  break;
	       }
	 }
      if (ref!=-1) break;
   }

   return ref;
}
   
bool PixelHitAnalyzer::checkprimaryparticle(const TrackingParticle* tp)
{
   int primarycheck=2;

   if(((tp->charge()==1)||(tp->charge()==-1))&&(tp->vertex().Rho()<0.2))
      {
	 primarycheck=1;
      } else {
	 primarycheck=0;
      }
   return primarycheck;
}       



// ------------ method called once each job just before starting event loop  ------------
void 
PixelHitAnalyzer::beginJob(const edm::EventSetup& iSetup)
{
  
  //  TFile* infile = new TFile(betafile,"read");
  //  corrections_  = new TrackletCorrections(infile);
   //  corrections_  = new TrackletCorrections(1,1,1);

  edm::ESHandle<TrackerGeometry> tGeo;
  iSetup.get<TrackerDigiGeometryRecord>().get(tGeo);
  geo_ = tGeo.product();
  iSetup.getData(pdt);

  //  finder_ = new TrackletFinder(corrections_,trGeo,true);
  nt = fs->make<TNtuple>("nt","Debug Ntuple","receta:recphi:simeta:simphi:process");
  nt2 = fs->make<TNtuple>("nt2","Debug Ntuple All SimHits","receta:recphi:simeta:simphi:process");

  pixelTree_ = fs->make<TTree>("PixelTree","Tree of Pixel Hits");
  pixelTree_->Branch("nhits1",&pev_.nhits1,"nhits1/I");
  pixelTree_->Branch("nhits2",&pev_.nhits2,"nhits2/I");
  pixelTree_->Branch("mult",&pev_.mult,"mult/I");
  //  pixelTree_->Branch("mult2",&pev_.mult2,"mult2/I");
  pixelTree_->Branch("nv",&pev_.nv,"nv/I");
  pixelTree_->Branch("vz",pev_.vz,"vz[nv]/F");
  pixelTree_->Branch("eta1",pev_.eta1,"eta1[nhits1]/F");
  pixelTree_->Branch("phi1",pev_.phi1,"phi1[nhits1]/F");
  pixelTree_->Branch("r1",pev_.r1,"r1[nhits1]/F");
  pixelTree_->Branch("id1",pev_.id1,"id1[nhits1]/I");
  pixelTree_->Branch("cs1",pev_.cs1,"cs1[nhits1]/F");
  pixelTree_->Branch("ch1",pev_.ch1,"ch1[nhits1]/F");
  pixelTree_->Branch("gp1",pev_.gp1,"gp1[nhits1]/I");
  pixelTree_->Branch("type1",pev_.type1,"type1[nhits1]/I");

  pixelTree_->Branch("eta2",pev_.eta2,"eta2[nhits2]/F");
  pixelTree_->Branch("phi2",pev_.phi2,"phi2[nhits2]/F");
  pixelTree_->Branch("r2",pev_.r2,"r2[nhits2]/F");
  pixelTree_->Branch("id2",pev_.id2,"id2[nhits2]/I");
  pixelTree_->Branch("cs2",pev_.cs2,"cs2[nhits2]/F");
  pixelTree_->Branch("ch2",pev_.ch2,"ch2[nhits2]/F");
  pixelTree_->Branch("gp2",pev_.gp2,"gp2[nhits2]/I");
  pixelTree_->Branch("type2",pev_.type2,"type2[nhits2]/I");

  pixelTree_->Branch("npart",&pev_.npart,"npart/I");
  pixelTree_->Branch("pt",pev_.pt,"pt[npart]/F");
  pixelTree_->Branch("eta",pev_.eta,"eta[npart]/F");
  pixelTree_->Branch("phi",pev_.phi,"phi[npart]/F");
  pixelTree_->Branch("pdg",pev_.pdg,"pdg[npart]/I");
  pixelTree_->Branch("chg",pev_.chg,"chg[npart]/I");

}

// ------------ method called once each job just after ending the event loop  ------------
void 
PixelHitAnalyzer::endJob() {
}

//define this as a plug-in
DEFINE_FWK_MODULE(PixelHitAnalyzer);
