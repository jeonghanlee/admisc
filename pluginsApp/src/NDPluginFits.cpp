/*
 * NDPluginFits.cpp
 *
 * Image statistics and profile fitting plugin
 * Author: Mark Rivers, Hinko Kocevar
 *
 * Updated March 13, 2017
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <epicsString.h>
#include <epicsMutex.h>
#include <iocsh.h>

#include <asynDriver.h>

#include <epicsExport.h>
#include "NDPluginDriver.h"
#include "NDPluginFits.h"

/* control some debug output of the fitting */
/*#define FITS_DEBUG 1*/

static const char *driverName = "NDPluginFits";

/******************************************************************************/
/*  Message texts (indexed by status.info)                                    */
/******************************************************************************/

static const char* __lm_infmsg[] = {
    "found zero (sum of squares below underflow limit)",
    "converged  (the relative error in the sum of squares is at most tol)",
    "converged  (the relative error of the parameter vector is at most tol)",
    "converged  (both errors are at most tol)",
    "trapped    (by degeneracy; increasing epsilon might help)",
    "exhausted  (number of function calls exceeding preset patience)",
    "failed     (ftol<tol: cannot reduce sum of squares any further)",
    "failed     (xtol<tol: cannot improve approximate solution any further)",
    "failed     (gtol<tol: cannot improve approximate solution any further)",
    "crashed    (not enough memory)",
    "exploded   (fatal coding error: improper input parameters)",
    "stopped    (break requested within function evaluation)",
    "found nan  (function value is not-a-number or infinite)"};

static const char* __lm_shortmsg[] = {
    "found zero",
    "converged (f)",
    "converged (p)",
    "converged (2)",
    "degenerate",
    "call limit",
    "failed (f)",
    "failed (p)",
    "failed (o)",
    "no memory",
    "invalid input",
    "user break",
    "found nan"};

/* model function: a n-peak gauss */
double f(double t, const double *p) {
	/* number of peaks */
	int P = (int)p[0];
	/* background */
	double B = p[1];
	/* amplitude */
	double A;
	/* mu */
	double mu;
	/* sigma */
	double sigma;
	double term;
	double fi = 0;
	int i;

    /* 3 parameters per peak */
	P *= 3;

	fi += B;
	for (i = 0; i < P; i += 3) {
		A = p[i + 2];
		mu = p[i + 3];
		sigma = p[i + 4];
		term = (t - mu) / sigma;
		fi += A * exp(-0.5 * term * term);
	}

	//printf("t %.9f fi %.9g ", t, fi);
	return fi;
}

/* for qsort() */
int cmpfunc(const void * a, const void * b) {
	NDFitsPeakOrder *aa = (NDFitsPeakOrder *)a;
	NDFitsPeakOrder *bb = (NDFitsPeakOrder *)b;
	return (aa->mu - bb->mu);
}

