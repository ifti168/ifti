// -*- C++ -*-
#include "Rivet/Analysis.hh"
#include "Rivet/Projections/FinalState.hh"
#include "Rivet/Projections/PromptFinalState.hh"
#include "Rivet/Projections/ChargedFinalState.hh"
#include "Rivet/Tools/AliceCommon.hh"
#include "Rivet/Projections/AliceCommon.hh"
#include "Rivet/Projections/PromptFinalState.hh"
#include "Rivet/Projections/PrimaryParticles.hh"
#include <fstream>
#include <iostream>
#include <math.h>
#include <vector>
#include "../Centralities/RHICCentrality.hh"

#define _USE_MATH_DEFINES

using namespace std;

namespace Rivet {

	class Correlator {

    private:
      std::vector<int> _indices;
      string _collSystemAndEnergy;
      pair<double,double> _centrality;
      pair<double,double> _triggerRange;
      pair<double,double> _associatedRange;
      vector<int> _pid;
      bool _noCentrality = false;
      bool _noAssoc = false;
    public:

      /// Constructor
      Correlator(int index0, int index1, int index2) {
        _indices = {index0, index1, index2};
      }

			Correlator(int index0, int index1) {
        _indices = {index0, index1};
      }

			Correlator(int index0) {
        _indices = {index0};
      }

			Correlator(std::vector<int> vindex) {
        _indices = vindex;
      }

      void SetCollSystemAndEnergy(string s){ _collSystemAndEnergy = s; }
      void SetCentrality(double cmin, double cmax){ _centrality = make_pair(cmin, cmax); }
      void SetNoCentrality(){ _noCentrality = true; }
      void SetNoAssoc(){ _noAssoc = true; }
      void SetTriggerRange(double tmin, double tmax){ _triggerRange = make_pair(tmin, tmax); }
      void SetAssociatedRange(double amin, double amax){ _associatedRange = make_pair(amin, amax); }
      void SetPID(std::initializer_list<int> pid){ _pid = pid; }

      string GetCollSystemAndEnergy(){ return _collSystemAndEnergy; }
      pair<double,double> GetCentrality(){ return _centrality; }
      double GetCentralityMin(){ return _centrality.first; }
      double GetCentralityMax(){ return _centrality.second; }
      pair<double,double> GetTriggerRange(){ return _triggerRange; }
      double GetTriggerRangeMin(){ return _triggerRange.first; }
      double GetTriggerRangeMax(){ return _triggerRange.second; }
      pair<double,double> GetAssociatedRange(){ return _associatedRange; }
      double GetAssociatedRangeMin(){ return _associatedRange.first; }
      double GetAssociatedRangeMax(){ return _associatedRange.second; }
      vector<int> GetPID(){ return _pid; }

      int GetIndex(int i){ return _indices[i]; }
      string GetFullIndex()
      {
          string fullIndex = to_string(GetIndex()) + to_string(GetSubIndex())+ to_string(GetSubIndex());
          return fullIndex;
      }

      bool CheckCollSystemAndEnergy(string s){ return _collSystemAndEnergy.compare(s) == 0 ? true : false; }
     bool CheckCentrality(double cent){ return ((cent>_centrality.first && cent<_centrality.second) || _noCentrality == true) ? true : false; }
      bool CheckTriggerRange(double tpt){ return (tpt>_triggerRange.first && tpt<_triggerRange.second) ? true : false; }
      bool CheckAssociatedRange(double apt){ return ((apt>_associatedRange.first && apt<_associatedRange.second) || _noAssoc == true) ? true : false; }
      bool CheckAssociatedRangeMaxTrigger(double apt, double tpt){ return (apt>_associatedRange.first && apt<tpt) ? true : false; }
      bool CheckPID(std::initializer_list<int> pid)
      {

          bool inList = false;

          for(int id : pid)
          {
              auto it = std::find(_pid.begin(), _pid.end(), id);

              if(it != _pid.end())
              {
                  inList = true;
                  break;
              }
          }

          return inList;

      }

      bool CheckConditions(string s, double cent, double tpt, double apt)
      {
        if(!CheckConditions(s, cent, tpt)) return false;
        if(!CheckAssociatedRange(apt)) return false;

        return true;

      }

      bool CheckConditions(string s, double cent, double tpt)
      {
        if(!CheckConditions(s, cent)) return false;
        if(!CheckTriggerRange(tpt)) return false;

        return true;

      }

      bool CheckConditions(string s, double cent)
      {
        if(!CheckCollSystemAndEnergy(s)) return false;
        if(!CheckCentrality(cent)) return false;

        return true;

      }

