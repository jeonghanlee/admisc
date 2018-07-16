/*
 * NDPluginImaging.cpp
 *
 * Image statistics and profile fitting plugin
 * Author: Mark Rivers, Hinko Kocevar
 *
 * Created March 14, 2016
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
#include "NDPluginImaging.h"

static const char *driverName = "NDPluginImaging";


template <typename epicsType>
asynStatus NDPluginImaging::doComputeImagingT(NDArray *pArray)
{
    asynStatus status = asynSuccess;
    
    this->unlock();

/* **************************** */
    // do real work here, for demonstration puposes we genrate some random
    // numbers and update the array
    for (size_t i = 0; i < this->points; i++) {
        this->dataSet[i] = (double)(random());
    }
/* **************************** */

    this->lock();

    // If we have changed the resulting array, call the parameter callbacks
    doCallbacksFloat64Array(this->dataSet, this->points, NDPluginImagingDummy1, 0);

    return status;
}

asynStatus NDPluginImaging::doComputeImaging(NDArray *pArray)
{
    asynStatus status;

    switch(pArray->dataType) {
        case NDInt8:
            status = doComputeImagingT<epicsInt8>(pArray);
            break;
        case NDUInt8:
            status = doComputeImagingT<epicsUInt8>(pArray);
            break;
        case NDInt16:
            status = doComputeImagingT<epicsInt16>(pArray);
            break;
        case NDUInt16:
            status = doComputeImagingT<epicsUInt16>(pArray);
            break;
        case NDInt32:
            status = doComputeImagingT<epicsInt32>(pArray);
            break;
        case NDUInt32:
            status = doComputeImagingT<epicsUInt32>(pArray);
            break;
        case NDFloat32:
            status = doComputeImagingT<epicsFloat32>(pArray);
            break;
        case NDFloat64:
            status = doComputeImagingT<epicsFloat64>(pArray);
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
void NDPluginImaging::processCallbacks(NDArray *pArray)
{
    /* It is called with the mutex already locked.
     * It unlocks it during long calculations when private structures don't
     * need to be protected.
     */
    size_t sizeX = 0;
    size_t sizeY = 0;
    size_t points = 0;
    NDArrayInfo arrayInfo;
    asynStatus status;
    int arrayCallbacks = 0;
    static const char* functionName = "processCallbacks";

    /* Call the base class method */
    NDPluginDriver::beginProcessCallbacks(pArray);
    
    pArray->getInfo(&arrayInfo);

    if (pArray->ndims > 0) sizeX = pArray->dims[0].size;
    if (pArray->ndims == 1) sizeY = 1;
    if (pArray->ndims > 1)  sizeY = pArray->dims[1].size;
    printf("%s::%s(): Handling image of size %ld x %ld\n", driverName, functionName, sizeX, sizeY);

    // Make sure that we have enough space for the image data
    points = sizeX * sizeY;
    if (points != this->points) {
        this->points = points;
        if (this->dataSet) free(this->dataSet);
        this->dataSet = (double *)calloc(this->points, sizeof(double));
    }
    printf("%s::%s(): Data set has %ld points\n", driverName, functionName, points);

    status = doComputeImaging(pArray);
	callParamCallbacks(0);

    if (status == asynSuccess) {
		getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
		if (arrayCallbacks == 1) {
			NDArray *pArrayOut = this->pNDArrayPool->copy(pArray, NULL, 1);
			if (NULL != pArrayOut) {
				this->getAttributes(pArrayOut->pAttributeList);
				this->unlock();
				doCallbacksGenericPointer(pArrayOut, NDArrayData, 0);
				this->lock();
				pArrayOut->release();
			}
			else {
				asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
					"%s::%s: Couldn't allocate output array. Further processing terminated.\n",
					driverName, functionName);
			}
		}
    }

    callParamCallbacks();
}

/** Called when asyn clients call pasynFloat64->write().
  * This function performs actions for some parameters.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus  NDPluginImaging::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int peak = 0;
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
    if (function == NDPluginImagingDummy2) {
    	// Do something else with the value, eg. call a function
    } else if (function < FIRST_NDPLUGIN_IMAGING_PARAM) {
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


/** Constructor for NDPluginImaging; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
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
  */
NDPluginImaging::NDPluginImaging(const char *portName, int queueSize, int blockingCallbacks,
							const char *NDArrayPort, int NDArrayAddr, int maxPeaks,
							int maxBuffers, size_t maxMemory,
							int priority, int stackSize)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, maxPeaks, NUM_NDPLUGIN_IMAGING_PARAMS, maxBuffers, maxMemory,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
				   ASYN_MULTIDEVICE, 1, priority, stackSize)
{
    //static const char *functionName = "NDPluginImaging";

    /* Gauss */
    createParam(NDPluginImagingDummy1String, asynParamFloat64Array,  &NDPluginImagingDummy1);
    createParam(NDPluginImagingDummy2String, asynParamFloat64,       &NDPluginImagingDummy2);

    setIntegerParam(NDPluginImagingDummy2, 0.0);
    callParamCallbacks(0);

    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginImaging");

    /* Try to connect to the array port */
    connectToArrayPort();
}

/** Configuration command */
extern "C" int NDImagingConfigure(const char *portName, int queueSize, int blockingCallbacks,
								const char *NDArrayPort, int NDArrayAddr,
								int maxPeaks, int maxBuffers, size_t maxMemory,
								int priority, int stackSize)
{
    NDPluginImaging *plugin = new NDPluginImaging(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
    					maxPeaks, maxBuffers, maxMemory, priority, stackSize);
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
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8,
                                            &initArg9};
static const iocshFuncDef initFuncDef = {"NDImagingConfigure",10,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDImagingConfigure(args[0].sval, args[1].ival, args[2].ival,
            args[3].sval, args[4].ival, args[5].ival,
            args[6].ival, args[7].ival, args[8].ival, args[9].ival);
}

extern "C" void NDImagingRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDImagingRegister);
}
