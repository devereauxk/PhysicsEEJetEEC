// ./matchingEffCorr.exe --Input v0/LEP1MC1994_recons_aftercut-001_Matched.root --Matched PairTree --Unmatched UnmatchedPairTree
#include <iostream>
#include <vector>
#include <map>
#include <filesystem>
using namespace std;

// root includes
#include "TTree.h"
#include "TChain.h"
#include "TFile.h"
#include "TTreeReader.h"
#include "TTreeReaderValue.h"
#include "TTreeReaderArray.h"
#include "TStyle.h"
#include "TH2D.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TGaxis.h"
#include "TLatex.h"
#include "TLegend.h"

#include "Messenger.h"
#include "CommandLine.h"
#include "Matching.h"
#include "ProgressBar.h"
#include "TauHelperFunctions3.h"
#include "SetStyle.h"
#include "EffCorrFactor.h"

int main(int argc, char *argv[]);
int FindBin(double Value, int NBins, double Bins[]); 
void MakeCanvasZ(vector<TH1D>& Histograms, vector<string> Labels, string Output, string X, string Y, double WorldMin, double WorldMax, bool DoRatio, bool LogX); 
void MakeCanvas(vector<TH1D>& Histograms, vector<string> Labels, string Output, string X, string Y, double WorldMin, double WorldMax, bool DoRatio, bool LogX); 
void SetPad(TPad &P); 
void DivideByBin(TH1D &H, double Bins[]); 

// can we put this in the CommonCode/ ?
void FillChain(TChain &chain, const vector<string> &files) {
  for (auto file : files) {
    chain.Add(file.c_str());
  }
}

