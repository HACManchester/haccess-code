# Introduction

The configuration file format is IniFile style, with various sections supplied
depending on the mode of the device.

Times are specified in an integer plus the unit, which are as follows:

      cs = centi-seconds
      ms = milli-seconds
      s = seconds
      m = minutes
      h = hours

# Internal logic

The logic nodes are built up from a set of simple logic blocks which act
on their inputs (dependencies) to form new outputs. The format is such that
it should be easy to write most of the functionality for each unit into the
configuration without having to put special software onto each unit.

There are some pre-built input and outputs created by the software for the
standard internal resources such as the buttons, rfid sensor and optical
isolator built as default.

The sections for the logic are read in order they appear in the .ini file
and start with an unique identifier. The parse recognises any of these
sections as logic.

	 input		to be used for inputs (mqtt, etc)
	 output		outputs, such as mqtt notifications
	 logic		intermediate logic sections
	 dependency	create a dependency link

The dependency nodes are included so that new dependencies can be created
after the initial nodes in case there are complicated links. These nodes
contain the same linking information as an output or logic node.

Input nodes are mostly related to extending the standard inputs.

# Example nodes

<pre>
[input]
name=input		;; name for node (debug, links, etc)
type=mqtt_in		;; this is an mqtt input node
topic=mqtt_topic	;; the mqtt topic to use
default=false		;; default (boolean) valye

[output]
name=output		;; see input node
type=mqtt_out		;; this is an mqtt output node
topic=mqtt_topic	;; the mqtt topic to use

[logic]
type=and		;; this is an 'and' node
name=and_node		;; the sname (debug, links, etc);
source1=input_a		;; the first input
source2=input_b		;; the second input
</pre>

An example node pipeline:

<pre>
[input1]
name=input1
type=mqtt_in
topic=in1

[input2]
name=input1
type=mqtt_in
topic=in2

[logic1]
name=or1
type=or
source1=input1
source2=input2

[logic2]
name=and1
type=and
source1=input1
source2=input2
</pre>

The above creates two inputs, and then two logic nodes which depends on
those two inputs.

Standard node (for all but special nodes) configuration file keys:

name=	This is the name given to the node, this is used for the debug
	output and for linking the node to other nodes.

type=	The type for the node, such as input, output, logic, timer or
	other information.

source{x}= The dependency source for the node. These are numbered 1 up.
	These keys are read until the read fails, so should be numbered
 	in sequence 1..n with no breaks.

topic=	The mqtt topic to publish.


## Timer nodes

time=	The time to remain on after the input has finished triggering
	the node.

Note, the timer nodes do not yet support edge trigger.

## MQTT nodes

topic=	The mqtt topic to either respond to, or publish to.


## Input nodes

default= The default value when the node is create.

