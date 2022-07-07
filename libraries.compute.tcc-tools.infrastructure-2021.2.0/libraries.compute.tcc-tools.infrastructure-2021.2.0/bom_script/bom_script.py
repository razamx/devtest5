# -*- mode: python-mode; python-indent-offset: 4 -*-

import os
import csv
import argparse
import shutil


def remove_item(path, subitem):
    fullpath = path + subitem
    if len(subitem) != 0 and os.path.exists(fullpath):
        if os.path.isdir(fullpath):
            shutil.rmtree(fullpath)
            print("---- Direcory was removed")
            return True
        else:
            os.remove(fullpath)
            print("---- File was removed")
            return True
    return False


parser = argparse.ArgumentParser(description='Tool for changing structure '
                                 'from source folder to target')
parser.add_argument(
    '--source',
    action="store",
    dest="source",
    required=True,
    help='Specify directory with source files'
    ' (cmake install dir)  with trailing "/"')
parser.add_argument(
    '--target',
    action="store",
    dest="target",
    required=True,
    help='Specify directory for target '
    'structure with trailing "/"')
parser.add_argument(
    '--bom_file',
    action="store",
    dest="bom_file",
    required=True,
    help='Specify bom file')
args = parser.parse_args()
print("Passed arguments " + str(args))

with open(args.bom_file, 'r') as f:
    reader = csv.reader(f)
    commands_list = list(reader)

line_number = 0
error_code = 0
for line in commands_list:
    line_number = line_number + 1
    print("Line number is " + str(line_number) + " : " + str(line))
    if len(line) == 0:
        print("---- Skip void line")
        continue
    if line[0].strip()[0] == "#":
        print("---- Skip commented command")
        continue
    if len(line) != 3:
        print("ERROR: Incorrect format of line " + str(line_number) + " : " +
              str(line))
        print("ERROR: Line should contains 3 collums like "
              "COPY|REMOVE,source_path,target_path")
        error_code = 1
        continue

    line = list(map(lambda x: x.strip(), line))

    if line[0] == "COPY":
        if os.path.isdir(args.source + line[1]):
            shutil.copytree(args.source + line[1], args.target + line[2])
            print("---- Direcory was copied")
        else:
            os.makedirs(os.path.dirname(args.target + line[2]), exist_ok=True)
            shutil.copy2(args.source + line[1], args.target + line[2])
            print("---- File was copied")
    elif line[0] == "REMOVE":
        if (not remove_item(args.source, line[1])
                and not remove_item(args.target, line[2])):
            print("ERROR: Cannot remove item " + str(line_number) + " : " +
                  str(line))
            error_code = 1

    else:
        print("ERROR: Incorrect command of line " + str(line_number) + " : " +
              str(line))
        error_code = 1

print("EXIT CODE: " + str(error_code))
exit(error_code)