int main(int argc, char *argv[])
{
   CommandLine CL(argc, argv);

   SetThesisStyle();
   static vector<int> Colors = GetCVDColors6();

   string InputFileName          = CL.Get("Input");
   string MatchingEffFileName    = CL.Get("MatchingEffName", "MatchingEff.root");
   string MatchingEffArgName     = CL.Get("MatchingEffArgName", "z");
   bool MakeMatchingEffCorrFactor= CL.GetBool("MakeMatchingEffCorrFactor", false);
   string MatchedTreeName        = CL.Get("Matched", "PairTree");
   string UnmatchedTreeName      = CL.Get("Unmatched", "UnmatchedPairTree");
   TFile InputFile(InputFileName.c_str());

   double TotalE = 91.1876;

   TChain MatchedTreeChain(MatchedTreeName.c_str());
   FillChain(MatchedTreeChain, {InputFileName});
   TTreeReader MatchedReader(&MatchedTreeChain);

   TTreeReaderValue<int> Matched_NPair(   MatchedReader, "NPair");
   // TTreeReaderArray<double> Matched_GenE1(      MatchedReader, "GenE1");
   // TTreeReaderArray<double> Matched_GenX1(      MatchedReader, "GenX1");
   // TTreeReaderArray<double> Matched_GenY1(      MatchedReader, "GenY1");
   // TTreeReaderArray<double> Matched_GenZ1(      MatchedReader, "GenZ1");
   // TTreeReaderArray<double> Matched_GenE2(      MatchedReader, "GenE2");
   // TTreeReaderArray<double> Matched_GenX2(      MatchedReader, "GenX2");
   // TTreeReaderArray<double> Matched_GenY2(      MatchedReader, "GenY2");
   // TTreeReaderArray<double> Matched_GenZ2(      MatchedReader, "GenZ2");
   // TTreeReaderArray<double> Matched_Distance1(  MatchedReader, "Distance1");
   // TTreeReaderArray<double> Matched_Distance2(  MatchedReader, "Distance2");
   TTreeReaderArray<double> Matched_DistanceGen(MatchedReader, "DistanceGen");
   TTreeReaderArray<double> Matched_E1E2Gen(    MatchedReader, "E1E2Gen");


   TChain UnmatchedTreeChain(UnmatchedTreeName.c_str());
   FillChain(UnmatchedTreeChain, {InputFileName});
   TTreeReader UnmatchedReader(&UnmatchedTreeChain);

   TTreeReaderValue<int> NUnmatchedPair(          UnmatchedReader, "NUnmatchedPair");
   TTreeReaderArray<double> DistanceUnmatchedGen( UnmatchedReader, "DistanceUnmatchedGen");
   TTreeReaderArray<double> E1E2GenUnmatched(     UnmatchedReader, "E1E2GenUnmatched");

   //------------------------------------
   // define the binning
   //------------------------------------

   // theta binning
   const int BinCount = 100;
   double Bins[2*BinCount+1];
   double BinMin = 0.002;
   double BinMax = M_PI / 2;

   // z binning
   double zBins[2*BinCount+1];
   double zBinMin = (1- cos(0.002))/2;
   double zBinMax = 0.5;

   // energy binning
   const int EnergyBinCount = 10; 
   double EnergyBins[EnergyBinCount+1];
   double EnergyBinMin = 4e-6;
   double EnergyBinMax = 0.2;
   double logMin = std::log10(EnergyBinMin);
   double logMax = std::log10(EnergyBinMax);
   double logStep = (logMax - logMin) / (EnergyBinCount);

   for(int i = 0; i <= BinCount; i++){
      // theta double log binning
      Bins[i] = exp(log(BinMin) + (log(BinMax) - log(BinMin)) / BinCount * i);
      Bins[2*BinCount-i] = BinMax * 2 - exp(log(BinMin) + (log(BinMax) - log(BinMin)) / BinCount * i);

      // z double log binning
      zBins[i] = exp(log(zBinMin) + (log(zBinMax) - log(zBinMin)) / BinCount * i);
      zBins[2*BinCount-i] = zBinMax * 2 - exp(log(zBinMin) + (log(zBinMax) - log(zBinMin)) / BinCount * i);
    
   }

   EnergyBins[0] = 0;
   for(int e = 1; e <= EnergyBinCount; e++){
      double logValue = logMin + e * logStep;
      EnergyBins[e] =  std::pow(10, logValue);
   }

   // -------------------------------------------
   // allocate the histograms
   // -------------------------------------------

   // 2D histograms
   TH2D h2_BeforeMatching_Theta("h2_BeforeMatching_Theta", "h2_BeforeMatching_Theta", 2 * BinCount, 0, 2 * BinCount,  EnergyBinCount, EnergyBins);
   TH2D h2_BeforeMatching_Z("h2_BeforeMatching_Z", "h2_BeforeMatching_Z", 2 * BinCount, 0, 2 * BinCount,  EnergyBinCount, EnergyBins);
   TH2D h2_Matching_Theta("h2_Matching_Theta", "h2_Matching_Theta", 2 * BinCount, 0, 2 * BinCount,  EnergyBinCount, EnergyBins);
   TH2D h2_Matching_Z("h2_Matching_Z", "h2_Matching_Z", 2 * BinCount, 0, 2 * BinCount,  EnergyBinCount, EnergyBins);

   // 1D histograms
   TH1D h1_Matching_Z("h1_Matching_Z", "h1_Matching_Z", 2 * BinCount, 0, 2 * BinCount); 
   TH1D h1_BeforeMatching_Z("h1_BeforeMatching_Z", "h1_BeforeMatching_Z", 2 * BinCount, 0, 2 * BinCount); 
   TH1D h1_Matching_Theta("h1_Matching_Theta", "h1_Matching_Theta", 2 * BinCount, 0, 2 * BinCount); 
   TH1D h1_BeforeMatching_Theta("h1_BeforeMatching_Theta", "h1_BeforeMatching_Theta", 2 * BinCount, 0, 2 * BinCount); 

   // corrected 1D histograms
   TH1D h1_MatchingCorrected_Z("h1_MatchingCorrected_Z", "h1_MatchingCorrected_Z", 2 * BinCount, 0, 2 * BinCount); 
   TH1D h1_MatchingCorrected_Theta("h1_MatchingCorrected_Theta", "h1_MatchingCorrected_Theta", 2 * BinCount, 0, 2 * BinCount); 

   //------------------------------------
   // define the matching efficiency correction factor
   //------------------------------------
   EffCorrFactor matchingEffCorrFactor;
   if (!filesystem::exists(MatchingEffFileName.c_str())) MakeMatchingEffCorrFactor = true;
   
   if (MakeMatchingEffCorrFactor)
   {
      printf("[INFO] produce matching efficiency correction factor (%s), MatchingEffArgName=%s\n", MatchingEffFileName.c_str(), MatchingEffArgName.c_str());
   } else {
      printf("[INFO] applying matching efficiency correction factor (%s), MatchingEffArgName=%s\n", MatchingEffFileName.c_str(), MatchingEffArgName.c_str());
      matchingEffCorrFactor.init(MatchingEffFileName.c_str(), MatchingEffArgName.c_str());
   }

   // -------------------------------------
   // loop over the tree after gen-matching
   // -------------------------------------
   int EntryCount = MatchedReader.GetEntries(true);
   for(int iE = 0; iE < EntryCount; iE++)
   {
      MatchedReader.Next();

      // calculate and fill the EECs
      for (int iPair = 0; iPair < *Matched_NPair; iPair++) 
      {
         // get the proper bins
         int BinThetaGen  = FindBin(Matched_DistanceGen[iPair], 2 * BinCount, Bins);
         int BinEnergyGen = FindBin(Matched_E1E2Gen[iPair], EnergyBinCount, EnergyBins);
         double zGen = (1-cos(Matched_DistanceGen[iPair]))/2; 
         int BinZGen = FindBin(zGen, 2*BinCount, zBins); 

         // calculate the EEC
         double EEC =  Matched_E1E2Gen[iPair]; 
         
         // fill the histograms
         h2_Matching_Theta.Fill(BinThetaGen, BinEnergyGen, EEC); 
         h2_Matching_Z.Fill(BinZGen, BinEnergyGen, EEC); 
         h1_Matching_Z.Fill(BinZGen, EEC); 
         h1_Matching_Theta.Fill(BinThetaGen, EEC);
         if (!MakeMatchingEffCorrFactor)
         {
            double efficiency = matchingEffCorrFactor.efficiency((MatchingEffArgName=="z")? BinZGen: BinThetaGen,
                                                                 BinEnergyGen);
            // printf("argBin: %d, z: %.3f, eff: %.3f \n", BinZGen, zGen, efficiency);
            // printf("argBin: %d, normEEBin:%d, z: %.3f, normEE: %.3f, eff: %.3f \n", BinZGen, BinEnergyGen, zGen, EEC, efficiency);
            h1_MatchingCorrected_Z.Fill(BinZGen, EEC/efficiency); 
            h1_MatchingCorrected_Theta.Fill(BinThetaGen, EEC/efficiency);
         }
      }
   } // end loop over the number of events'

   // -------------------------------------
   // loop over the tree before gen-matching
   // -------------------------------------
   int EntryCountBefore = UnmatchedReader.GetEntries(true);
   for(int iE = 0; iE < EntryCountBefore; iE++)
   {
      UnmatchedReader.Next();

      // calculate and fill the EECs
      for (int iPair = 0; iPair < *NUnmatchedPair; iPair++) 
      {
         // get the proper bins
         int BinThetaGen  = FindBin(DistanceUnmatchedGen[iPair], 2 * BinCount, Bins);
         int BinEnergyGen = FindBin(E1E2GenUnmatched[iPair]/(TotalE*TotalE), EnergyBinCount, EnergyBins);
         double zGen = (1-cos(DistanceUnmatchedGen[iPair]))/2; 
         int BinZGen = FindBin(zGen, 2*BinCount, zBins); 

         // calculate the EEC
         double EEC =  E1E2GenUnmatched[iPair]/(TotalE*TotalE); 
         
         // fill the histograms
         h2_BeforeMatching_Theta.Fill(BinThetaGen, BinEnergyGen, EEC); 
         h2_BeforeMatching_Z.Fill(BinZGen, BinEnergyGen, EEC); 
         h1_BeforeMatching_Z.Fill(BinZGen, EEC); 
         h1_BeforeMatching_Theta.Fill(BinThetaGen, EEC);
      }
   } // end loop over the number of events
   // EEC is per-event so scale by the event number
   printf( "h1_Matching_Z.GetEntries(): %.3f, EntryCount: %d, h1_BeforeMatching_Z.GetEntries(): %.3f, EntryCountBefore: %d\n", 
            h1_Matching_Z.GetEntries(), EntryCount,
            h1_BeforeMatching_Z.GetEntries(), EntryCountBefore );
   h1_Matching_Z.Scale(1.0/EntryCount); 
   h1_BeforeMatching_Z.Scale(1.0/EntryCountBefore);
   h1_Matching_Theta.Scale(1.0/EntryCount);
   h1_BeforeMatching_Theta.Scale(1.0/EntryCountBefore);
   h2_Matching_Z.Scale(1.0/EntryCount); 
   h2_BeforeMatching_Z.Scale(1.0/EntryCountBefore);
   h2_Matching_Theta.Scale(1.0/EntryCount);
   h2_BeforeMatching_Theta.Scale(1.0/EntryCountBefore);
   if (!MakeMatchingEffCorrFactor)
   {
      h1_MatchingCorrected_Z.Scale(1.0/EntryCount);
      h1_MatchingCorrected_Theta.Scale(1.0/EntryCount);
   }

   // divide by the bin width
   DivideByBin(h1_Matching_Z, zBins);
   DivideByBin(h1_BeforeMatching_Z, zBins);
   DivideByBin(h1_Matching_Theta, Bins);
   DivideByBin(h1_BeforeMatching_Theta, Bins);
   if (!MakeMatchingEffCorrFactor)
   {
      DivideByBin(h1_MatchingCorrected_Z,zBins);
      DivideByBin(h1_MatchingCorrected_Theta,Bins);
   }

   // set the style for the plots
   h1_Matching_Z.SetMarkerColor(Colors[2]);
   h1_BeforeMatching_Z.SetMarkerColor(Colors[3]);
   h1_Matching_Theta.SetMarkerColor(Colors[2]);
   h1_BeforeMatching_Theta.SetMarkerColor(Colors[3]);
   h1_Matching_Z.SetLineColor(Colors[2]);
   h1_BeforeMatching_Z.SetLineColor(Colors[3]);
   h1_Matching_Theta.SetLineColor(Colors[2]);
   h1_BeforeMatching_Theta.SetLineColor(Colors[3]);
   h1_Matching_Z.SetMarkerStyle(20);
   h1_BeforeMatching_Z.SetMarkerStyle(20);
   h1_Matching_Theta.SetMarkerStyle(20);
   h1_BeforeMatching_Theta.SetMarkerStyle(20);
   h1_Matching_Z.SetLineWidth(2);
   h1_BeforeMatching_Z.SetLineWidth(2);
   h1_Matching_Theta.SetLineWidth(2);
   h1_BeforeMatching_Theta.SetLineWidth(2);
   if (!MakeMatchingEffCorrFactor)
   {
      h1_MatchingCorrected_Z.SetMarkerColor(Colors[4]);
      h1_MatchingCorrected_Z.SetLineColor(Colors[4]);
      h1_MatchingCorrected_Z.SetMarkerStyle(20);
      h1_MatchingCorrected_Z.SetLineWidth(2);
      h1_MatchingCorrected_Theta.SetMarkerColor(Colors[4]);
      h1_MatchingCorrected_Theta.SetLineColor(Colors[4]);
      h1_MatchingCorrected_Theta.SetMarkerStyle(20);
      h1_MatchingCorrected_Theta.SetLineWidth(2);

   }

   // Matching is given by SelectedEvents/TotalEvents
   if (!MakeMatchingEffCorrFactor)
   {
      std::vector<TH1D> hists = {h1_BeforeMatching_Z, h1_Matching_Z, h1_MatchingCorrected_Z}; 
      std::vector<TH1D> hists_theta = {h1_BeforeMatching_Theta, h1_Matching_Theta, h1_MatchingCorrected_Theta};

      string plotPrefix(MatchingEffFileName);
      plotPrefix.replace(plotPrefix.find(".root"), 5, "");
      MakeCanvasZ(hists, {"Before Matching", "After Matching", "Matching Corrected"}, 
                  Form("%s_Closure", plotPrefix.c_str()), "#it{z} = (1- cos(#theta))/2", "#frac{1}{#it{N}_{event}}#frac{d(Sum E_{i}E_{j}/E^{2})}{d#it{z}}",1e-3,1e3, true, true);
      MakeCanvas(hists_theta, {"Before Matching", "After Matching", "Matching Corrected"},
                  Form("%s_Theta_Closure", plotPrefix.c_str()),"#theta", "#frac{1}{#it{N}_{event}}#frac{d(Sum E_{i}E_{j}/E^{2})}{d#it{#theta}}",1e-3, 2e0, true, true);
   } 
   else 
   {
      std::vector<TH1D> hists = {h1_BeforeMatching_Z, h1_Matching_Z}; 
      std::vector<TH1D> hists_theta = {h1_BeforeMatching_Theta, h1_Matching_Theta};

      string plotPrefix(MatchingEffFileName);
      plotPrefix.replace(plotPrefix.find(".root"), 5, "");
      MakeCanvasZ(hists, {"Before Matching", "After Matching"}, 
                  Form("%s_Matching", plotPrefix.c_str()), "#it{z} = (1- cos(#theta))/2", "#frac{1}{#it{N}_{event}}#frac{d(Sum E_{i}E_{j}/E^{2})}{d#it{z}}",1e-3,1e3, true, true);
      MakeCanvas(hists_theta, {"Before Matching", "After Matching"},
                  Form("%s_Matching_Theta", plotPrefix.c_str()),"#theta", "#frac{1}{#it{N}_{event}}#frac{d(Sum E_{i}E_{j}/E^{2})}{d#it{#theta}}",1e-3, 2e0, true, true);
   }

   // now handle the 2D plots

   // write to the output file
   if (MakeMatchingEffCorrFactor)
   {
      TFile OutputFile(MatchingEffFileName.c_str(), "RECREATE");
      matchingEffCorrFactor.write(OutputFile, "theta", &h1_Matching_Theta, &h1_BeforeMatching_Theta);
      matchingEffCorrFactor.write(OutputFile, "z", &h1_Matching_Z, &h1_BeforeMatching_Z);
      matchingEffCorrFactor.write(OutputFile, "theta", &h2_Matching_Theta, &h2_BeforeMatching_Theta);
      matchingEffCorrFactor.write(OutputFile, "z", &h2_Matching_Z, &h2_BeforeMatching_Z);

      OutputFile.Close();
   }
   // cleanup
   InputFile.Close();

   return 0;
}

