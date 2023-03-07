### This Source Code Form is subject to the terms of the Mozilla Public
### License, v. 2.0. If a copy of the MPL was not distributed with this
### file, You can obtain one at http://mozilla.org/MPL/2.0/.

###    Copyright 2014-2015 (c) TU-Dresden (Author: Chris Iatrou)
###    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
###    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH

# Opaque types are in general defined as simple byte strings. For the base opaque types there is a corresponding node id definition
# in the nodeset. E.g. Opc.Ua.Types.bsd contains the simple definition for OpaqueType LocaleId. In Opc.Ua.NodeSet2.xml the LocaleId
# is defined as a Subtype of String(i=12) thus LocaleId is a String object.
# TODO we can automate this mapping by loading the NodeSet2.xml and read those mappings automatically. For now we just use this map
opaque_type_mapping = {
    'Image': {
        'ns': 0,
        'id': 15,
        'name': 'ByteString'
    },
    'Number': {
        'ns': 0,
        'id': 24,
        'name': 'BaseDataType'
    },
    'UInteger': {
        'ns': 0,
        'id': 24,
        'name': 'BaseDataType'
    },
    'ImageBMP': {
        'ns': 0,
        'id': 15,
        'name': 'ByteString'
    },
    'ImageGIF': {
        'ns': 0,
        'id': 15,
        'name': 'ByteString'
    },
    'ImageJPG': {
        'ns': 0,
        'id': 15,
        'name': 'ByteString'
    },
    'ImagePNG': {
        'ns': 0,
        'id': 15,
        'name': 'ByteString'
    },
    'BitFieldMaskDataType': {
        'ns': 0,
        'id': 9,
        'name': 'UInt64'
    },
    'NormalizedString': {
        'ns': 0,
        'id': 12,
        'name': 'String'
    },
    'DecimalString': {
        'ns': 0,
        'id': 12,
        'name': 'String'
    },
    'DurationString': {
        'ns': 0,
        'id': 12,
        'name': 'String'
    },
    'TimeString': {
        'ns': 0,
        'id': 12,
        'name': 'String'
    },
    'DateString': {
        'ns': 0,
        'id': 12,
        'name': 'String'
    },
    'UriString': {
        'ns': 0,
        'id': 12,
        'name': 'String'
    },
    'SemanticVersionString': {
        'ns': 0,
        'id': 12,
        'name': 'String'
    },
    'Duration': {
        'ns': 0,
        'id': 11,
        'name': 'Double'
    },
    'UtcTime': {
        'ns': 0,
        'id': 13,
        'name': 'DateTime'
    },
    'LocaleId': {
        'ns': 0,
        'id': 12,
        'name': 'String'
    },
    'IntegerId': {
        'ns': 0,
        'id': 7,
        'name': 'UInt32'
    },
    'ApplicationInstanceCertificate': {
        'ns': 0,
        'id': 15,
        'name': 'ByteString'
    },
    'SessionAuthenticationToken': {
        'ns': 0,
        'id': 17,
        'name': 'NodeId'
    },
    'ContinuationPoint': {
        'ns': 0,
        'id': 15,
        'name': 'ByteString'
    },
    'Counter': {
        'ns': 0,
        'id': 7,
        'name': 'UInt32'
    },
    'NumericRange': {
        'ns': 0,
        'id': 12,
        'name': 'String'
    },
    'Time': {
        'ns': 0,
        'id': 12,
        'name': 'String'
    },
    'Date': {
        'ns': 0,
        'id': 13,
        'name': 'DateTime'
    }
}

def get_base_type_for_opaque(opaqueTypeName):
    if opaqueTypeName in opaque_type_mapping:
        return opaque_type_mapping[opaqueTypeName]
    # Default if not in mapping is ByteString
    return {
        'ns': 0,
        'id': 15,
        'name': 'ByteString'
    }
