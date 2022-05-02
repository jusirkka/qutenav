#!/usr/bin/env python
# -*- coding: utf-8 -*-

import xml.etree.ElementTree as et
import sys
import os
import re

def is_default(fn):
  return 'translations/id_en.ts' in fn

def parseObjectClasses(default_tr):
  ofn = './data/s57objectclasses.csv'

  r = re.compile(r'\d+,"([^"]+)",(\w{6}),.*')

  f = open(ofn)
  lc = 0
  items = []
  first = True
  for line in f:
    lc += 1
    m = r.match(line)
    if not m:
      continue

    item = et.Element('message')
    item.set('id', 'qutenav-{}'.format(m.group(2)))

    loc = et.Element('location')
    if first:
      loc.set('filename', '.{}'.format(ofn))
      first = False
    loc.set('line', '+{}'.format(lc))
    item.append(loc)
    lc = 0

    src = et.Element('source')
    src.text = m.group(1)
    item.append(src)

    tr = et.Element('translation')
    if default_tr:
      tr.text = m.group(1)
    else:
      tr.set('type', 'unfinished')
    item.append(tr)

    items.append(item)

  f.close()
  return items

def parseAttributes(default_tr):
  afn = './data/s57attributes.csv'
  efn = './data/s57expectedinput.csv'

  r = re.compile(r'(\d+),([^,]+),(.{6}),.*')
  f = open(afn)
  lc = 0
  attrs = {}
  for line in f:
    lc += 1
    m = r.match(line)
    if not m:
      continue
    attrs[m.group(1)] = {'id': 'qutenav-{}'.format(m.group(3)),
                         'line': lc,
                         'tr': m.group(2),
                         'enums': []}

  f.close()

  r = re.compile(r'(\d+),(\d+),"([^"]+)"')
  f = open(efn)
  lc = 0
  for line in f:
    lc += 1
    m = r.match(line)
    if not m:
      continue
    item = {'id': m.group(2), 'line': lc, 'tr': m.group(3)}
    attrs[m.group(1)]['enums'].append(item)

  f.close()

  items = []
  prevIsAttr = False
  prevAttrLine = 0
  prevEnumLine = 0
  for k, v in attrs.items():
    item = et.Element('message')
    item.set('id', v['id'])

    loc = et.Element('location')
    if not prevIsAttr:
      loc.set('filename', '.{}'.format(afn))
      prevIsAttr = True

    loc.set('line', '+{}'.format(v['line'] - prevAttrLine))
    prevAttrLine = v['line']
    item.append(loc)

    src = et.Element('source')
    src.text = v['tr']
    item.append(src)

    tr = et.Element('translation')
    if default_tr:
      tr.text = v['tr']
    else:
      tr.set('type', 'unfinished')
    item.append(tr)

    items.append(item)

    prevIsEnum = False
    for enum in v['enums']:
      item = et.Element('message')
      item.set('id', '{}-{}'.format(v['id'], enum['id']))

      loc = et.Element('location')
      if not prevIsEnum:
        loc.set('filename', '.{}'.format(efn))
        prevIsEnum = True
        prevIsAttr = False

      loc.set('line', '+{}'.format(enum['line'] - prevEnumLine))
      prevEnumLine = enum['line']

      item.append(loc)

      src = et.Element('source')
      src.text = enum['tr']
      item.append(src)

      tr = et.Element('translation')
      if default_tr:
        tr.text = enum['tr']
      else:
        tr.set('type', 'unfinished')
      item.append(tr)

      items.append(item)

  return items

def parseS57Files(default_tr):
  # <message id="qtnav-units">
  #   <location filename="../qml/UnitPreferencesPage.qml" line="+25"/>
  #   <source>Units</source>
  #   <translation>Units</translation>
  # </message>
  items = parseObjectClasses(default_tr)
  items += parseAttributes(default_tr)
  return items

def main(tr_file):

  items = parseS57Files(is_default(tr_file));

  tree = et.parse(tr_file)
  context = tree.getroot().find('./context')

  for item in items:
    m = context.find('./message[@id="{}"]'.format(item.get('id')))
    if m:
      context.remove(m)
    context.append(item)

  et.indent(tree)
  tree.write(tr_file, encoding='unicode')

if __name__ == '__main__':
  main(sys.argv[1])
