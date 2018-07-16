#ifndef NDPluginFits_H
#define NDPluginFits_H

#include <epicsTypes.h>
#include <lmcurve.h>

#include "NDPluginDriver.h"

typedef enum {
	dataSetX,
	dataSetY,
	dataSetFit,
} NDFitsSetType;
#define MAX_SET_TYPES dataSetFit+1

typedef enum {
	statusFitNotExecuted = 0,
	statusFitSuccessful,
	statusFitFailed,
	statusFitWrongArrayDim,
} NDFitsStatus;

typedef struct {
	int index;
	int mu;
} NDFitsPeakOrder;

#define NDPluginFitsXString                  "FITS_X"                   /* (asynFloat64Array, r/o) x points array */
#define NDPluginFitsYString                  "FITS_Y"                   /* (asynFloat64Array, r/o) y points array */
#define NDPluginFitsFitString                "FITS_FIT"                 /* (asynFloat64Array, r/o) gauss fit array */
#define NDPluginFitsPeakAmplitudeString      "FITS_PEAK_AMPLITUDE"      /* (asynFloat64,      r/o) peak amplitude */
#define NDPluginFitsPeakMuString             "FITS_PEAK_MU"             /* (asynFloat64,      r/o) peak mu */
#define NDPluginFitsPeakSigmaString          "FITS_PEAK_SIGMA"          /* (asynFloat64,      r/o) peak sigma */
#define NDPluginFitsBackgroundString         "FITS_BACKGROUND"          /* (asynFloat64,      r/o) background value */
#define NDPluginFitsPeakAmplitudeActualString "FITS_PEAK_AMPLITUDE_ACTUAL"      /* (asynFloat64,      r/o) peak amplitude actual */
#define NDPluginFitsPeakMuActualString       "FITS_PEAK_MU_ACTUAL"             /* (asynFloat64,      r/o) peak mu actual*/
#define NDPluginFitsPeakSigmaActualString    "FITS_PEAK_SIGMA_ACTUAL"          /* (asynFloat64,      r/o) peak sigma actual */
#define NDPluginFitsBackgroundActualString   "FITS_BACKGROUND_ACTUAL"          /* (asynFloat64,      r/o) background value actual */
#define NDPluginFitsStatusString             "FITS_STATUS"              /* (asynInt32,        r/o) status of the fitting */
#define NDPluginFitsPeaksString              "FITS_PEAKS"               /* (asynInt32,        r/o) number of peaks to fit */
#define NDPluginFitsPointsString             "FITS_POINTS"              /* (asynInt32,        r/o) number of points in fit */
#define NDPluginFitsGoodCounterString        "FITS_GOOD_COUNTER"        /* (asynInt32,        r/o) number of successful fits so far */
#define NDPluginFitsBadCounterString         "FITS_BAD_COUNTER"         /* (asynInt32,        r/o) number of failed fits so far */
#define NDPluginFitsResetCountersString      "FITS_RESET_COUNTERS"      /* (asynInt32,        w/o) reset the good and bad fit counter */
#define NDPluginFitsFtolString               "FITS_FTOL"                /* (asynFloat64,      r/w) lm_control_struct ftol value */
#define NDPluginFitsXtolString               "FITS_XTOL"                /* (asynFloat64,      r/w) lm_control_struct xtol value */
#define NDPluginFitsGtolString               "FITS_GTOL"                /* (asynFloat64,      r/w) lm_control_struct gtol value */
#define NDPluginFitsEpsilonString            "FITS_EPSILON"             /* (asynFloat64,      r/w) lm_control_struct epsilon value */
#define NDPluginFitsStepboundString          "FITS_STEPBOUND"           /* (asynFloat64,      r/w) lm_control_struct stepbound value */
#define NDPluginFitsPatienceString           "FITS_PATIENCE"            /* (asynInt32,        r/w) lm_control_struct patience value */
#define NDPluginFitsScaleDiagString          "FITS_SCALE_DIAG"          /* (asynInt32,        r/w) lm_control_struct scale_diag value */
#define NDPluginFitsNrIterationsString       "FITS_NR_ITERATIONS"       /* (asynInt32,        r/o) lm_status_struct nfev value */
#define NDPluginFitsResidVectorNormString    "FITS_RESID_VECTOR_NORM"   /* (asynFloat64,      r/o) lm_status_struct fnorm value */
#define NDPluginFitsOutcomeString            "FITS_OUTCOME"             /* (asynInt32,        r/o) lm_status_struct outcome value */
#define NDPluginFitsOutcomeStrString         "FITS_OUTCOME_STR"         /* (asynOctetRead,    r/o) lm_status_struct outcome value as string */
#define NDPluginFitsOutcomeStrLongString     "FITS_OUTCOME_STR_LONG"    /* (asynOctetRead,    r/o) lm_status_struct outcome value as long string */

/** Does curve fitting to Gaussian model.
  */
class epicsShareClass NDPluginFits : public NDPluginDriver {
public:
    NDPluginFits(const char *portName, int queueSize, int blockingCallbacks,
					const char *NDArrayPort, int NDArrayAddr, int maxPeaks,
					int maxBuffers, size_t maxMemory,
					int priority, int stackSize, int maxThreads);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
    asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

    template <typename epicsType> asynStatus doComputeFitsT(NDArray *pArray);
    asynStatus doComputeFits(NDArray *pArray);

protected:
    int NDPluginFitsX;
#define FIRST_NDPLUGIN_FITS_PARAM NDPluginFitsX
    int NDPluginFitsY;
    int NDPluginFitsFit;
    int NDPluginFitsPeakAmplitude;
    int NDPluginFitsPeakMu;
    int NDPluginFitsPeakSigma;
    int NDPluginFitsBackground;
    int NDPluginFitsPeakAmplitudeActual;
    int NDPluginFitsPeakMuActual;
    int NDPluginFitsPeakSigmaActual;
    int NDPluginFitsBackgroundActual;
    int NDPluginFitsStatus;
    int NDPluginFitsPeaks;
    int NDPluginFitsPoints;
    int NDPluginFitsGoodCounter;
    int NDPluginFitsBadCounter;
    int NDPluginFitsResetCounters;
    int NDPluginFitsFtol;
    int NDPluginFitsXtol;
    int NDPluginFitsGtol;
    int NDPluginFitsEpsilon;
    int NDPluginFitsStepbound;
    int NDPluginFitsPatience;
    int NDPluginFitsScaleDiag;
    int NDPluginFitsNrIterations;
    int NDPluginFitsResidVectorNorm;
    int NDPluginFitsOutcome;
    int NDPluginFitsOutcomeStr;
    int NDPluginFitsOutcomeStrLong;

private:
    epicsInt32 points;
    epicsInt32 maxPeaks;
    double background;
    epicsInt32 nrFitParam;
    double *fitParam;
    double *initialFitParam;
    double *dataSet[MAX_SET_TYPES];
    lm_control_struct lmControl;
    NDFitsPeakOrder *order;
};

#endif
