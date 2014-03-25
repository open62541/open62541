from __future__ import print_function

def setup():
	config = dict()
	config["pluginType"] = "structuredObject";
	config["tagName"] = "Node";
	config["functionCall"] = "createElement(element, fc, fh)"
	return config

def createElement(element, fc, fh):
	print("/* Hello from plugin */", end='\n', file=fc)
	#return "default" to execute the default createElement
	return "default"
