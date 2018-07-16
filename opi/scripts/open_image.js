importPackage(Packages.org.csstudio.opibuilder.scriptUtil);

var open_selector = "open_selector"

var PREFIX_selector = "prefix_selector"
var DEVICE_selector = "device_selector_image"
var IMAGE_selector = "image_selector"

var macros = DataUtil.createMacrosInput(true);

var PREFIX = display.getWidget(PREFIX_selector).getPropertyValue("text")
var DEVICE = display.getWidget(DEVICE_selector).getPropertyValue("text")
var IMAGE = display.getWidget(IMAGE_selector).getPropertyValue("text")

macros.put("P", PREFIX)
macros.put("R", DEVICE)
macros.put("D", IMAGE)

var openmode = display.getWidget(open_selector).getValue()
if (openmode == "New Window")
  openmode = 2
else
  openmode = 0

ScriptUtil.openOPI(widget, "admisc_imageview.opi", openmode, macros)
