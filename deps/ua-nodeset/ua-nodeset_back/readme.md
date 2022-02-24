# OPC UA NodeSets and Other Supporting Files

## Contents

* [Files Found Here](#files-found-here)
* [Release Process](#release-process)
* [Error reporting](#error-reporting)

## Files Found Here

This repository contains UANodeSets and other normative files which are released with a specification.
Any change to a specification (release of a new version or errata) may require a change to the files in this repository.  

For each specification the following normative files need to be published:

* .NodeSet2.xml - The formal definition of the Nodes defined by the specification;
* .Types.xsd - The XML schema for the DataTypes defined by the specification;
* .Types.bsd - The OPC Binary schema for the DataTypes defined by the specification (obsolete);
* .NodeIds.csv - A CSV file containing the NodeIds assigned to Nodes defined by the specification;

In addition, the following non-normative support files may be published:

* .Classes.cs - C# classes for Nodes used with the [.NETStandard](https://github.com/OPCFoundation/UA-.NETStandard) stack;
* .DataTypes.cs - C# classes for DataTypes used with the [.NETStandard](https://github.com/OPCFoundation/UA-.NETStandard) stack;
* .Constants.cs - C# constant declarations used with the [.NETStandard](https://github.com/OPCFoundation/UA-.NETStandard) stack;
* .PredefinedNodes.uanodes - A non-normative binary representation of the UANodeSet for use with [.NETStandard](https://github.com/OPCFoundation/UA-.NETStandard) stack;

If a companion specification working group uses the ModelCompiler to create the UANodeSet then the following files are published:

* .Model.xml - The Nodes defined by the specification using the [schema](https://github.com/OPCFoundation/UA-ModelCompiler/blob/master/ModelCompiler/UA%20Model%20Design.xsd) needed by the [ModelCompiler](https://github.com/OPCFoundation/UA-ModelCompiler);
* .Model.csv - The NodeIds assigned to the Nodes defined by the specification;

For the core OPC UA specifications the following additional files are published:

* StatusCode.csv - The StatusCodes defined by the OPC UA specification;
* AttributeIds.csv - The identifiers for the Attributes defined by the OPC UA specification;
* UNECE_to_OPCUA.csv - The numeric codes assigned to the UNECE units (see Part 8);
* ServerCapabilities.csv - The ServerCapabilities defined by the OPC UA specification (see Part 12);
* Opc.Ua.NodeSet2.Services.xml - A UANodeSet that includes DataTypes which are used only with OPC UA Services;
* OPCBinarySchema.xsd - The OPC Binary schema definition;
* UANodeSet.xsd - The UANodeSet schema definition;
* SecuredApplication.xsd - The SecuredApplication schema definition (see Part 6);

The files for each companion specification are in a subdirectory with the short name of the specification.
The files for the core specification are in the Schema subdirectory.

## Release Process

**This repository only has the released versions of the normative files.**

Draft and Release Candidate are available in the [member only repository](https://github.com/OPCF-Members/UA-NodeSet).  

Instructions on requesting access to the member only repository can be found [here](https://opcf-members.github.io/Help/).  

The files in the 1.04 branch are also public to the OPC Foundation [website](https://opcfoundation.org/UA/schemas/).
**If someone is looking for the officially released version of the UANodeSets they must follow the links in the specification.**  

When the files are reviewed and published a tag will be created in this repo with the publication date specified in the UANodeSet.  
**Note** that adding tags is a step that was added late in the process so tags prior to 2019-05-01 do not exist and users must use the dates on the commits.  

**There are currently 3 branches in the repository:**

* v1.04 - default
* v1.03
* master (empty) - only available in a private repository

These branches correspond to a release of the OPC UA specification.
When a companion specification is released it will be added to the branch that matches the release of the OPC UA specification used.
If errata are later published for a companion specification the version in the appropriate branch will be updated.

## Error reporting

If an error or problem is found in any of the files it should be [reported](https://apps.opcfoundation.org/mantis/main_page.php) as a mantis issue against the appropriate specification.

More information on the process can befound [here](https://opcfoundation.org/resources/issue-tracking/).