template <typename epicsType>
asynStatus NDPluginFits::doComputeFitsT(NDArray *pArray)
{
	epicsInt32 i, j, k;
	asynStatus ret;
    epicsFloat64 ftol;
    epicsFloat64 xtol;
    epicsFloat64 gtol;
    epicsFloat64 epsilon;
    epicsFloat64 stepbound;
    epicsInt32 patience;
    epicsInt32 scaleDiag;

    if (pArray->ndims > 2) {
    	setIntegerParam(NDPluginFitsStatus, statusFitWrongArrayDim);
    	return asynError;
    }

    this->unlock();

    epicsType *pData = (epicsType *)pArray->pData;
	for (i = 0; i < this->points; i++) {
		this->dataSet[dataSetX][i] = i;
		this->dataSet[dataSetFit][i] = 0.0;
		this->dataSet[dataSetY][i] = (double)pData[i];
	}
	double *t = this->dataSet[dataSetX];
	double *y = this->dataSet[dataSetY];
	double *fit = this->dataSet[dataSetFit];

    /* data points size */
	int m = this->points;
	/* number of parameters in model function f */
	int n = this->nrFitParam;

	/* always grab starting values from user input */
	memcpy(this->fitParam, this->initialFitParam, this->nrFitParam * sizeof(double));
    double *p = this->fitParam;

#ifdef FITS_DEBUG
	printf("\nInitial params: ");
	for (i = 0; i < n; ++i) {
		printf("%.7g ", p[i]);
	}
	printf("\n\n");
	printf("\nInitial t: ");
	for (i = 0; i < m; ++i) {
		printf("%.7g ", t[i]);
	}
	printf("\n\n");

	printf("\nInitial y: ");
	for (i = 0; i < m; ++i) {
		printf("%.7g ", y[i]);
	}
	printf("\n\n");
#endif

    lm_control_struct control = lmControl;
    getDoubleParam(NDPluginFitsFtol, &ftol);
    getDoubleParam(NDPluginFitsXtol, &xtol);
    getDoubleParam(NDPluginFitsGtol, &gtol);
    getDoubleParam(NDPluginFitsEpsilon, &epsilon);
    getDoubleParam(NDPluginFitsStepbound, &stepbound);
    getIntegerParam(NDPluginFitsPatience, &patience);
    getIntegerParam(NDPluginFitsScaleDiag, &scaleDiag);

    control.ftol = ftol;
    control.xtol = xtol;
    control.gtol = gtol;
    control.epsilon = epsilon;
    control.stepbound = stepbound;
    control.patience = patience;
    control.scale_diag = scaleDiag;

    lm_status_struct status;
    //control.verbosity = 9;
    control.verbosity = 0;

    /* now the call to lmfit */
    lmcurve(n, p, m, t, y, f, &control, &status);

#ifdef FITS_DEBUG
    printf("Results:\n" );
    printf("status after %d function evaluations:\n  %s\n",
            status.nfev, lm_infmsg[status.outcome] );

    printf("obtained parameters:\n");
    for (i = 0; i < n; ++i)
        printf("  par[%i] = %12g\n", i, p[i]);
    printf("obtained norm:\n  %12g\n", status.fnorm );

    printf("fitting data as follows:\n");
    for (i = 0; i < m; ++i) {
        printf("  t[%2d]=%4g y=%6g fit=%10g residue=%12g\n",
                i, t[i], y[i], f(t[i],p), y[i] - f(t[i],p));
    }
#endif

	bool goodFit = false;
    /* see lmfit.c for outcome codes; positive <4 is success, other failure */
    if ((status.nfev > 0) && (status.outcome >= 0 && status.outcome <= 3)) {
        /* we want positive amplitudes and sigmas. if fit was successful it
         * might still not be good for us. */
    	goodFit = true;
    	for (i = 0, j = 2; i < this->maxPeaks; i++, j += 3) {
    		if ((p[j] < 0) || (p[j+2] < 0)) {
    			goodFit = false;
    		}
    	}
    }

    setIntegerParam(NDPluginFitsOutcome, status.outcome);
    i = status.nfev;
    if (i > 0) {
    	/* nrIter = patience * (number_of_parameters + 1) */
    	i = i / (this->nrFitParam + 1);
    }
	setIntegerParam(NDPluginFitsNrIterations, i);
    setDoubleParam(NDPluginFitsResidVectorNorm, status.fnorm);

    if (goodFit) {
	    if ((status.outcome >= 0) && (status.outcome < 4) && (status.outcome < (int)(sizeof(__lm_infmsg)))) {
	    	setStringParam(NDPluginFitsOutcomeStr, __lm_shortmsg[status.outcome]);
	    	setStringParam(NDPluginFitsOutcomeStrLong, __lm_infmsg[status.outcome]);
	    } else {
	    	setStringParam(NDPluginFitsOutcomeStr, "Unknown outcome");
	    	setStringParam(NDPluginFitsOutcomeStrLong, "Unknown outcome");
	    }

	    /* update fitted values */
		for (i = 0; i < m; ++i) {
			fit[i] = f(t[i],p);
		}

        /* Reorder the peaks according to ascending mu */
#ifdef FITS_DEBUG
        printf("PRE peak order: \n");
#endif
		for (i = 0, j = 2; i < this->maxPeaks; i++, j += 3) {
			this->order[i].index = i;
			this->order[i].mu = p[j+1];
#ifdef FITS_DEBUG
			printf("%d = %d\n", this->order[i].index, this->order[i].mu);
#endif
		}

		qsort(this->order, this->maxPeaks, sizeof(NDFitsPeakOrder), cmpfunc);

#ifdef FITS_DEBUG
        printf("POST peak order: \n");
		for (i = 0, j = 2; i < this->maxPeaks; i++, j += 3) {
			printf("%d = %d\n", this->order[i].index, this->order[i].mu);
		}
#endif

		setDoubleParam(NDPluginFitsBackgroundActual, p[1]);
		for (i = 0, j = 2; i < this->maxPeaks; i++, j += 3) {
			/*  Use ordered indexes, by ascending mu */
            k = this->order[i].index;
			setDoubleParam(k, NDPluginFitsPeakAmplitudeActual, p[j]);
			setDoubleParam(k, NDPluginFitsPeakMuActual, p[j+1]);
			setDoubleParam(k, NDPluginFitsPeakSigmaActual, p[j+2]);
			callParamCallbacks(k);
		}
		getIntegerParam(NDPluginFitsGoodCounter, &i);
		i++;
		setIntegerParam(NDPluginFitsGoodCounter, i);
		setIntegerParam(NDPluginFitsStatus, statusFitSuccessful);
		ret = asynSuccess;
    } else {
	    if ((status.outcome > 3) && (status.outcome < (int)(sizeof(__lm_infmsg)))) {
	    	setStringParam(NDPluginFitsOutcomeStr, __lm_shortmsg[status.outcome]);
	    	setStringParam(NDPluginFitsOutcomeStrLong, __lm_infmsg[status.outcome]);
	    } else {
	    	setStringParam(NDPluginFitsOutcomeStr, "Bad fit");
	    	setStringParam(NDPluginFitsOutcomeStrLong, "Bad fit");
	    }

		/* reset fitted values */
        for (i = 0; i < m; ++i) {
        	fit[i] = 0.0;
        }
		setDoubleParam(NDPluginFitsBackgroundActual, 0);
		for (i = 0, j = 2; i < this->maxPeaks; i++, j += 3) {
			setDoubleParam(i, NDPluginFitsPeakAmplitudeActual, 0);
			setDoubleParam(i, NDPluginFitsPeakMuActual, 0);
			setDoubleParam(i, NDPluginFitsPeakSigmaActual, 0);
			callParamCallbacks(i);
		}
		getIntegerParam(NDPluginFitsBadCounter, &i);
		i++;
		setIntegerParam(NDPluginFitsBadCounter, i);
		setIntegerParam(NDPluginFitsStatus, statusFitFailed);
		ret = asynError;
	}

    this->lock();
    doCallbacksFloat64Array(this->dataSet[dataSetY],   this->points, NDPluginFitsY, 0);
    doCallbacksFloat64Array(this->dataSet[dataSetX],   this->points, NDPluginFitsX, 0);
    doCallbacksFloat64Array(this->dataSet[dataSetFit], this->points, NDPluginFitsFit, 0);

    return ret;
}