      bool CheckConditionsMaxTrigger(string s, double cent, double tpt, double apt)
      {
        if(!CheckCollSystemAndEnergy(s)) return false;
        if(!CheckCentrality(cent)) return false;
        if(!CheckTriggerRange(tpt)) return false;
        if(!CheckAssociatedRangeMaxTrigger(apt,tpt)) return false;

        return true;

      }
     };

  /// @brief Add a short analysis description here
  class CorrelatorExample : public Analysis {
  public:

    /// Constructor
    DEFAULT_RIVET_ANALYSIS_CTOR(CorrelatorExample);

    Histo1DPtr SubtractBackgroundZYAM(Histo1DPtr histo)
    {

        YODA::Histo1D hist = *histo;

        double minValue = sqrt(-2);
        double binWidth = 0.;
        int minValueEntries = 0.;

        for(auto &bin : hist.bins())
        {
            if(std::isnan(minValue))
            {
                minValue = bin.sumW();
                binWidth = bin.width();
                minValueEntries = bin.numEntries();
            }
            if(bin.sumW()/bin.width() < minValue/binWidth)
            {
                minValue = bin.sumW();
                binWidth = bin.width();
                minValueEntries = bin.numEntries();
            }
        }

        hist.reset();

        for(auto &bin : hist.bins())
        {
            bin.fillBin((minValue*bin.width())/(minValueEntries*binWidth), minValueEntries);
        }

        *histo = YODA::subtract(*histo, hist);

        return histo;

    }


    double getYieldRangeUser(Histo1DPtr histo, double xmin, double xmax, double &fraction)
    {
        //This will include bins partially covered by the user range

        YODA::Histo1D hist = *histo;

        double integral = 0.;

        if(xmax < xmin) throw RangeError("Error: xmin > xmax");
        if(xmin < hist.bin(0).xMin()) throw RangeError("xmin is out of range");
        if(xmax > hist.bin(hist.numBins()-1).xMax()) throw RangeError("xmax is out of range");

        for(auto &bin : hist.bins())
        {
            if((bin.xMin() > xmin) && (bin.xMax() < xmax))
            {
                integral += bin.sumW();
                fraction += bin.numEntries();
            }
            else if((bin.xMin() < xmin) && (bin.xMax() > xmin))
            {
                double perc = bin.xMax() - xmin;
                integral += perc*bin.sumW();
                fraction += perc*bin.numEntries();

            }
            else if((bin.xMin() < xmax) && (bin.xMax() > xmax))
            {
                double perc = xmax - bin.xMin();
                integral += perc*bin.sumW();
                fraction += perc*bin.numEntries();
            }
        }

        return integral;

    }





    bool isSameParticle(const Particle& p1, const Particle& p2)
      {
        //if pT, eta and phi are equal, they are the same particle
        if(p1.pt() != p2.pt()) return false;
        if(p1.eta() != p2.eta()) return false;
        if(p1.phi() != p2.phi()) return false;

        return true;
      }


      bool isSecondary(Particle p)
      {
        //return true if is secondary
        if (( p.hasAncestor(310) || p.hasAncestor(-310)  ||     // K0s
          p.hasAncestor(130)  || p.hasAncestor(-130)  ||     // K0l
          p.hasAncestor(3322) || p.hasAncestor(-3322) ||     // Xi0
          p.hasAncestor(3122) || p.hasAncestor(-3122) ||     // Lambda
          p.hasAncestor(3222) || p.hasAncestor(-3222) ||     // Sigma+/-
          p.hasAncestor(3312) || p.hasAncestor(-3312) ||     // Xi-/+
          p.hasAncestor(3334) || p.hasAncestor(-3334) ))    // Omega-/+
          return true;
        else return false;

      }
    double CalculateVn(YODA::Histo1D& hist, int nth)
    {
        int nBins = hist.numBins();
        double integral = 0.;

        double Vn = 0.;
        for (int i = 0; i < nBins; i++)
        {
            integral += hist.bin(i).sumW();
            Vn += hist.bin(i).sumW()*cos(nth*hist.bin(i).xMid());
        }
        Vn /= integral;
        return Vn;
    }

    double GetDeltaPhi(Particle pAssoc, Particle pTrig)
    {
        //https://rivet.hepforge.org/code/dev/structRivet_1_1DeltaPhiInRange.html
        double dPhi = deltaPhi(pTrig, pAssoc, true);//this does NOT rotate the delta phi to be in a given range

        if(dPhi < -M_PI/2.)
        {
            dPhi += 2.*M_PI;
        }
        else if(dPhi > 3.*M_PI/2.)
        {
            dPhi -= 2*M_PI;
        }

        return dPhi;
      }

