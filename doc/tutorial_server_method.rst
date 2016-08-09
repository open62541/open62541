3. Adding a server-side method
==============================

This tutorial demonstrates how to add method nodes to the server. Use an UA
client, e.g., UaExpert to call the method (right-click on the method node ->
call).

The first example shows how to define input and output arguments (lines 72 - 88),
make the method executable (lines 94,95), add the method node (line 96-101)
with a specified method callback (lines 10 - 24).

The second example shows that a method can also be applied on an array
as input argument and output argument.

The last example presents a way to bind a new method callback to an already
instantiated method node.


.. literalinclude:: ../../examples/server_method.c
   :language: c
   :linenos:
   :lines: 4,5,14,16-