asynStatus NDPluginFits::doComputeFits(NDArray *pArray)
{
    asynStatus status;

    switch(pArray->dataType) {
        case NDInt8:
            status = doComputeFitsT<epicsInt8>(pArray);
            break;
        case NDUInt8:
            status = doComputeFitsT<epicsUInt8>(pArray);
            break;
        case NDInt16:
            status = doComputeFitsT<epicsInt16>(pArray);
            break;
        case NDUInt16:
            status = doComputeFitsT<epicsUInt16>(pArray);
            break;
        case NDInt32:
            status = doComputeFitsT<epicsInt32>(pArray);
            break;
        case NDUInt32:
            status = doComputeFitsT<epicsUInt32>(pArray);
            break;
        case NDFloat32:
            status = doComputeFitsT<epicsFloat32>(pArray);
            break;
        case NDFloat64:
            status = doComputeFitsT<epicsFloat64>(pArray);
            break;
        default:
            status = asynError;
        break;
    }
    return(status);
}

/** Callback function that is called by the NDArray driver with new NDArray data.
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginFits::processCallbacks(NDArray *pArray)
{
    /* It is called with the mutex already locked.
     * It unlocks it during long calculations when private structures don't
     * need to be protected.
     */
    size_t sizeX = 0;
    size_t sizeY = 0;
    size_t points = 0;
    int i;
    NDArrayInfo arrayInfo;
    asynStatus status;
    static const char* functionName = "processCallbacks";

    /* Call the base class method */
    NDPluginDriver::beginProcessCallbacks(pArray);

    pArray->getInfo(&arrayInfo);

    if (pArray->ndims > 0) sizeX = pArray->dims[0].size;
    if (pArray->ndims == 1) sizeY = 1;
    if (pArray->ndims > 1)  sizeY = pArray->dims[1].size;

    points = sizeX;
    if ((sizeX <= 1) && (sizeY > sizeX)) {
    	points = sizeY;
    }
    if (points != (size_t)this->points) {
        this->points = points;
        for (i = 0; i < MAX_SET_TYPES; i++) {
            if (this->dataSet[i]) free(this->dataSet[i]);
            this->dataSet[i] = (double *)calloc(this->points, sizeof(double));
        }
		setIntegerParam(NDPluginFitsPoints, this->points);
    }

    status = doComputeFits(pArray);

    NDPluginDriver::endProcessCallbacks(pArray, true, true);

    if (status) {
				asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
					"%s::%s: Failed to compute fits.\n", driverName, functionName);
		}

    callParamCallbacks();
}

