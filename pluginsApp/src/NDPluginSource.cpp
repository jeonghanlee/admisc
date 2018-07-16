/*
 * NDPluginSource.cpp
 *
 * Image statistics plugin
 * Author: Hinko Kocevar
 *
 * Created March 19, 2015
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <epicsString.h>
#include <epicsMutex.h>
#include <iocsh.h>

#include "NDArray.h"
#include "NDPluginSource.h"
#include <epicsExport.h>


static const char *driverName="NDPluginSource";


/** Callback function that is called by the NDArray driver with new NDArray data.
  * Does image statistics.
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginSource::processCallbacks(NDArray *pArray)
{
	/* XXX: Should not happen!!! */
	printf("%s: This should not happen?!?!\n", __func__);

}

/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus NDPluginSource::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;

    /* Set the parameter in the parameter library. */
    status = (asynStatus) setIntegerParam(function, value);

    if (function == NDPluginSourceLength) {
		this->lock();

		if (pArray) {
			NDArrayInfo info;
    		pArray->getInfo(&info);
    		if ((epicsUInt32)value != info.nElements) {
    			pArray->release();
    			pArray = NULL;
    			setIntegerParam(NDPluginSourceIndex, 0);
    		}
    	}

		this->unlock();
    } else if (function == NDPluginDriverEnableCallbacks) {
    	if (value) {
    		doRun = true;
    	} else {
    		doRun = false;
    	}
    } else {
        /* If this parameter belongs to a base class call its method */
        if (function < FIRST_NDPLUGIN_SOURCE_PARAM)
            status = NDPluginDriver::writeInt32(pasynUser, value);
    }

    /* Do callbacks so higher layers see any changes */
    status = (asynStatus) callParamCallbacks();

    if (status)
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                  "%s:%s: status=%d, function=%d, value=%d",
                  driverName, __func__, status, function, value);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s: function=%d, value=%d\n",
              driverName, __func__, function, value);
    return status;
}

/** Called when asyn clients call pasynFloat64->write().
  * This function performs actions for some parameters.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus  NDPluginSource::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
	size_t dims[2];
	epicsInt32 index, length;
	epicsFloat64 *data;
	bool doCallback = false;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;

    /* Set the parameter and readback in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status = setDoubleParam(function, value);

	if (function == NDPluginSourceExtValue) {
		// we have received new external value
		this->lock();

		if (! doRun) {
	        asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
					"%s:%s:, plugin not enabled\n", driverName,
					__func__);
		} else {
			getIntegerParam(NDPluginSourceIndex, &index);
			getIntegerParam(NDPluginSourceLength, &length);
			if (! pArray) {
	        	dims[0] = length;
	        	dims[1] = 1;
	        	pArray = pNDArrayPool->alloc(2, dims, NDFloat64, 0, NULL);
	    	}

			data = (epicsFloat64 *)(pArray->pData) + index;
			*data = value;

			index++;
			if (index == length) {
				doCallback = true;
				index = 0;
			}
			setIntegerParam(NDPluginSourceIndex, index);
		}

		this->unlock();

	} else {
        /* If this parameter belongs to a base class call its method */
        if (function < FIRST_NDPLUGIN_SOURCE_PARAM)
        	status = NDPluginDriver::writeFloat64(pasynUser, value);
    }

    /* Do callbacks so higher layers see any changes */
    status = (asynStatus) callParamCallbacks();

    if (doCallback) {

        /* Call the base class method */
        NDPluginDriver::processCallbacks(pArray);

        asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
				"%s:%s:, calling array callbacks\n", driverName,
				__func__);

		this->lock();

		epicsInt32 imageCounter;
		epicsTimeStamp startTime;
		epicsTimeGetCurrent(&startTime);
		getIntegerParam(NDArrayCounter, &imageCounter);
		imageCounter++;

		/* Put the frame number and time stamp into the buffer */
		pArray->uniqueId = imageCounter;
		pArray->timeStamp = startTime.secPastEpoch + startTime.nsec / 1.e9;
		this->unlock();

		doCallbacksGenericPointer(pArray, NDArrayData, 0);

		this->lock();
		pArray->release();
		pArray = NULL;
		this->unlock();
    }

    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s: error, status=%d function=%d, value=%f\n", 
              driverName, __func__, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%f\n", 
              driverName, __func__, function, value);
    return status;
}

/** Constructor for NDPluginSource; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
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
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
NDPluginSource::NDPluginSource(const char *portName, int queueSize, int blockingCallbacks,
                         const char *NDArrayPort, int NDArrayAddr,
                         int maxBuffers, size_t maxMemory,
                         int priority, int stackSize)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, NUM_NDPLUGIN_SOURCE_PARAMS, maxBuffers, maxMemory,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   0, 1, priority, stackSize),
      pArray(NULL),
      doRun(false)
{
    createParam(NDPluginSourceExtValueString, asynParamFloat64,    &NDPluginSourceExtValue);
    createParam(NDPluginSourceSourceString,   asynParamOctet,      &NDPluginSourceSource);
    createParam(NDPluginSourceIndexString,    asynParamInt32,      &NDPluginSourceIndex);
    createParam(NDPluginSourceLengthString,   asynParamInt32,      &NDPluginSourceLength);

    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginSource");

    /* Try to connect to the array port */
    connectToArrayPort();
}

/** Configuration command */
extern "C" int NDSourceConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                 const char *NDArrayPort, int NDArrayAddr,
                                 int maxBuffers, size_t maxMemory,
                                 int priority, int stackSize)
{
	//printf("%s: ENTER\n", __func__);
    NDPluginSource *plugin = new NDPluginSource(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                      maxBuffers, maxMemory, priority, stackSize);
    return plugin->start();
}

/* EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArrayPort",iocshArgString};
static const iocshArg initArg4 = { "NDArrayAddr",iocshArgInt};
static const iocshArg initArg5 = { "maxBuffers",iocshArgInt};
static const iocshArg initArg6 = { "maxMemory",iocshArgInt};
static const iocshArg initArg7 = { "priority",iocshArgInt};
static const iocshArg initArg8 = { "stackSize",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8};
static const iocshFuncDef initFuncDef = {"NDSourceConfigure",9,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDSourceConfigure(args[0].sval, args[1].ival, args[2].ival,
                     args[3].sval, args[4].ival, args[5].ival,
                     args[6].ival, args[7].ival, args[8].ival);
}

extern "C" void NDSourceRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDSourceRegister);
}
