import argparse
import xml.etree.ElementTree as ET
import re
import sys

###############################
# Parse the Command Line Input#
###############################

parser = argparse.ArgumentParser()
parser.add_argument('-x', '--xml',
                    metavar="<nodeSetXML>",
                    dest="xmlfile")

parser.add_argument('-c', '--csv',
                    dest="csvfile")

args = parser.parse_args()

def namespace(element):
    m = re.match(r'{(.*)}', element.tag)
    return m.group(1) if m else ''

# TODO: use regex...?
def extract_browse_name(v):
    return v.split(':')[1]

def extract_nodeid(v):
    return v.split('=')[2]

def is_fixing_required(csv_path):
    with open(csv_path, 'r') as fp:
        for line in fp:
            if line.startswith("DefaultBinary") or line.startswith("DefaultXML"):
                return True
    return False


def fix_siomecsv(xml_path, csv_path):
    print("-- Try to fix csv file: ", csv_path)
    tree = ET.parse(xml_path)
    root = tree.getroot()

    ns = {'opcua': namespace(root)}

    binary_nodeid = []
    for o in root.findall('opcua:UAObject', ns):
        bn = o.attrib['BrowseName']
        if bn == 'Default Binary':
            nid = extract_nodeid(o.attrib['NodeId'])
            binary_nodeid.append(nid)

    encoding = []
    for d in root.findall('opcua:UADataType', ns):
        bn = extract_browse_name(d.attrib['BrowseName'])
        for r in d.findall('opcua:References/opcua:Reference', ns):
            rt = r.attrib['ReferenceType']
            if rt == 'HasEncoding':
                e_nid = extract_nodeid(r.text)
                if e_nid in binary_nodeid:
                    encoding.append(
                        '{0}_Encoding_DefaultBinary,{1},Object\n'.format(bn, e_nid))

    if(len(encoding) == 0):
        print("-- Could not fix csv file: ", csv_path)
        return

    # clean csv
    new_csv = []
    with open(csv_path, 'r') as fp:
        for line in fp:
            # remove redundant data...
            if 'DefaultBinary' in line:
                continue
            if 'DefaultXML' in line:
                continue
            new_csv.append(line)

    new_csv += encoding

    with open(csv_path, 'w') as fp:
        fp.writelines(new_csv)

if is_fixing_required(args.csvfile):
    fix_siomecsv(args.xmlfile, args.csvfile)