/** Called when asyn clients call pasynFloat64->write().
  * This function performs actions for some parameters.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus  NDPluginFits::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int peak = 0;
    int idx = 0;
    static const char *functionName = "writeFloat64";

    status = getAddress(pasynUser, &peak);
    if (status != asynSuccess) {
      return status;
    }

    /* Set the parameter and readback in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status = setDoubleParam(peak, function, value);

    /* Save the user input parameters as initial parameters for each
     * processing. After processing results are set and available to user. */
	idx = peak * 3;
    if (function == NDPluginFitsPeakAmplitude) {
    	idx += 2;
    	this->initialFitParam[idx] = value;
    } else if (function == NDPluginFitsPeakMu) {
    	idx += 3;
    	this->initialFitParam[idx] = value;
    } else if (function == NDPluginFitsPeakSigma) {
    	idx += 4;
    	this->initialFitParam[idx] = value;
    } else if (function == NDPluginFitsBackground) {
    	idx = 1;
    	this->initialFitParam[idx] = value;
    } else if (function < FIRST_NDPLUGIN_FITS_PARAM) {
		/* If this parameter belongs to a base class call its method */
    	status = NDPluginDriver::writeFloat64(pasynUser, value);
    }

    /* Do callbacks so higher layers see any changes */
    status = (asynStatus) callParamCallbacks(peak);
    status = (asynStatus) callParamCallbacks();

    if (status)
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: error, status=%d function=%d, value=%f\n",
              driverName, functionName, status, function, value);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s: function=%d, value=%f\n",
              driverName, functionName, function, value);
    return status;
}

