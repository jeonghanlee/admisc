importPackage(Packages.org.csstudio.opibuilder.scriptUtil);

var container = "image_container"

var PREFIX_selector = "prefix_selector"
var DEVICE_selector = "device_selector"
var IMAGE_selector = "image_selector"

var macros = DataUtil.createMacrosInput(true);

var PREFIX = display.getWidget(PREFIX_selector).getPropertyValue("text")
var DEVICE = display.getWidget(DEVICE_selector).getPropertyValue("text")
var IMAGE = display.getWidget(IMAGE_selector).getPropertyValue("text")

macros.put("P", PREFIX)
macros.put("R", DEVICE)
macros.put("D", IMAGE)

display.getWidget(container).setPropertyValue("macros", macros)

display.getWidget(container).setPropertyValue("opi_file", "");

display.getWidget(container).setPropertyValue("opi_file", "admisc_imageview.opi");
