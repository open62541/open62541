/* ========================================================================
 * Copyright (c) 2005-2020 The OPC Foundation, Inc. All rights reserved.
 *
 * OPC Foundation MIT License 1.00
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * The complete license agreement can be found here:
 * http://opcfoundation.org/License/MIT/1.00/
 * ======================================================================*/

#ifndef _OpcUa_Attributes_H_
#define _OpcUa_Attributes_H_ 1

OPCUA_BEGIN_EXTERN_C

/*============================================================================
 * The canonical identifier for the node.
 *===========================================================================*/
#define OpcUa_Attributes_NodeId 1

/*============================================================================
 * The class of the node.
 *===========================================================================*/
#define OpcUa_Attributes_NodeClass 2

/*============================================================================
 * A non-localized, human readable name for the node.
 *===========================================================================*/
#define OpcUa_Attributes_BrowseName 3

/*============================================================================
 * A localized, human readable name for the node.
 *===========================================================================*/
#define OpcUa_Attributes_DisplayName 4

/*============================================================================
 * A localized description for the node.
 *===========================================================================*/
#define OpcUa_Attributes_Description 5

/*============================================================================
 * Indicates which attributes are writable.
 *===========================================================================*/
#define OpcUa_Attributes_WriteMask 6

/*============================================================================
 * Indicates which attributes are writable by the current user.
 *===========================================================================*/
#define OpcUa_Attributes_UserWriteMask 7

/*============================================================================
 * Indicates that a type node may not be instantiated.
 *===========================================================================*/
#define OpcUa_Attributes_IsAbstract 8

/*============================================================================
 * Indicates that forward and inverse references have the same meaning.
 *===========================================================================*/
#define OpcUa_Attributes_Symmetric 9

/*============================================================================
 * The browse name for an inverse reference.
 *===========================================================================*/
#define OpcUa_Attributes_InverseName 10

/*============================================================================
 * Indicates that following forward references within a view will not cause a loop.
 *===========================================================================*/
#define OpcUa_Attributes_ContainsNoLoops 11

/*============================================================================
 * Indicates that the node can be used to subscribe to events.
 *===========================================================================*/
#define OpcUa_Attributes_EventNotifier 12

/*============================================================================
 * The value of a variable.
 *===========================================================================*/
#define OpcUa_Attributes_Value 13

/*============================================================================
 * The node id of the data type for the variable value.
 *===========================================================================*/
#define OpcUa_Attributes_DataType 14

/*============================================================================
 * The number of dimensions in the value.
 *===========================================================================*/
#define OpcUa_Attributes_ValueRank 15

/*============================================================================
 * The length for each dimension of an array value.
 *===========================================================================*/
#define OpcUa_Attributes_ArrayDimensions 16

/*============================================================================
 * How a variable may be accessed.
 *===========================================================================*/
#define OpcUa_Attributes_AccessLevel 17

/*============================================================================
 * How a variable may be accessed after taking the user's access rights into account.
 *===========================================================================*/
#define OpcUa_Attributes_UserAccessLevel 18

/*============================================================================
 * Specifies (in milliseconds) how fast the server can reasonably sample the value for changes.
 *===========================================================================*/
#define OpcUa_Attributes_MinimumSamplingInterval 19

/*============================================================================
 * Specifies whether the server is actively collecting historical data for the variable.
 *===========================================================================*/
#define OpcUa_Attributes_Historizing 20

/*============================================================================
 * Whether the method can be called.
 *===========================================================================*/
#define OpcUa_Attributes_Executable 21

/*============================================================================
 * Whether the method can be called by the current user.
 *===========================================================================*/
#define OpcUa_Attributes_UserExecutable 22

/*============================================================================
 * Provides the metadata and encoding information for custom DataTypes.
 *===========================================================================*/
#define OpcUa_Attributes_DataTypeDefinition 23

/*============================================================================
 * The permissions for the node granted to roles.
 *===========================================================================*/
#define OpcUa_Attributes_RolePermissions 24

/*============================================================================
 * The subset of permissions available for the roles available to the current session.
 *===========================================================================*/
#define OpcUa_Attributes_UserRolePermissions 25

/*============================================================================
 * The access restrictions assigned to the node.
 *===========================================================================*/
#define OpcUa_Attributes_AccessRestrictions 26

/*============================================================================
 * How a variable may be accessed.
 *===========================================================================*/
#define OpcUa_Attributes_AccessLevelEx 27

OPCUA_END_EXTERN_C

#endif /* _OpcUa_Attributes_H_ */
/* This is the last line of an autogenerated file. */
