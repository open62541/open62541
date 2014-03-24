from __future__ import print_function

def setup():
	config = dict()
	config["pluginType"] = "structuredObjectCreation";
	config["tagName"] = "Node";
	return config

def createElement(element, fc, fh):
	print("/* Hello from plugin */", end='\n', file=fc)
	return "continue"
