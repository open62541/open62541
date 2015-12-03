Data Types
==========

Introduction
------------

In open62541, all data types share the same basic API for creation, copying and
deletion. The following functions are present for all data types ``T``.

``void T_init(T *ptr)``
  Initialize the data type. This is synonymous with zeroing out the memory, i.e. *memset(dataptr, 0, sizeof(T))*.
``T* T_new()``
  Allocate and return the memory for the data type. The memory is already initialized.
``UA_StatusCode T_copy(const T *src, T *dst)``
  Copy the content of the data type. Returns *UA_STATUSCODE_GOOD* if it succeeded.
``void T_deleteMembers(T *ptr)``
  Delete the dynamically allocated content of the data type, but not the data type itself.
``void T_delete(T *ptr)``
  Delete the content of the data type and the memory for the data type itself.

Here's a small example for the UA_String data type. UA_String will be introduced
in more detail later on, but you should be able to follow the example already.

.. code-block:: c

  /* The definition of UA_String copied from ua_types.h */ 
  typedef struct {
      size_t length; ///< The length of the string
      UA_Byte *data; ///< The string's content (not null-terminated)
  } UA_String;

  UA_String s1 = UA_STRING("test1");       /* s1 points to the statically allocated string buffer */
  UA_String_init(&s1);                     /* Reset s1 (no memleak due to the statically allocated buffer) */
  
  UA_String s2 = UA_STRING_ALLOC("test2"); /* s2 points to a new copy of the string buffer (with malloc) */
  UA_String_deleteMembers(&s2);            /* Free the content of s2, but not s2 itself */
  
  UA_String *s3 = UA_String_new();         /* The string s3 is malloced and initialized */
  *s3 = UA_STRING_ALLOC("test3");          /* s3 points to a new copy of the string buffer */
  
  UA_String s4;
  UA_copy(s3, &s4);                        /* Copy the content of s3 to s4 */
  
  UA_String_delete(s3);                    /* Free the string buffer and the string itself */
  UA_String_deleteMembers(&s4);            /* Again, delete only the string buffer */

The builtin data types
----------------------

OPC UA defines 25 builtin data types. All other data types are combinations of
the 25 builtin data types.

*TODO*

Generic Data Type Handling
--------------------------

All standard-defined types are described with an ``UA_DataType`` structure.

.. doxygenstruct:: UA_DataType
   :members:

.. c:var:: const UA_DataType UA_TYPES[UA_TYPES_COUNT]

  The datatypes defined in the standard are stored in the ``UA_TYPES`` array.
  A typical function call is ``UA_Array_new(&data_ptr, 20, &UA_TYPES[UA_TYPES_STRING])``.

.. doxygenfunction:: UA_new
.. doxygenfunction:: UA_init
.. doxygenfunction:: UA_copy
.. doxygenfunction:: UA_deleteMembers
.. doxygenfunction:: UA_delete

For all datatypes, there are also macros with syntactic sugar over calling the
generic functions with a pointer into the ``UA_TYPES`` array.

.. c:function:: <typename>_new()

  Allocates the memory for the type and runs _init on the returned variable.
  Returns null if no memory could be allocated.

.. c:function:: <typename>_init(<typename> *value)

  Sets all members of the type to a default value, usually zero. Arrays (e.g.
  for strings) are set to a length of -1.

.. c:function:: <typename>_copy(<typename> *src, <typename> *dst)

  Copies a datatype. This performs a deep copy iterating over the members.
  Copying into variants with an external data source is not permitted. If
  copying fails, a deleteMembers is performed and an error code returned.

.. c:function:: <typename>_deleteMembers(<typename> *value)

   Frees the memory of dynamically sized members of a datatype (e.g. arrays).

.. c:function:: <typename>_delete(<typename> *value)

   Frees the memory of the datatype and its dynamically allocated members.

Array Handling
--------------
   
.. doxygenfunction:: UA_Array_new
.. doxygenfunction:: UA_Array_copy
.. doxygenfunction:: UA_Array_delete