/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus  NDPluginFits::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    static const char *functionName = "writeInt32";

    /* Set the parameter and readback in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status = setIntegerParam(function, value);

    if (function == NDPluginFitsResetCounters) {
    	setIntegerParam(NDPluginFitsGoodCounter, 0);
    	setIntegerParam(NDPluginFitsBadCounter, 0);
    } else if (function < FIRST_NDPLUGIN_FITS_PARAM) {
		/* If this parameter belongs to a base class call its method */
    	status = NDPluginDriver::writeInt32(pasynUser, value);
    }

    /* Do callbacks so higher layers see any changes */
    status = (asynStatus) callParamCallbacks();

    if (status)
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: error, status=%d function=%d, value=%d\n",
              driverName, functionName, status, function, value);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s: function=%d, value=%d\n",
              driverName, functionName, function, value);
    return status;
}


/** Constructor for NDPluginFits; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
  * After calling the base class constructor this method sets reasonable default values for all of the
  * parameters.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when
  *            NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *            at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *            0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *            of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param[in] maxPeaks The maximum number of peaks that this plugin will support
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] maxThreads The maximum number of threads this plugin is allowed to use.
  */
NDPluginFits::NDPluginFits(const char *portName, int queueSize, int blockingCallbacks,
							const char *NDArrayPort, int NDArrayAddr, int maxPeaks,
							int maxBuffers, size_t maxMemory,
							int priority, int stackSize, int maxThreads)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, maxPeaks, maxBuffers, maxMemory,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
				   ASYN_MULTIDEVICE, 1, priority, stackSize, maxThreads)
{
    //static const char *functionName = "NDPluginFits";

    /* Gauss */
    createParam(NDPluginFitsXString,                 asynParamFloat64Array,  &NDPluginFitsX);
    createParam(NDPluginFitsYString,                 asynParamFloat64Array,  &NDPluginFitsY);
    createParam(NDPluginFitsFitString,               asynParamFloat64Array,  &NDPluginFitsFit);
    createParam(NDPluginFitsPeakAmplitudeString,     asynParamFloat64,       &NDPluginFitsPeakAmplitude);
    createParam(NDPluginFitsPeakMuString,            asynParamFloat64,       &NDPluginFitsPeakMu);
    createParam(NDPluginFitsPeakSigmaString,         asynParamFloat64,       &NDPluginFitsPeakSigma);
    createParam(NDPluginFitsBackgroundString,        asynParamFloat64,       &NDPluginFitsBackground);
    createParam(NDPluginFitsPeakAmplitudeActualString, asynParamFloat64,       &NDPluginFitsPeakAmplitudeActual);
    createParam(NDPluginFitsPeakMuActualString,      asynParamFloat64,       &NDPluginFitsPeakMuActual);
    createParam(NDPluginFitsPeakSigmaActualString,   asynParamFloat64,       &NDPluginFitsPeakSigmaActual);
    createParam(NDPluginFitsBackgroundActualString,  asynParamFloat64,       &NDPluginFitsBackgroundActual);
    createParam(NDPluginFitsStatusString,            asynParamInt32,         &NDPluginFitsStatus);
    createParam(NDPluginFitsPeaksString,             asynParamInt32,         &NDPluginFitsPeaks);
    createParam(NDPluginFitsPointsString,            asynParamInt32,         &NDPluginFitsPoints);
    createParam(NDPluginFitsGoodCounterString,       asynParamInt32,         &NDPluginFitsGoodCounter);
    createParam(NDPluginFitsBadCounterString,        asynParamInt32,         &NDPluginFitsBadCounter);
    createParam(NDPluginFitsResetCountersString,     asynParamInt32,         &NDPluginFitsResetCounters);
    createParam(NDPluginFitsFtolString,              asynParamFloat64,       &NDPluginFitsFtol);
    createParam(NDPluginFitsGtolString,              asynParamFloat64,       &NDPluginFitsGtol);
    createParam(NDPluginFitsXtolString,              asynParamFloat64,       &NDPluginFitsXtol);
    createParam(NDPluginFitsEpsilonString,           asynParamFloat64,       &NDPluginFitsEpsilon);
    createParam(NDPluginFitsStepboundString,         asynParamFloat64,       &NDPluginFitsStepbound);
    createParam(NDPluginFitsPatienceString,          asynParamInt32,         &NDPluginFitsPatience);
    createParam(NDPluginFitsScaleDiagString,         asynParamInt32,         &NDPluginFitsScaleDiag);
    createParam(NDPluginFitsNrIterationsString,      asynParamInt32,         &NDPluginFitsNrIterations);
    createParam(NDPluginFitsResidVectorNormString,   asynParamFloat64,       &NDPluginFitsResidVectorNorm);
    createParam(NDPluginFitsOutcomeString,           asynParamInt32,         &NDPluginFitsOutcome);
    createParam(NDPluginFitsOutcomeStrString,        asynParamOctet,         &NDPluginFitsOutcomeStr);
    createParam(NDPluginFitsOutcomeStrLongString,    asynParamOctet,         &NDPluginFitsOutcomeStrLong);

    /* Make sure that buffers are not allocated */
    for (int i = 0; i < MAX_SET_TYPES; i++) {
    	this->dataSet[i] = NULL;
    }
    /* number of peaks to detect */
    this->maxPeaks = maxPeaks;
    /* for each peak three parameters need to be fitted:
     * amplitude, mu and sigma
     * fit parameters also include background and number of peaks */
    this->nrFitParam = this->maxPeaks * 3 + 1 + 1;
    this->fitParam = (double *)calloc(this->nrFitParam, sizeof(double));
    this->initialFitParam = (double *)calloc(this->nrFitParam, sizeof(double));
    this->initialFitParam[0] = maxPeaks;
    /* initialize input parameters for fit control */
    this->lmControl = lm_control_double;
    this->order = (NDFitsPeakOrder *)calloc(this->maxPeaks, sizeof(NDFitsPeakOrder));

    setStringParam(NDPluginFitsOutcomeStr, "");
    setStringParam(NDPluginFitsOutcomeStrLong, "");
    setIntegerParam(NDPluginFitsGoodCounter, 0);
    setIntegerParam(NDPluginFitsBadCounter, 0);

    setIntegerParam(NDPluginFitsPeaks, this->maxPeaks);
    callParamCallbacks(0);

    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginFits");

    /* Try to connect to the array port */
    connectToArrayPort();
}

