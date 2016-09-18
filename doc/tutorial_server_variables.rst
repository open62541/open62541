.. role:: ccode(code)
      :language: c

Adding variables to a server
----------------------------

This tutorial shows how to add variable nodes to a server and how these can be
connected to a physical process in the background.

This is the code for a server with a single variable node holding an integer. We
will take this example to explain some of the fundamental concepts of open62541.

.. literalinclude:: ${PROJECT_SOURCE_DIR}/examples/server_variable.c
   :language: c
   :linenos:
   :lines: 4,13,15-

Variants and Datatypes
^^^^^^^^^^^^^^^^^^^^^^

The datatype :ref:`variant` belongs to the built-in datatypes of OPC UA and is
used as a container type. A variant can hold any other datatype as a scalar
(except variant) or as an array. Array variants can additionally denote the
dimensionality of the data (e.g. a 2x3 matrix) in an additional integer array.

The `UA_VariableAttributes` type contains a variant member `value`. The command
:ccode:`UA_Variant_setScalar(&attr.value, &myInteger,
&UA_TYPES[UA_TYPES_INT32])` sets the variant to point to the integer. Note that
this does not make a copy of the integer (for which `UA_Variant_setScalarCopy`
can be used). The variant (and its content) is then copied into the newly
created node.

The above code could have used allocations by making copies of all entries of
the attribute variant. Then, of course, the variant content needs to be deleted
to prevent memleaks.

.. code-block:: c

    UA_VariableAttributes attr;
    UA_VariableAttributes_init(&attr);
    UA_Int32 myInteger = 42;
    UA_Variant_setScalarCopy(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT_ALLOC("en_US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT_ALLOC("en_US","the answer");

    /* add the variable node here */
    UA_VariableAttributes_deleteMembers(&attr); /* free the allocated memory */

Finally, one needs to tell the server where to add the new variable in the
information model. For that, we state the NodeId of the parent node and the
(hierarchical) reference to the parent node.

NodeIds
^^^^^^^

A :ref:`nodeid` is a unique identifier in server's context. It contains the
number of node's namespace and either a numeric, string or GUID (Globally
Unique ID) identifier. The following are some examples for their usage.

.. code-block:: c

   UA_NodeId id1 = UA_NODEID_NUMERIC(1, 1234);

   UA_NodeId id2 = UA_NODEID_STRING(1, "testid"); /* points to the static string */

   UA_NodeId id3 = UA_NODEID_STRING_ALLOC(1, "testid"); /* copy to memory */
   UA_NodeId_deleteMembers(&id3); /* free the allocated string */


Adding a Variable with an external Data Source
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In order to couple a running process to a variable, a :ref:`datasource` is used.
Consider ``examples/server_datasource.c`` in the repository for an example.
