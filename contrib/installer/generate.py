#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Script to generate the WiX source for an installer.
#
# This file is a contribution to COMP_hack.
#
# Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import uuid
import os
import re

VARS = {
    '@COMPONENT_REFS@': '',
    '@MEDIA@': '',
}

def ScanDir(root):
    files = []
    dirs = []

    for item in os.listdir(root):
        path = os.path.join(root, item)

        if os.path.isdir(path):
            dirs.append(item)
        else:
            files.append(item)

    return (files, dirs)

def SanitizeID(s):
    return s.replace(os.path.sep, '.').replace('-', '.')

def GenerateFileList(root, indent = 0):
    (files, dirs) = ScanDir(root)

    indent_str = " " * indent

    component_ids = []
    disk_ids = []

    xml = ''

    for d in dirs:
        path = os.path.join(root, d)
        dir_id = SanitizeID(path.replace(os.path.sep, '.'))
        xml += '{}<Directory Id="{}" Name="{}">\n'.format(indent_str, dir_id, d)
        (sub_xml, sub_component_ids, sub_disk_ids) = GenerateFileList(path, indent + 2)
        component_ids += sub_component_ids
        disk_ids += sub_disk_ids
        xml += sub_xml
        xml += '{}</Directory>\n'.format(indent_str)

    for f in files:
        match = re.match(r'base([0-9]+)\.pak', f)
        if match:
            disk_id = int(match.group(1)) + 2
        else:
            disk_id = 1

        path = os.path.join(root, f)
        sanitized_path = SanitizeID(path)
        component_id = 'C_' + sanitized_path
        file_id = 'F_' + sanitized_path

        xml += '{}<Component Id="{}" Guid="{}">\n'.format(indent_str, component_id, uuid.uuid4())
        xml += '{}  <File Id="{}" Source="{}" DiskId="{}"/>\n'.format(indent_str, file_id, path, disk_id)
        xml += '{}</Component>\n'.format(indent_str)

        component_ids.append(component_id)
        disk_ids.append(disk_id)

    return (xml, component_ids, disk_ids)

(VARS['@FILES@'], component_ids, disk_ids) = GenerateFileList("files", 6)

for component_id in set(component_ids):
    VARS['@COMPONENT_REFS@'] += '      <ComponentRef Id="{}"/>\n'.format(component_id)

for disk_id in set(disk_ids):
    VARS['@MEDIA@'] += '    <Media Id="{}" Cabinet="data{}.cab" EmbedCab="yes"/>\n'.format(disk_id, disk_id)

file = open('installer.in.wxs', 'r').read()

for key, value in VARS.items():
    file = file.replace(key, value.strip())

open('installer.wxs', 'w').write(file)
