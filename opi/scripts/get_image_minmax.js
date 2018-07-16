importPackage(Packages.org.csstudio.opibuilder.scriptUtil);

var minimum_widget = "image_minimum"
var maximum_widget = "image_maximum"
var graph_widget = "image_graph"

var manufacturer = PVUtil.getString(pvs[0]);
var model = PVUtil.getString(pvs[1]);
var minimum = 0
var maximum = 256
if (model == "GT3300" || model == "Luc285_MONO") maximum = 16383

display.getWidget(minimum_widget).setPropertyValue("text", minimum)
display.getWidget(maximum_widget).setPropertyValue("text", maximum)

var minimum = display.getWidget(minimum_widget).getPropertyValue("text")
var maximum = display.getWidget(maximum_widget).getPropertyValue("text")

display.getWidget(graph_widget).setPropertyValue("minimum", minimum);
display.getWidget(graph_widget).setPropertyValue("maximum", maximum);
