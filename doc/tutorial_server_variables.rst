.. role:: ccode(code)
      :language: c

2. Adding variables to a server
===============================

This tutorial shows how to add variable nodes to a server and how these can be
connected to a physical process in the background. Make sure to read the
:ref:`introduction <introduction>` first.

This is the code for a server with a single variable node holding an integer. We
will take this example to explain some of the fundamental concepts of open62541.

.. code-block:: c

    #include <signal.h>
    #include "open62541.h"

    UA_Boolean running = true;
    static void stopHandler(int sign) {
        running = false;
    }

    int main(int argc, char** argv) {
        signal(SIGINT, stopHandler); /* catch ctrl-c */

        UA_ServerConfig config = UA_ServerConfig_standard;
        UA_ServerNetworkLayer nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16664);
        config.networkLayers = &nl;
        config.networkLayersSize = 1;
        UA_Server *server = UA_Server_new(config);

        /* 1) Define the attribute of the myInteger variable node */
        UA_VariableAttributes attr;
        UA_VariableAttributes_init(&attr);
        UA_Int32 myInteger = 42;
        UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
        attr.description = UA_LOCALIZEDTEXT("en_US","the answer");
        attr.displayName = UA_LOCALIZEDTEXT("en_US","the answer");

        /* 2) Add the variable node to the information model */
        UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
        UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
        UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
        UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
        UA_StatusCode retval = UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                                                         parentReferenceNodeId, myIntegerName,
                                                         UA_NODEID_NULL, attr, NULL, NULL);

        if(retval == UA_STATUSCODE_GOOD)
            UA_Server_run(server, &running);

        UA_Server_delete(server);
        nl.deleteMembers(&nl);

        return retval;
    }

Variants and Datatypes
----------------------

The datatype *variant* belongs to the built-in datatypes of OPC UA and is used
as a container type. A variant can hold any other datatype as a scalar (except
Variant) or as an array. Array variants can additionally denote the
dimensionality of the data (e.g. a 2x3 matrix) in an additional integer array.
You can find the code that defines the variant datatype :ref:`here <variant>`.

The `UA_VariableAttributes` type contains a variant member `value`. The command
:ccode:`UA_Variant_setScalar(&attr.value, &myInteger,
&UA_TYPES[UA_TYPES_INT32])` sets the variant to point to the integer. Note that
this does not make a copy of the integer (for which `UA_Variant_setScalarCopy`
can be used). The variant (and its content) is then copied into the newly
created node.

Since it is a bit involved to set variants by hand, there are four basic
functions you should be aware of:

  * **UA_Variant_setScalar** will set the contents of the variant to a pointer
    to the object that you pass with the call. Make sure to never deallocate
    that object while the variant exists!
  * **UA_Variant_setScalarCopy** will copy the object pointed to into a new
    object of the same type and attach that to the variant.
  * **UA_Variant_setArray** will set the contents of the variant to be an array
    and point to the exact pointer/object that you passed the call.
  * **UA_Variant_setArrayCopy** will create a copy of the array passed with the
    call.

The equivalent code using allocations is as follows:

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
-------

A node ID is a unique identifier in server's context. It is composed of two members:

+-------------+-----------------+---------------------------+
| Type        | Name            | Notes                     |
+=============+=================+===========================+
| UInt16      | namespaceIndex  |  Number of the namespace  |
+-------------+-----------------+---------------------------+
| Union       | identifier      |  One idenifier of the     |
|             |  * String       |  listed types             |
|             |  * Integer      |                           |
|             |  * GUID         |                           |
|             |  * ByteString   |                           |
+-------------+-----------------+---------------------------+

The first parameter is the number of node's namespace, the second one may be a
numeric, a string or a GUID (Globally Unique ID) identifier. The following are
some examples for their usage.

.. code-block:: c

   UA_NodeId id1 = UA_NODEID_NUMERIC(1, 1234);

   UA_NodeId id2 = UA_NODEID_STRING(1, "testid"); /* points to the static string */

   UA_NodeId id3 = UA_NODEID_STRING_ALLOC(1, "testid");
   UA_NodeId_deleteMembers(&id3); /* free the allocated string */

Adding a variable node to the server that contains a user-defined callback
--------------------------------------------------------------------------

The latter case allows to define callback functions that are executed on read or
write of the node. In this case an "UA_DataSource" containing the respective
callback pointer is intserted into the node.

Consider ``examples/server_datasource.c`` in the repository. The examples are
compiled if the Cmake option UA_BUILD_EXAMPLE is turned on.

Asserting success/failure
-------------------------

Almost all functions of the open62541 API will return a `StatusCode` 32-bit
integer. The actual statuscodes are defined :ref:`here <statuscodes>`. Normally,
the functions should return `UA_STATUSCODE_GOOD`, which maps to the zero
integer.
