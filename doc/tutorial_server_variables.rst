Adding nodes to a server and connecting nodes to user-defined values
====================================================================

This tutorial shows how to add variable nodes to a server and how these can be connected to user-defined values and callbacks.

Firstly, we need to introduce a concept of Variants. This is a data structure able to hold any datatype.

Variants
--------
The datatype UA_Variant a belongs to the built-in datatypes of OPC UA and is used as a container type. A Variant can hold any other built-in scalar datatype (except Variants) or array built-in datatype (array of variants too). The variant is structured like this in open62541:

.. code-block:: c

	typedef struct {
		const UA_DataType *type; ///< The nodeid of the datatype
		enum {
		    UA_VARIANT_DATA, ///< The data is "owned" by this variant (copied and deleted together)
		    UA_VARIANT_DATA_NODELETE, /**< The data is "borrowed" by the variant and shall not be
		                                   deleted at the end of this variant's lifecycle. It is not
		                                   possible to overwrite borrowed data due to concurrent access.
		                                   Use a custom datasource with a mutex. */
		} storageType; ///< Shall the data be deleted together with the variant
		UA_Int32  arrayLength;  ///< the number of elements in the data-pointer
		void     *data; ///< points to the scalar or array data
		UA_Int32  arrayDimensionsSize; ///< the number of dimensions the data-array has
		UA_Int32 *arrayDimensions; ///< the length of each dimension of the data-array
	} UA_Variant;

The members of the struct are

* type: a pointer to the vtable entry. It points onto the functions which handles the stored data i.e. encode/decode etc.

* storageType:  used to declare who the owner of data is and to which lifecycle it belongs. Three different cases are possible:

 * UA_VARIANT_DATA: this is the simplest case. The data belongs to the variant, which means if the variant is deleted so does the data which is kept inside
 
 * UA_VARIANT_DATASOURCE: in this case user-defined functions are called to access the data. The signature of the functions is defined by UA_VariantDataSource structure. A use-case could be to access a sensor only when the data is asked by some client

* arrayLength: length of the array (-1 if a scalar is saved)

* data: raw pointer to the saved vale or callback

* arrayDimensionsSize: size of arrayDimensions array

* arrayDimensions: dimensinos array in case the array is interpreted as a multi-dimensional construction, e.g., [5,5] for a 5x5 matrix

Adding a variable node to the server that contains a user-defined variable
--------------------------------------------------------------------------

This simple case allows to 'inject' a pre-defined variable into a variable node. The variable is wrapped by a "UA_Variant" before being insterted into the node.

Consider 'examples/server_variable.c' in the repository. The examples are compiled if the Cmake option UA_BUILD_EXAMPLE is turned on.

Adding a variable node to the server that contains a user-defined callback
--------------------------------------------------------------------------

The latter case allows to define callback functions that are executed on read or write of the node. In this case an "UA_DataSource" containing the respective callback pointer is intserted into the node.

Consider 'examples/server_datasource.c' in the repository. The examples are compiled if the Cmake option UA_BUILD_EXAMPLE is turned on.
