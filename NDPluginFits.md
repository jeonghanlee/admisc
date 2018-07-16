# Quick use guide for areaDetector Fits plugin (NDPluginFits) under EEE

Setup:

 We want to do gaussian fit over three peaks.

 We have 1004 x 1002 detector.
 
 CSS is used to interact with the IOC.
 
 AD core and detector specific AD OPI need to be in CSS workspace.

## Editing st.cmd

Add require line:

	require admisc

Add directives to configure NDPluginFits plugin and load NDFits.template:

	NDFitsConfigure("FITS1", $(QSIZE), 0, "$(PORT)", 0, 3, 0, 0, 0)
	dbLoadRecords("NDFits.template",      "P=$(PREFIX),R=Fits1:,   PORT=FITS1,  ADDR=0,TIMEOUT=1,XSIZE=$(XSIZE),YSIZE=$(YSIZE),NCHANS=$(NCHANS),NDARRAY_PORT=$(PORT)")

Add directives to load NDPluginFits NDFitsN.template, three times, one for each peak:

	dbLoadRecords("NDFitsN.template",     "P=$(PREFIX),R=Fits1:1:, PORT=FITS1,  ADDR=0,TIMEOUT=1,NCHANS=$(NCHANS)")
	dbLoadRecords("NDFitsN.template",     "P=$(PREFIX),R=Fits1:2:, PORT=FITS1,  ADDR=1,TIMEOUT=1,NCHANS=$(NCHANS)")
	dbLoadRecords("NDFitsN.template",     "P=$(PREFIX),R=Fits1:3:, PORT=FITS1,  ADDR=2,TIMEOUT=1,NCHANS=$(NCHANS)")

## Starting the IOC

	iocsh st.cmd

## Using the supplied CSS OPI

NDFits.opi provides control over fitting process. It also displays experimental data and result of fit.

Note:
> Stock ADPlugins.opi may need to be edited to include of the NDFits, for example under 'Others' menu button.

In main screen setup the detector acquisition parameters as needed.

Also enable the Array callbacks and start the acquisition.

### Setup ROI plugin

Setup ROI plugin ROI1 to bin the Y to 1 pixel wide (Binning Y = 1002).

Enable scaling and set scale divisor to the same value as Binning Y (1002).

Setup the X ROI start and ROI size to encompass three peaks of the experimental data.

Note:
> You can see the experimental data in the Fits plugin plot widget or use Stats Profile at Cursor X plot to assess where the peaks are.

Enable the ROI1 plugin.

### Setup the Fits plugin

Setup Fits plugin FITS1 to use ROI1 plugin as its data input.

Fill in the intial peak Amplitude, Mu and Sigma values.

You can leave background at 0.

Enable the FITS1 plugin.

Note
> You can use the 'Show Value Labels' button of the plot widget to get Amplitude and Mu values for the Fits plugin. For Sigma use rule of thumb value by inspecting the plot widget.

At this point you should see the experimental data as black trace, and if fit was successfull, fit result as red trace.

Resulting Amplitude, Mu and Sigma of fit are seen next to initial values.