int FindBin(double Value, int NBins, double Bins[])
{
   for(int i = 0; i < NBins; i++)
      if(Value < Bins[i])
         return i - 1;
   return NBins;
}

void MakeCanvasZ(vector<TH1D>& Histograms, vector<string> Labels, string Output,
   string X, string Y, double WorldMin, double WorldMax, bool DoRatio, bool LogX){
   

   int NLine = Histograms.size();
   int N = Histograms[0].GetNbinsX();

   double MarginL = 180;
   double MarginR = 90;
   double MarginB = 120;
   double MarginT = 90;

   double WorldXMin = LogX ? 0 : 0;
   double WorldXMax = LogX ? N : 1;
   
   double PadWidth = 1200;
   double PadHeight = DoRatio ? 640 : 640 + 240;
   double PadRHeight = DoRatio ? 240 : 0.001;

   double CanvasWidth = MarginL + PadWidth + MarginR;
   double CanvasHeight = MarginT + PadHeight + PadRHeight + MarginB;

   MarginL = MarginL / CanvasWidth;
   MarginR = MarginR / CanvasWidth;
   MarginT = MarginT / CanvasHeight;
   MarginB = MarginB / CanvasHeight;

   PadWidth   = PadWidth / CanvasWidth;
   PadHeight  = PadHeight / CanvasHeight;
   PadRHeight = PadRHeight / CanvasHeight;

   TCanvas Canvas("Canvas", "", CanvasWidth, CanvasHeight);
   Canvas.cd(); 

   TPad Pad("Pad", "", MarginL, MarginB + PadRHeight, MarginL + PadWidth, MarginB + PadHeight + PadRHeight);
   Pad.SetLogy();
   SetPad(Pad);
   
   TPad PadR("PadR", "", MarginL, MarginB, MarginL + PadWidth, MarginB + PadRHeight);
   if(DoRatio)
      SetPad(PadR);

   Pad.cd();

   TH2D HWorld("HWorld", "", N, WorldXMin, WorldXMax, 100, WorldMin, WorldMax);
   HWorld.SetStats(0);
   HWorld.GetXaxis()->SetTickLength(0);
   HWorld.GetXaxis()->SetLabelSize(0);

   HWorld.Draw("axis");
   for(TH1D H : Histograms){
      TH1D *HClone = (TH1D *)H.Clone();
      HClone->Draw("exp same");
   }

   TGraph G;
   G.SetPoint(0, LogX ? N / 2 : 1 / 2, 0);
   G.SetPoint(1, LogX ? N / 2 : 1/ 2, 10000);
   G.SetLineStyle(kDashed);
   G.SetLineColor(kGray);
   G.SetLineWidth(1);
   G.Draw("l");

   if(DoRatio)
      PadR.cd();

   double WorldRMin = 0.5;
   double WorldRMax = 1.5;
   
   TH2D HWorldR("HWorldR", "", N, WorldXMin, WorldXMax, 100, WorldRMin, WorldRMax);
   TGraph G2;
   
   if(DoRatio)
   {
      HWorldR.SetStats(0);
      HWorldR.GetXaxis()->SetTickLength(0);
      HWorldR.GetXaxis()->SetLabelSize(0);
      HWorldR.GetYaxis()->SetNdivisions(505);

      HWorldR.Draw("axis");
      for(int i = 1; i < NLine; i++)
      {
         TH1D *H = (TH1D *)Histograms[i].Clone();
         H->Divide(&Histograms[0]);
         H->Draw("same");
      }

      G.Draw("l");

      G2.SetPoint(0, 0, 1);
      G2.SetPoint(1, 99999, 1);
      G2.Draw("l");
   }
   
   double BinMin    = (1- cos(0.002))/2;
   double BinMiddle = 0.5;
   double BinMax    = 1 - BinMin;

   Canvas.cd();
   std::cout << "Here 1" << std::endl;
   int nDiv = 505;
   std::cout << "Bin Min " << BinMin << " Bin Middle " << BinMiddle << std::endl;
   TGaxis X1(MarginL, MarginB, MarginL + PadWidth / 2, MarginB, BinMin, BinMiddle, nDiv, "GS");
   TGaxis X2(MarginL + PadWidth, MarginB, MarginL + PadWidth / 2, MarginB, BinMin, BinMiddle, nDiv, "-GS");
   TGaxis X3(MarginL, MarginB + PadRHeight, MarginL + PadWidth / 2, MarginB + PadRHeight, BinMin, BinMiddle, nDiv, "+-GS");
   TGaxis X4(MarginL + PadWidth, MarginB + PadRHeight, MarginL + PadWidth / 2, MarginB + PadRHeight, BinMin, BinMiddle, nDiv, "+-GS");

   std::cout << "Here 2" << std::endl;
   TGaxis Y1(MarginL, MarginB, MarginL, MarginB + PadRHeight, WorldRMin, WorldRMax, 505, "");
   TGaxis Y2(MarginL, MarginB + PadRHeight, MarginL, MarginB + PadRHeight + PadHeight, WorldMin, WorldMax, 510, "G");

   std::cout << "Here 3" << std::endl;
   TGaxis XL1(MarginL, MarginB, MarginL + PadWidth, MarginB,  (1- cos(0.002))/2, 1, 210, "S");
   TGaxis XL2(MarginL, MarginB + PadRHeight, MarginL + PadWidth, MarginB + PadRHeight,  (1- cos(0.002))/2, 1, 210, "+-S");

   Y1.SetLabelFont(42);
   Y2.SetLabelFont(42);
   XL1.SetLabelFont(42);
   XL2.SetLabelFont(42);

   X1.SetLabelSize(0);
   X2.SetLabelSize(0);
   X3.SetLabelSize(0);
   X4.SetLabelSize(0);
   // XL1.SetLabelSize(0);
   XL2.SetLabelSize(0);

   X1.SetTickSize(0.06);
   X2.SetTickSize(0.06);
   X3.SetTickSize(0.06);
   X4.SetTickSize(0.06);
   XL1.SetTickSize(0.03);
   XL2.SetTickSize(0.03);

   if(LogX == true)
   {
      X1.Draw();
      X2.Draw();
      if(DoRatio) X3.Draw();
      if(DoRatio) X4.Draw();
   }
   if(LogX == false)
   {
      XL1.Draw();
      if(DoRatio)
         XL2.Draw();
   }
   if(DoRatio)
      Y1.Draw();
   Y2.Draw();

   TLatex Latex;
   Latex.SetNDC();
   Latex.SetTextFont(42);
   Latex.SetTextSize(0.035);
   Latex.SetTextAlign(23);
   if(LogX) Latex.DrawLatex(MarginL + (1 - MarginR - MarginL) * 0.02, MarginB - 0.01, "10^{-6} ");
   if(LogX) Latex.DrawLatex(MarginL + (1 - MarginR - MarginL) * 0.180, MarginB - 0.01, "10^{-4} ");
   if(LogX) Latex.DrawLatex(MarginL + (1 - MarginR - MarginL) * 0.365, MarginB - 0.01, "10^{-2} ");
   if(LogX) Latex.DrawLatex(MarginL + (1 - MarginR - MarginL) * 0.500, MarginB - 0.01, "1/2");
   if(LogX) Latex.DrawLatex(MarginL + (1 - MarginR - MarginL) * 0.650, MarginB - 0.01, "1 - 10^{-2}");
   if(LogX) Latex.DrawLatex(MarginL + (1 - MarginR - MarginL) * 0.823, MarginB - 0.01, "1 - 10^{-4}");
   if(LogX) Latex.DrawLatex(MarginL + (1 - MarginR - MarginL) * 0.995, MarginB - 0.01, "1 - 10^{-6}");

   Latex.SetTextAlign(12);
   Latex.SetTextAngle(270);
   Latex.SetTextColor(kGray);
   Latex.DrawLatex(MarginL + (1 - MarginR - MarginL) * 0.5 + 0.0175, 1 - MarginT - 0.015, "#it{z} = 1/2");

   Latex.SetTextAlign(22);
   Latex.SetTextAngle(0);
   Latex.SetTextColor(kBlack);
   Latex.DrawLatex(MarginL + PadWidth * 0.9, MarginB * 0.4, X.c_str());

   Latex.SetTextAlign(22);
   Latex.SetTextAngle(90);
   Latex.SetTextColor(kBlack);
   if(DoRatio)
      Latex.DrawLatex(MarginL * 0.3, MarginB + PadRHeight * 0.5, "Matching Eff.");
   Latex.DrawLatex(MarginL * 0.3, MarginB + PadRHeight + PadHeight * 0.5, Y.c_str());

   Latex.SetTextAlign(11);
   Latex.SetTextAngle(0);
   Latex.DrawLatex(MarginL, MarginB + PadRHeight + PadHeight + 0.012, "ALEPH e^{+}e^{-}, #sqrt{s} = 91.2 GeV, Work-in-progress");

   Latex.SetTextAlign(11);
   Latex.SetTextAngle(0);
   Latex.SetTextColor(19);
   Latex.SetTextSize(0.02);
   Latex.DrawLatex(0.01, 0.01, "Work-in-progress, 2024 August 7th HB");

   TLegend Legend(0.15, 0.90, 0.35, 0.90 - 0.04 * min(NLine, 4));
   Legend.SetTextFont(42);
   Legend.SetTextSize(0.035);
   Legend.SetFillStyle(0);
   Legend.SetBorderSize(0);
   for(int i = 0; i < NLine && i < 4; i++)
      Legend.AddEntry(&Histograms[i], Labels[i].c_str(), "pl");
   Legend.Draw();

   TLegend Legend2(0.55, 0.90, 0.8, 0.90 - 0.04 * (NLine - 4));
   Legend2.SetTextFont(42);
   Legend2.SetTextSize(0.035);
   Legend2.SetFillStyle(0);
   Legend2.SetBorderSize(0);
   if(NLine >= 4)
   {
      for(int i = 4; i < NLine; i++)
         Legend2.AddEntry(&Histograms[i], Labels[i].c_str(), "pl");
      Legend2.Draw();
   }

   Canvas.SaveAs((Output + ".pdf").c_str());
}




