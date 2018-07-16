#!/bin/bash

# optimal startup values for ROI region, X is a sequence 0, 1, 2, ..

caput 13SIM1:Fits1:Background -41.0
caput 13SIM1:Fits1:1:PeakAmplitude 870.0
caput 13SIM1:Fits1:2:PeakAmplitude 5530.0
caput 13SIM1:Fits1:3:PeakAmplitude 2554.0
caput 13SIM1:Fits1:1:PeakSigma 5
caput 13SIM1:Fits1:2:PeakSigma 5
caput 13SIM1:Fits1:3:PeakSigma 5
caput 13SIM1:Fits1:1:PeakMu 49
caput 13SIM1:Fits1:2:PeakMu 125
caput 13SIM1:Fits1:3:PeakMu 159

