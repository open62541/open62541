/* ========================================================================
 * Copyright (c) 2005-2019 The OPC Foundation, Inc. All rights reserved.
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

using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;
using System.Runtime.Serialization;
using Opc.Ua.Di;
using Opc.Ua;

namespace Opc.Ua.Robotics
{
    #region ExecutionModeEnumeration Enumeration
    #if (!OPCUA_EXCLUDE_ExecutionModeEnumeration)
    /// <summary>
    /// 
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Opc.Ua.Robotics.Namespaces.OpcUaRoboticsXsd)]
    public enum ExecutionModeEnumeration
    {
        /// <remarks />
        [EnumMember(Value = "CYCLE_0")]
        CYCLE = 0,

        /// <remarks />
        [EnumMember(Value = "CONTINUOUS_1")]
        CONTINUOUS = 1,

        /// <remarks />
        [EnumMember(Value = "STEP_2")]
        STEP = 2,
    }

    #region ExecutionModeEnumerationCollection Class
    /// <summary>
    /// A collection of ExecutionModeEnumeration objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfExecutionModeEnumeration", Namespace = Opc.Ua.Robotics.Namespaces.OpcUaRoboticsXsd, ItemName = "ExecutionModeEnumeration")]
    #if !NET_STANDARD
    public partial class ExecutionModeEnumerationCollection : List<ExecutionModeEnumeration>, ICloneable
    #else
    public partial class ExecutionModeEnumerationCollection : List<ExecutionModeEnumeration>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public ExecutionModeEnumerationCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public ExecutionModeEnumerationCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public ExecutionModeEnumerationCollection(IEnumerable<ExecutionModeEnumeration> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator ExecutionModeEnumerationCollection(ExecutionModeEnumeration[] values)
        {
            if (values != null)
            {
                return new ExecutionModeEnumerationCollection(values);
            }

            return new ExecutionModeEnumerationCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator ExecutionModeEnumeration[](ExecutionModeEnumerationCollection values)
        {
            if (values != null)
            {
                return values.ToArray();
            }

            return null;
        }
        #endregion

        #if !NET_STANDARD
        #region ICloneable Methods
        /// <summary>
        /// Creates a deep copy of the collection.
        /// </summary>
        public object Clone()
        {
            return (ExecutionModeEnumerationCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            ExecutionModeEnumerationCollection clone = new ExecutionModeEnumerationCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((ExecutionModeEnumeration)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion

    #region MotionDeviceCategoryEnumeration Enumeration
    #if (!OPCUA_EXCLUDE_MotionDeviceCategoryEnumeration)
    /// <summary>
    /// 
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Opc.Ua.Robotics.Namespaces.OpcUaRoboticsXsd)]
    public enum MotionDeviceCategoryEnumeration
    {
        /// <remarks />
        [EnumMember(Value = "OTHER_0")]
        OTHER = 0,

        /// <remarks />
        [EnumMember(Value = "ARTICULATED_ROBOT_1")]
        ARTICULATED_ROBOT = 1,

        /// <remarks />
        [EnumMember(Value = "SCARA_ROBOT_2")]
        SCARA_ROBOT = 2,

        /// <remarks />
        [EnumMember(Value = "CARTESIAN_ROBOT_3")]
        CARTESIAN_ROBOT = 3,

        /// <remarks />
        [EnumMember(Value = "SPHERICAL_ROBOT_4")]
        SPHERICAL_ROBOT = 4,

        /// <remarks />
        [EnumMember(Value = "PARALLEL_ROBOT_5")]
        PARALLEL_ROBOT = 5,

        /// <remarks />
        [EnumMember(Value = "CYLINDRICAL_ROBOT_6")]
        CYLINDRICAL_ROBOT = 6,
    }

    #region MotionDeviceCategoryEnumerationCollection Class
    /// <summary>
    /// A collection of MotionDeviceCategoryEnumeration objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfMotionDeviceCategoryEnumeration", Namespace = Opc.Ua.Robotics.Namespaces.OpcUaRoboticsXsd, ItemName = "MotionDeviceCategoryEnumeration")]
    #if !NET_STANDARD
    public partial class MotionDeviceCategoryEnumerationCollection : List<MotionDeviceCategoryEnumeration>, ICloneable
    #else
    public partial class MotionDeviceCategoryEnumerationCollection : List<MotionDeviceCategoryEnumeration>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public MotionDeviceCategoryEnumerationCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public MotionDeviceCategoryEnumerationCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public MotionDeviceCategoryEnumerationCollection(IEnumerable<MotionDeviceCategoryEnumeration> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator MotionDeviceCategoryEnumerationCollection(MotionDeviceCategoryEnumeration[] values)
        {
            if (values != null)
            {
                return new MotionDeviceCategoryEnumerationCollection(values);
            }

            return new MotionDeviceCategoryEnumerationCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator MotionDeviceCategoryEnumeration[](MotionDeviceCategoryEnumerationCollection values)
        {
            if (values != null)
            {
                return values.ToArray();
            }

            return null;
        }
        #endregion

        #if !NET_STANDARD
        #region ICloneable Methods
        /// <summary>
        /// Creates a deep copy of the collection.
        /// </summary>
        public object Clone()
        {
            return (MotionDeviceCategoryEnumerationCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            MotionDeviceCategoryEnumerationCollection clone = new MotionDeviceCategoryEnumerationCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((MotionDeviceCategoryEnumeration)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion

    #region AxisMotionProfileEnumeration Enumeration
    #if (!OPCUA_EXCLUDE_AxisMotionProfileEnumeration)
    /// <summary>
    /// 
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Opc.Ua.Robotics.Namespaces.OpcUaRoboticsXsd)]
    public enum AxisMotionProfileEnumeration
    {
        /// <remarks />
        [EnumMember(Value = "OTHER_0")]
        OTHER = 0,

        /// <remarks />
        [EnumMember(Value = "ROTARY_1")]
        ROTARY = 1,

        /// <remarks />
        [EnumMember(Value = "ROTARY_ENDLESS_2")]
        ROTARY_ENDLESS = 2,

        /// <remarks />
        [EnumMember(Value = "LINEAR_3")]
        LINEAR = 3,

        /// <remarks />
        [EnumMember(Value = "LINEAR_ENDLESS_4")]
        LINEAR_ENDLESS = 4,
    }

    #region AxisMotionProfileEnumerationCollection Class
    /// <summary>
    /// A collection of AxisMotionProfileEnumeration objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfAxisMotionProfileEnumeration", Namespace = Opc.Ua.Robotics.Namespaces.OpcUaRoboticsXsd, ItemName = "AxisMotionProfileEnumeration")]
    #if !NET_STANDARD
    public partial class AxisMotionProfileEnumerationCollection : List<AxisMotionProfileEnumeration>, ICloneable
    #else
    public partial class AxisMotionProfileEnumerationCollection : List<AxisMotionProfileEnumeration>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public AxisMotionProfileEnumerationCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public AxisMotionProfileEnumerationCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public AxisMotionProfileEnumerationCollection(IEnumerable<AxisMotionProfileEnumeration> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator AxisMotionProfileEnumerationCollection(AxisMotionProfileEnumeration[] values)
        {
            if (values != null)
            {
                return new AxisMotionProfileEnumerationCollection(values);
            }

            return new AxisMotionProfileEnumerationCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator AxisMotionProfileEnumeration[](AxisMotionProfileEnumerationCollection values)
        {
            if (values != null)
            {
                return values.ToArray();
            }

            return null;
        }
        #endregion

        #if !NET_STANDARD
        #region ICloneable Methods
        /// <summary>
        /// Creates a deep copy of the collection.
        /// </summary>
        public object Clone()
        {
            return (AxisMotionProfileEnumerationCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            AxisMotionProfileEnumerationCollection clone = new AxisMotionProfileEnumerationCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((AxisMotionProfileEnumeration)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion

    #region OperationalModeEnumeration Enumeration
    #if (!OPCUA_EXCLUDE_OperationalModeEnumeration)
    /// <summary>
    /// 
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Opc.Ua.Robotics.Namespaces.OpcUaRoboticsXsd)]
    public enum OperationalModeEnumeration
    {
        /// <remarks />
        [EnumMember(Value = "OTHER_0")]
        OTHER = 0,

        /// <remarks />
        [EnumMember(Value = "MANUAL_REDUCED_SPEED_1")]
        MANUAL_REDUCED_SPEED = 1,

        /// <remarks />
        [EnumMember(Value = "MANUAL_HIGH_SPEED_2")]
        MANUAL_HIGH_SPEED = 2,

        /// <remarks />
        [EnumMember(Value = "AUTOMATIC_3")]
        AUTOMATIC = 3,

        /// <remarks />
        [EnumMember(Value = "AUTOMATIC_EXTERNAL_4")]
        AUTOMATIC_EXTERNAL = 4,
    }

    #region OperationalModeEnumerationCollection Class
    /// <summary>
    /// A collection of OperationalModeEnumeration objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfOperationalModeEnumeration", Namespace = Opc.Ua.Robotics.Namespaces.OpcUaRoboticsXsd, ItemName = "OperationalModeEnumeration")]
    #if !NET_STANDARD
    public partial class OperationalModeEnumerationCollection : List<OperationalModeEnumeration>, ICloneable
    #else
    public partial class OperationalModeEnumerationCollection : List<OperationalModeEnumeration>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public OperationalModeEnumerationCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public OperationalModeEnumerationCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public OperationalModeEnumerationCollection(IEnumerable<OperationalModeEnumeration> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator OperationalModeEnumerationCollection(OperationalModeEnumeration[] values)
        {
            if (values != null)
            {
                return new OperationalModeEnumerationCollection(values);
            }

            return new OperationalModeEnumerationCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator OperationalModeEnumeration[](OperationalModeEnumerationCollection values)
        {
            if (values != null)
            {
                return values.ToArray();
            }

            return null;
        }
        #endregion

        #if !NET_STANDARD
        #region ICloneable Methods
        /// <summary>
        /// Creates a deep copy of the collection.
        /// </summary>
        public object Clone()
        {
            return (OperationalModeEnumerationCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            OperationalModeEnumerationCollection clone = new OperationalModeEnumerationCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((OperationalModeEnumeration)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion
}