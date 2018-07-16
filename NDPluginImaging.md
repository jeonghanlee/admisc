# Quick use guide for areaDetector Imaging plugin (NDPluginImaging) under EEE

## Editing st.cmd

Add require line:

    require admisc

Add directives to configure NDPluginImaging plugin and load NDImaging.template:

    epicsEnvSet("PREFIX",   "DEMO")
    epicsEnvSet("PORT",     "IMAG")
    epicsEnvSet("QSIZE",    "20")
    epicsEnvSet("NCHANS",   "20")
    epicsEnvSet("NELM",     "1000")

    NDImagingConfigure("IMAG1", $(QSIZE), 0, "$(PORT)", 0, 1, 0, 0, 0)
    dbLoadRecords("NDImaging.template", "P=$(PREFIX),R=Imag1:, PORT=$(PORT), ADDR=0,TIMEOUT=1,NELM=$(NELM),NCHANS=$(NCHANS),NDARRAY_PORT=$(PORT)")

## Starting the IOC

    iocsh st.cmd
