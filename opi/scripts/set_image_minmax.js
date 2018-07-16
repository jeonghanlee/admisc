importPackage(Packages.org.csstudio.opibuilder.scriptUtil);

var minimum_widget = "image_minimum"
var maximum_widget = "image_maximum"
var graph_widget = "image_graph"

var minimum = display.getWidget(minimum_widget).getPropertyValue("text")
var maximum = display.getWidget(maximum_widget).getPropertyValue("text")

display.getWidget(graph_widget).setPropertyValue("minimum", minimum);
display.getWidget(graph_widget).setPropertyValue("maximum", maximum);