void SetPad(TPad &P){
   P.SetLeftMargin(0);
   P.SetTopMargin(0);
   P.SetRightMargin(0);
   P.SetBottomMargin(0);
   P.SetTickx();
   P.SetTicky();
   P.Draw();
}



void DivideByBin(TH1D &H, double Bins[])
{
   int N = H.GetNbinsX();
   for(int i = 1; i <= N; i++)
   {
      double L = Bins[i-1];
      double R = Bins[i];
      H.SetBinContent(i, H.GetBinContent(i) / (R - L));
      H.SetBinError(i, H.GetBinError(i) / (R - L));
   }
}


void MakeCanvas(vector<TH1D>& Histograms, vector<string> Labels, string Output,
   string X, string Y, double WorldMin, double WorldMax, bool DoRatio, bool LogX)
{
   int NLine = Histograms.size();
   int N = Histograms[0].GetNbinsX();

   double MarginL = 180;
   double MarginR = 90;
   double MarginB = 120;
   double MarginT = 90;

   double WorldXMin = LogX ? 0 : 0;
   double WorldXMax = LogX ? N : M_PI;
   
   double PadWidth = 1200;
   double PadHeight = DoRatio ? 640 : 640 + 240;
   double PadRHeight = DoRatio ? 240 : 0.001;

   double CanvasWidth = MarginL + PadWidth + MarginR;
   double CanvasHeight = MarginT + PadHeight + PadRHeight + MarginB;

   MarginL = MarginL / CanvasWidth;
   MarginR = MarginR / CanvasWidth;
   MarginT = MarginT / CanvasHeight;
   MarginB = MarginB / CanvasHeight;

   PadWidth   = PadWidth / CanvasWidth;
   PadHeight  = PadHeight / CanvasHeight;
   PadRHeight = PadRHeight / CanvasHeight;

   TCanvas Canvas("Canvas", "", CanvasWidth, CanvasHeight);

   TPad Pad("Pad", "", MarginL, MarginB + PadRHeight, MarginL + PadWidth, MarginB + PadHeight + PadRHeight);
   Pad.SetLogy();
   SetPad(Pad);
   
   TPad PadR("PadR", "", MarginL, MarginB, MarginL + PadWidth, MarginB + PadRHeight);
   if(DoRatio)
      SetPad(PadR);

   Pad.cd();

   TH2D HWorld("HWorld", "", N, WorldXMin, WorldXMax, 100, WorldMin, WorldMax);
   HWorld.SetStats(0);
   HWorld.GetXaxis()->SetTickLength(0);
   HWorld.GetXaxis()->SetLabelSize(0);

   HWorld.Draw("axis");
   for(TH1D H : Histograms){
      TH1D *H1 = (TH1D *)H.Clone();
      H1->Draw("same");
   }

   TGraph G;
   G.SetPoint(0, LogX ? N / 2 : M_PI / 2, 0);
   G.SetPoint(1, LogX ? N / 2 : M_PI / 2, 1000);
   G.SetLineStyle(kDashed);
   G.SetLineColor(kGray);
   G.SetLineWidth(1);
   G.Draw("l");

   if(DoRatio)
      PadR.cd();

   double WorldRMin = 0.6;
   double WorldRMax = 1.4;
   
   TH2D HWorldR("HWorldR", "", N, WorldXMin, WorldXMax, 100, WorldRMin, WorldRMax);
   TGraph G2;
   
   if(DoRatio)
   {
      HWorldR.SetStats(0);
      HWorldR.GetXaxis()->SetTickLength(0);
      HWorldR.GetXaxis()->SetLabelSize(0);
      HWorldR.GetYaxis()->SetNdivisions(505);

      HWorldR.Draw("axis");
      for(int i = 1; i < NLine; i++)
      {
         TH1D *H = (TH1D *)Histograms[i].Clone();
         H->Divide(&Histograms[0]);
         H->Draw("same");
      }

      G.Draw("l");

      G2.SetPoint(0, 0, 1);
      G2.SetPoint(1, 99999, 1);
      G2.Draw("l");
   }
   
   double BinMin    = 0.002;
   double BinMiddle = M_PI / 2;
   double BinMax    = M_PI - 0.002;

   Canvas.cd();
   TGaxis X1(MarginL, MarginB, MarginL + PadWidth / 2, MarginB, BinMin, BinMiddle, 510, "GS");
   TGaxis X2(MarginL + PadWidth, MarginB, MarginL + PadWidth / 2, MarginB, BinMin, BinMiddle, 510, "-GS");
   TGaxis X3(MarginL, MarginB + PadRHeight, MarginL + PadWidth / 2, MarginB + PadRHeight, BinMin, BinMiddle, 510, "+-GS");
   TGaxis X4(MarginL + PadWidth, MarginB + PadRHeight, MarginL + PadWidth / 2, MarginB + PadRHeight, BinMin, BinMiddle, 510, "+-GS");
   TGaxis Y1(MarginL, MarginB, MarginL, MarginB + PadRHeight, WorldRMin, WorldRMax, 505, "");
   TGaxis Y2(MarginL, MarginB + PadRHeight, MarginL, MarginB + PadRHeight + PadHeight, WorldMin, WorldMax, 510, "G");
   
   TGaxis XL1(MarginL, MarginB, MarginL + PadWidth, MarginB, 0, M_PI, 510, "S");
   TGaxis XL2(MarginL, MarginB + PadRHeight, MarginL + PadWidth, MarginB + PadRHeight, 0, M_PI, 510, "+-S");

   Y1.SetLabelFont(42);
   Y2.SetLabelFont(42);
   XL1.SetLabelFont(42);
   XL2.SetLabelFont(42);

   X1.SetLabelSize(0);
   X2.SetLabelSize(0);
   X3.SetLabelSize(0);
   X4.SetLabelSize(0);
   // XL1.SetLabelSize(0);
   XL2.SetLabelSize(0);

   X1.SetTickSize(0.06);
   X2.SetTickSize(0.06);
   X3.SetTickSize(0.06);
   X4.SetTickSize(0.06);
   XL1.SetTickSize(0.03);
   XL2.SetTickSize(0.03);

   if(LogX == true)
   {
      X1.Draw();
      X2.Draw();
      if(DoRatio) X3.Draw();
      if(DoRatio) X4.Draw();
   }
   if(LogX == false)
   {
      XL1.Draw();
      if(DoRatio)
         XL2.Draw();
   }
   if(DoRatio)
      Y1.Draw();
   Y2.Draw();

   TLatex Latex;
   Latex.SetNDC();
   Latex.SetTextFont(42);
   Latex.SetTextSize(0.035);
   Latex.SetTextAlign(23);
   if(LogX) Latex.DrawLatex(MarginL + (1 - MarginR - MarginL) * 0.115, MarginB - 0.01, "0.01");
   if(LogX) Latex.DrawLatex(MarginL + (1 - MarginR - MarginL) * 0.290, MarginB - 0.01, "0.1");
   if(LogX) Latex.DrawLatex(MarginL + (1 - MarginR - MarginL) * 0.465, MarginB - 0.01, "1");
   if(LogX) Latex.DrawLatex(MarginL + (1 - MarginR - MarginL) * 0.535, MarginB - 0.01, "#pi - 1");
   if(LogX) Latex.DrawLatex(MarginL + (1 - MarginR - MarginL) * 0.710, MarginB - 0.01, "#pi - 0.1");
   if(LogX) Latex.DrawLatex(MarginL + (1 - MarginR - MarginL) * 0.885, MarginB - 0.01, "#pi - 0.01");

   Latex.SetTextAlign(12);
   Latex.SetTextAngle(270);
   Latex.SetTextColor(kGray);
   Latex.DrawLatex(MarginL + (1 - MarginR - MarginL) * 0.5 + 0.0175, 1 - MarginT - 0.015, "#theta_{L} = #pi/2");

   Latex.SetTextAlign(22);
   Latex.SetTextAngle(0);
   Latex.SetTextColor(kBlack);
   Latex.DrawLatex(MarginL + PadWidth * 0.5, MarginB * 0.3, X.c_str());

   Latex.SetTextAlign(22);
   Latex.SetTextAngle(90);
   Latex.SetTextColor(kBlack);
   if(DoRatio)
      Latex.DrawLatex(MarginL * 0.3, MarginB + PadRHeight * 0.5, "Ratio");
   Latex.DrawLatex(MarginL * 0.3, MarginB + PadRHeight + PadHeight * 0.5, Y.c_str());

   Latex.SetTextAlign(11);
   Latex.SetTextAngle(0);
   Latex.DrawLatex(MarginL, MarginB + PadRHeight + PadHeight + 0.012, "ALEPH e^{+}e^{-}, #sqrt{s} = 91.2 GeV, Work-in-progress");

   Latex.SetTextAlign(11);
   Latex.SetTextAngle(0);
   Latex.SetTextColor(19);
   Latex.SetTextSize(0.02);
   Latex.DrawLatex(0.01, 0.01, "Work-in-progress Yi Chen + HB/YJL/AB/..., 2024 Feb 12 (26436_FullEventEEC)");

   TLegend Legend(0.15, 0.90, 0.35, 0.90 - 0.06 * min(NLine, 4));
   Legend.SetTextFont(42);
   Legend.SetTextSize(0.035);
   Legend.SetFillStyle(0);
   Legend.SetBorderSize(0);
   for(int i = 0; i < NLine && i < 4; i++)
      Legend.AddEntry(&Histograms[i], Labels[i].c_str(), "pl");
   Legend.Draw();

   TLegend Legend2(0.7, 0.90, 0.9, 0.90 - 0.06 * (NLine - 4));
   Legend2.SetTextFont(42);
   Legend2.SetTextSize(0.035);
   Legend2.SetFillStyle(0);
   Legend2.SetBorderSize(0);
   if(NLine >= 4)
   {
      for(int i = 4; i < NLine; i++)
         Legend2.AddEntry(&Histograms[i], Labels[i].c_str(), "pl");
      Legend2.Draw();
   }

   Canvas.SaveAs((Output + ".pdf").c_str());
}