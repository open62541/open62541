#!/usr/bin/env python3
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
"""Generate test modules, dependency locks and CMake targets from the shared catalog."""
import argparse
import json
import sys
import re
from pathlib import Path
import xml.etree.ElementTree as ET

UA = 'http://opcfoundation.org/UA/'

def local(element):
    return element.tag.rsplit('}', 1)[-1]

def cstring(value):
    return json.dumps(value, ensure_ascii=False)

def cmake_path(value):
    return '"' + str(value).replace('\\', '/').replace('"', '\\"') + '"'

def write(path, content):
    if not path.exists() or path.read_text() != content:
        path.write_text(content)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--nodesets', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    source = Path(__file__).resolve().parent
    tools = source.parents[1] / 'tools'
    sys.path.insert(0, str(tools))
    from nodeset_compiler.type_parser import has_native_types
    output = args.output
    output.mkdir(parents=True, exist_ok=True)
    overrides = json.loads((source/"dependency_overrides.json").read_text())
    used_overrides = set()
    models = []
    namespace_uris = set()
    profiles = []
    standard = ET.parse(args.nodesets/'Schema/Opc.Ua.NodeSet2.xml').getroot()
    standard_aliases = {e.attrib['Alias']: e.text for section in standard
                        if local(section) == 'Aliases' for e in section}
    for line in (source/'catalog.txt').read_text().splitlines():
        if not line or line.startswith('#'):
            continue
        kind, path = line.split(' ', 1)
        root = ET.parse(args.nodesets/path).getroot()
        if kind == 'profile':
            if local(root) != 'UAProfileSet':
                raise ValueError('Expected a profile document: '+path)
            profiles.append(path)
            continue
        if local(root) != 'UANodeSet':
            raise ValueError('Expected a NodeSet: '+path)
        sections = {local(e): e for e in root}
        metadata = list(sections['Models'])[0]
        uris = [UA]+[e.text for e in sections.get('NamespaceUris', [])]
        namespace_uris.update(uris[1:])
        aliases = dict(standard_aliases)
        aliases.update({e.attrib['Alias']: e.text for e in sections.get('Aliases', [])})
        def identifier(value):
            value = aliases.get(value, value)
            ns = 0
            if value.startswith('ns='):
                prefix, value = value.split(';', 1)
                ns = int(prefix[3:])
            return uris[ns], value
        nodes = []
        has_types = False
        for node in root:
            tag = local(node)
            if not tag.startswith('UA') or 'NodeId' not in node.attrib:
                continue
            node_identifier = node.attrib['NodeId']
            ns = 0
            if node_identifier.startswith('ns='):
                prefix, node_identifier = node_identifier.split(';', 1)
                ns = int(prefix[3:])
            # Compare source-defined attributes and all variable shape constraints.
            attrs = [1, 3]
            children = {local(e): e for e in node}
            for name, index in [('DisplayName',4), ('Description',5)]:
                if name in children: attrs.append(index)
            for name, index in [('IsAbstract',8),('Symmetric',9),('InverseName',10),
                                ('EventNotifier',12),('DataType',14),('AccessLevel',17),
                                ('MinimumSamplingInterval',19),('Historizing',20),('Executable',21)]:
                if name in node.attrib or name in children: attrs.append(index)
            if tag in ('UAVariable','UAVariableType'):
                attrs += [14,15,16]
                if 'Value' in children: attrs.append(13)
            if (tag == 'UADataType' and 'Definition' in children and
                    any(local(e) == 'Field' for e in children['Definition'])):
                attrs.append(23)
                has_types |= any(local(e)=='Field' for e in children['Definition'])
            references = []
            for ref in children.get('References', []):
                type_uri, type_id = identifier(ref.attrib['ReferenceType'])
                target_uri, target_id = identifier(ref.text.strip())
                references.append((type_uri, type_id, target_uri, target_id,
                                   ref.get('IsForward', 'true').lower() != 'false'))
            nodes.append((uris[ns], node_identifier, sum(1 << x for x in set(attrs)), references))
        models.append(dict(index=len(models), path=path, uri=metadata.attrib['ModelUri'],
                           date=metadata.get('PublicationDate',''), version=metadata.get('Version',''), required=[d.attrib for d in metadata],
                           nodes=nodes, has_types=has_native_types(root)))
    if len({m['path'] for m in models}) != len(models):
        raise ValueError('Duplicate catalog paths')
    providers = {}
    for m in models:
        if m['uri'] not in providers or m['date'] > providers[m['uri']]['date']:
            providers[m['uri']] = m
    def closure(root):
        chosen = dict(providers)
        chosen[root['uri']] = root
        done = set(); active = set(); ordered = []
        def visit(m):
            if m['index'] in done: return
            if m['index'] in active: raise ValueError('Dependency cycle: '+m['path'])
            active.add(m['index'])
            for dep in m['required']:
                if dep['ModelUri'] == UA: continue
                provider = chosen[dep['ModelUri']]
                if dep.get('PublicationDate','') > provider['date']:
                    match = next((i for i, x in enumerate(overrides)
                                  if x['path'] == m['path'] and x['uri'] == dep['ModelUri']
                                  and x['requested'] == dep['PublicationDate']
                                  and x['available'] == provider['date']
                                  and x['version'] == dep.get('Version') == provider['version']), None)
                    if match is None:
                        raise ValueError('Dependency revision unavailable: '+m['path'])
                    used_overrides.add(match)
                visit(provider)
            active.remove(m['index']);done.add(m['index']);ordered.append(m)
        visit(root)
        return ordered
    namespace_map = [(i + 2, uri) for i, uri in enumerate(sorted(namespace_uris))]
    write(output/'namespace_plan.h',
          '/* Shared compile-time namespace plan for static registration. */\n'
          'static void companionPinNamespaces(UA_Server *server) {\n' +
          ''.join('    UA_Server_addNamespace(server, '+cstring(uri)+');\n'
                  for _, uri in namespace_map) + '}\n')
    cm = ['# Generated companion test targets.',
          'set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS '+
          ' '.join(cmake_path(args.nodesets/m['path']) for m in models)+')']
    registry_headers = []
    registry_items = []
    for m in models:
        i = m['index']; ident = 'cs_'+str(i)
        dependencies = closure(m)[:-1]
        m['dependencies'] = [d['index'] for d in dependencies]
        m['preferred'] = providers[m['uri']] is m
        m['label'] = re.sub('[^A-Za-z0-9]+', '_', m['path'][:-4]).strip('_')
        registry_headers.append('#include "'+ident+'.h"')
        registry_items.append('&companion_'+ident)
        write(output/(ident+'.h'), '#include "companion_test.h"\nextern const CompanionModule companion_'+ident+';\n')
        lines = ['#include "'+ident+'.h"', '#include "namespace_'+ident+'_generated.h"',
                 'static const size_t dependencies[] = {0, '+', '.join(str(d['index']) for d in dependencies)+'};']
        for n, (_, _, _, references) in enumerate(m['nodes']):
            if not references:
                continue
            lines.append('static const CompanionReference refs_'+str(n)+'[] = {')
            lines += ['    {'+', '.join(cstring(x) for x in ref[:4])+', '+
                      ('true' if ref[4] else 'false')+'},' for ref in references]
            lines.append('};')
        lines += ['static const UA_DataType *staticType(size_t index) {',
                  '#if UA_TYPES_CS_'+str(i)+'_COUNT > 0',
                  'return &UA_TYPES_CS_'+str(i)+'[index];',
                  '#else', '(void)index; return NULL;', '#endif', '}']
        lines.append('static const CompanionNode nodes[] = {{NULL,NULL,0,NULL,0},')
        lines += ['    {'+cstring(uri)+', '+cstring(node)+', '+str(mask)+'u, '+
                  ('refs_'+str(n) if refs else 'NULL')+', '+str(len(refs))+'},'
                  for n,(uri,node,mask,refs) in enumerate(m['nodes'])]
        lines += ['};', 'const CompanionModule companion_'+ident+' = {',
                  str(i)+', '+cstring(m['path'])+', dependencies + 1, '+str(len(dependencies))+',',
                  'nodes + 1, '+str(len(m['nodes']))+', '+('true' if m['preferred'] else 'false')+', namespace_'+ident+'_generated, namespace_'+ident+'_generated_getDataType, UA_TYPES_CS_'+str(i)+'_COUNT, staticType};']
        write(output/(ident+'.c'), '\n'.join(lines)+'\n')
        files = [output/('namespace_'+ident+'_generated.c'), output/('namespace_'+ident+'_generated.h'),
                 output/('types_'+ident+'_generated.c'),output/('types_'+ident+'_generated.h'),output/('types_'+ident+'_generated_internal.h')]
        command = ['${Python3_EXECUTABLE}', tools/'nodeset_compiler/nodeset_compiler.py',
                   '--existing',args.nodesets/'Schema/Opc.Ua.NodeSet2.xml','--types-array','UA_TYPES',
                   '--types-schema',args.nodesets/'Schema']
        for index, uri in namespace_map:
            command += ['--namespaceMap='+str(index)+':'+uri]
        for dep in dependencies:
            command += ['--existing',args.nodesets/dep['path']]
            if dep['has_types']: command += ['--types-array','UA_TYPES_CS_'+str(dep['index'])]
        command += ['--xml',args.nodesets/m['path'],'--types-output',output/('types_'+ident),output/('namespace_'+ident+'_generated')]
        cm += ['add_custom_command(OUTPUT '+' '.join(map(cmake_path,files)),
               ' COMMAND '+' '.join(cmake_path(x) for x in command),
               ' DEPENDS ${companion_generator_dependencies} '+' '.join(cmake_path(args.nodesets/d['path']) for d in dependencies+[m]),
               ' VERBATIM)',
               'add_custom_target(generate_'+ident+' DEPENDS '+' '.join(map(cmake_path,files))+')',
               'add_library('+ident+' OBJECT '+cmake_path(files[0])+' '+cmake_path(files[2])+' '+cmake_path(output/(ident+'.c'))+')',
               'set_target_properties('+ident+' PROPERTIES UNITY_BUILD OFF)',
               'add_dependencies('+ident+' generate_'+ident+' open62541-generator-types)']
    for m in models:
        i=m['index'];ident='cs_'+str(i)
        for dep in m['dependencies']: cm += ['add_dependencies('+ident+' generate_cs_'+str(dep)+')']
        selected = m['dependencies']+[i]
        write(output/('registry_'+ident+'.c'), '\n'.join(registry_headers[j] for j in selected)+
            '\nconst CompanionModule *const companionModules[] = {'+', '.join(registry_items[j] for j in selected)+'};\nconst size_t companionModulesSize = sizeof(companionModules)/sizeof(companionModules[0]);\n')
        cm += ['add_executable(check_companion_'+ident+' "${CMAKE_CURRENT_SOURCE_DIR}/runner.c" '+cmake_path(output/('registry_'+ident+'.c'))+' '+' '.join('$<TARGET_OBJECTS:cs_'+str(j)+'>' for j in selected)+')',
               'target_compile_definitions(check_companion_'+ident+' PRIVATE COMPANION_CASE='+str(i)+')',
               'target_link_libraries(check_companion_'+ident+' PRIVATE companion_test_support open62541)',
               'list(APPEND companion_binaries check_companion_'+ident+')']
        cm += ['set_target_properties(check_companion_'+ident+' PROPERTIES OUTPUT_NAME check_companion_'+m['label']+')']
        for mode in ('copy','inplace'):
            cm += ['add_test(NAME compiler_'+m['label']+'_'+mode+' COMMAND check_companion_'+ident+' --'+mode+')',
                   'set_tests_properties(compiler_'+m['label']+'_'+mode+' PROPERTIES LABELS companion TIMEOUT 300)']
        cm += ['add_test(NAME monster_'+m['label']+' COMMAND check_companion_monster --case '+str(i)+' --copy)',
               'set_tests_properties(monster_'+m['label']+' PROPERTIES LABELS companion TIMEOUT 300)']
    write(output/'registry_all.c', '\n'.join(registry_headers)+'\nconst CompanionModule *const companionModules[] = {'+', '.join(registry_items)+'};\nconst size_t companionModulesSize = sizeof(companionModules)/sizeof(companionModules[0]);\n')
    cm += ['add_executable(check_companion_monster "${CMAKE_CURRENT_SOURCE_DIR}/runner.c" '+cmake_path(output/'registry_all.c')+' '+' '.join('$<TARGET_OBJECTS:cs_'+str(m['index'])+'>' for m in models)+')',
           'target_compile_definitions(check_companion_monster PRIVATE COMPANION_CASE=-1)',
           'target_link_libraries(check_companion_monster PRIVATE companion_test_support open62541)',
           'add_custom_target(check_companion_specs DEPENDS check_companion_monster ${companion_binaries})']
    for mode in ('copy','inplace'):
        cm += ['add_test(NAME compiler_monster_'+mode+' COMMAND check_companion_monster --'+mode+')',
               'set_tests_properties(compiler_monster_'+mode+' PROPERTIES LABELS companion TIMEOUT 1800)']
    if used_overrides != set(range(len(overrides))):
        raise ValueError('Stale dependency override; update the catalog exceptions')
    write(output/'targets.cmake', '\n'.join(cm)+'\n')
    write(output/'catalog.json', json.dumps(dict(models=[{**{k:v for k,v in m.items() if k != 'nodes'}, 'nodesSize':len(m['nodes'])} for m in models],profiles=profiles),indent=2))
    print('Companion catalog:',len(models),'NodeSets,',len(profiles),'profile documents;',len(providers),'shared-server model revisions')

if __name__ == '__main__': main()