/** Configuration command */
extern "C" int NDFitsConfigure(const char *portName, int queueSize, int blockingCallbacks,
								const char *NDArrayPort, int NDArrayAddr,
								int maxPeaks, int maxBuffers, size_t maxMemory,
								int priority, int stackSize, int maxThreads)
{
    NDPluginFits *plugin = new NDPluginFits(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
    					maxPeaks, maxBuffers, maxMemory, priority, stackSize, maxThreads);
    return plugin->start();
}

/* EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArrayPort",iocshArgString};
static const iocshArg initArg4 = { "NDArrayAddr",iocshArgInt};
static const iocshArg initArg5 = { "maxPeaks",iocshArgInt};
static const iocshArg initArg6 = { "maxBuffers",iocshArgInt};
static const iocshArg initArg7 = { "maxMemory",iocshArgInt};
static const iocshArg initArg8 = { "priority",iocshArgInt};
static const iocshArg initArg9 = { "stackSize",iocshArgInt};
static const iocshArg initArg10 = { "maxThreads",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8,
                                            &initArg9,
                                            &initArg10};
static const iocshFuncDef initFuncDef = {"NDFitsConfigure",11,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDFitsConfigure(args[0].sval, args[1].ival, args[2].ival,
            args[3].sval, args[4].ival, args[5].ival,
            args[6].ival, args[7].ival, args[8].ival,
            args[9].ival, args[10].ival);
}

extern "C" void NDFitsRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDFitsRegister);
}
