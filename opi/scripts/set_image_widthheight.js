importPackage(Packages.org.csstudio.opibuilder.scriptUtil);

var width = PVUtil.getDouble(pvs[0]);
var height = PVUtil.getDouble(pvs[1]);

widget.setPropertyValue("data_width", width);
widget.setPropertyValue("data_height", height);

widget.setPropertyValue("x_axis_maximum", width);
widget.setPropertyValue("y_axis_maximum", height);