    int FindBinAtMinimum(YODA::Histo1D& hist, double bmin, double bmax)
    {
        int minBin = -1;
        double minVal = 999.;

        for(unsigned int i = 0; i < hist.numBins(); i++)
        {
            if(hist.bin(i).xMid() < bmin || hist.bin(i).xMid() > bmax) continue;
            if( (hist.bin(i).sumW()/hist.bin(i).xWidth()) < minVal )
            {
                minVal = hist.bin(i).sumW()/hist.bin(i).xWidth();
                minBin = i;
            }
        }

        return minBin;

    }

    void SubtractBackground(YODA::Histo1D& fullHist, YODA::Histo1D& hist, vector<int> n, double bmin, double bmax)
    {
        vector<double> Vn(n.size(), 0);
        for(unsigned int i = 0; i < n.size(); i++)
        {
            Vn[i] = CalculateVn(fullHist, n[i]);
        }

        double bmod = 1.;
        int minBin = FindBinAtMinimum(fullHist, bmin, bmax);

        for(unsigned int i = 0; i < Vn.size(); i++)
        {
            bmod += 2*Vn[i]*cos(n[i]*fullHist.bin(minBin).xMid());
        }

        double b = (fullHist.bin(minBin).sumW()/fullHist.bin(minBin).xWidth())/bmod; //Divided by bin width in order to generalize it and enable it to be used for histograms with different binning

        for(unsigned int ibin = 0; ibin < hist.numBins(); ibin++)
        {
            double modulation = 1;
            for(unsigned int i = 0; i < Vn.size(); i++)
            {
                modulation += 2*Vn[i]*cos(n[i]*hist.bin(ibin).xMid());
            }
            modulation *= b;
            hist.bin(ibin).scaleW(1 - (modulation/(hist.bin(ibin).sumW()/hist.bin(ibin).xWidth()))); //Divided by bin width to compensate the calculation of "b"
        }

    }

    void init() {

        const ChargedFinalState cfs(Cuts::abseta < 0.35 && Cuts::abscharge > 0);
        declare(cfs, "CFS");

        declareCentrality(RHICCentrality("PHENIX"), "RHIC_2019_CentralityCalibration:exp=PHENIX", "CMULT", "CMULT");







    }
    void analyze(const Event& event) {
      const ChargedFinalState& cfs = apply<ChargedFinalState>(event, "CFS");

      string SysAndEnergy = "";
      //string cmsEnergy = "200GeV";

      if(collSys == pp0) SysAndEnergy = "pp200GeV";
      else if(collSys == AuAu) SysAndEnergy = "AuAu200GeV";

      //SysAndEnergy = collSys + cmsEnergy;

      double triggerptMin = 999.;
      double triggerptMax = -999.;
      double associatedptMin = 999.;
      double associatedptMax = -999.;

      bool isVeto = true;

      const CentralityProjection& cent = apply<CentralityProjection>(event, "CMULT");
      const double c = cent();


    }

    void finalize() {

      bool AuAu200_available = false;
      bool pp_available = false;














    }

 	map<string, Histo1DPtr> _h;
 	map<string, Profile1DPtr> _p;
    map<string, CounterPtr> sow;
    map<string, Histo1DPtr> _DeltaPhixE;
    map<int, Histo1DPtr> _DeltaPhiSub;
    map<string, int> nTriggers;
    vector<Correlator> Correlators;
    vector<Correlator> Correlators38;
    vector<Correlator> Correlators31;
    vector<Correlator> Correlators30;
    vector<Correlator> Correlators26;
    vector<Correlator> Correlators25;
    vector<Correlator> Correlators24;
    vector<Correlator> Correlators23;
    vector<Correlator> Correlators18;
    vector<Correlator> Correlators17;
    vector<Correlator> Correlators16;
    vector<Correlator> Correlators12;
    vector<Correlator> Correlators12corr;
    vector<Correlator> Correlators11;
    vector<Correlator> Correlators10;
    vector<Correlator> Correlators9;
    vector<Correlator> Correlators8;
    vector<Correlator> Correlators7;
    vector<Correlator> Correlators6;
    vector<Correlator> Correlators4;

    std::initializer_list<int> pdgPi0 = {111, -111};  // Pion 0
    std::initializer_list<int> pdgPhoton = {22};  // Pion 0

    string beamOpt;
    enum CollisionSystem {pp0, AuAu};
    CollisionSystem collSys;
  };


  DECLARE_RIVET_PLUGIN(PHENIX_2008_I778396);

}
