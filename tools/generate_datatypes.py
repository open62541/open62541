#!/usr/bin/env python

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

from __future__ import print_function
from nodeset_compiler.type_parser import CSVBSDTypeParser
import nodeset_compiler.backend_open62541_typedefinitions as backend
import argparse

###############################
# Parse the Command Line Input#
###############################

parser = argparse.ArgumentParser()
parser.add_argument('-c', '--type-csv',
                    metavar="<typeDescriptions>",
                    type=argparse.FileType('r'),
                    dest="type_csv",
                    action='append',
                    default=[],
                    help='csv file with type descriptions')

parser.add_argument('--namespace',
                    type=int,
                    dest="namespace",
                    default=0,
                    help='namespace id of the generated type nodeids (defaults to 0)')

parser.add_argument('-s', '--selected-types',
                    metavar="<selectedTypes>",
                    type=argparse.FileType('r'),
                    dest="selected_types",
                    action='append',
                    default=[],
                    help='file with list of types (among those parsed) to be generated. If not given, all types are generated')

parser.add_argument('--no-builtin',
                    action='store_true',
                    dest="no_builtin",
                    help='Do not generate builtin types')

parser.add_argument('--opaque-map',
                    metavar="<opaqueTypeMap>",
                    type=argparse.FileType('r'),
                    dest="opaque_map",
                    action='append',
                    default=[],
                    help='JSON file with opaque type mapping: { \'typename\': { \'ns\': 0,  \'id\': 7, \'name\': \'UInt32\' }, ... }')

parser.add_argument('--internal',
                    action='store_true',
                    dest="internal",
                    help='Given bsd are internal types which do not have any .csv file')

parser.add_argument('-t', '--type-bsd',
                    metavar="<typeBsds>",
                    type=argparse.FileType('r'),
                    dest="type_bsd",
                    action='append',
                    default=[],
                    help='bsd file with type definitions')

parser.add_argument('-i', '--import',
                    metavar="<importBsds>",
                    type=str,
                    dest="import_bsd",
                    action='append',
                    default=[],
                    help='combination of TYPE_ARRAY#filepath.bsd with type definitions which should be loaded but not exported/generated')

parser.add_argument('outfile',
                    metavar='<outputFile>',
                    help='output file w/o extension')
args = parser.parse_args()

outname = args.outfile.split("/")[-1]
inname = ', '.join(list(map(lambda x: x.name.split("/")[-1], args.type_bsd)))

parser = CSVBSDTypeParser(args.opaque_map, args.selected_types, args.no_builtin, outname, args.namespace, args.import_bsd,
                          args.type_bsd, args.type_csv)
parser.create_types()

generator = backend.CGenerator(parser, inname, args.outfile, args.internal)
generator.write_definitions()
