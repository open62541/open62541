#!/usr/bin/env python
# -*- coding: utf-8 -*-

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

###
### Author:  Chris Iatrou (ichrispa@core-vector.net)
### Version: rev 13
###
### This program was created for educational purposes and has been
### contributed to the open62541 project by the author. All licensing
### terms for this source is inherited by the terms and conditions
### specified for by the open62541 project (see the projects readme
### file for more information on the MPLv2 terms and restrictions).
###
### This program is not meant to be used in a production environment. The
### author is not liable for any complications arising due to the use of
### this program.
###

NODE_CLASS_GENERERIC = 0
NODE_CLASS_OBJECT = 1
NODE_CLASS_VARIABLE = 2
NODE_CLASS_METHOD = 4
NODE_CLASS_OBJECTTYPE = 8
NODE_CLASS_VARIABLETYPE = 16
NODE_CLASS_REFERENCETYPE = 32
NODE_CLASS_DATATYPE = 64
NODE_CLASS_VIEW = 128

# Not in OPC-UA, but exists in XML
NODE_CLASS_METHODTYPE = 256

