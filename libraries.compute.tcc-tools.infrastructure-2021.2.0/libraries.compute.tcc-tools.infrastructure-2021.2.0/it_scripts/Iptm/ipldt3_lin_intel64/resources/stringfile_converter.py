import sys

if len(sys.argv) < 2:
    print "Please provide the old string file as the first argument"
    exit()

old_file = open(sys.argv[1])
new_file = open("stringfile.yaml", "w")

section_start = False
accept_start = False
hit_start = False
hit_end_position = 0

line_number = 0
empty_prev_line = False

for line in old_file:
    line_number += 1
# comment
    if line.startswith("### <String File, Version "):
        version = line.replace("### <String File, Version ", '').replace('>', '').strip()
        new_file.write("version: %s\n" % version)
    elif line.startswith("#"):
        new_file.write("%s" % line)

#hit + modifiers
    elif line.startswith('"'):
        hit_start = False
        if section_start:
            new_file.write("hits:\n")
            section_start = False

        hit_end_pos = line.find('"', 1)
        hit_name = line[1:hit_end_pos]
        new_file.write("\n- %s" % hit_name.strip())
        hit_end_position = new_file.tell()
        accept_start = True

        if " @case=" in line:
           if not hit_start:
               new_file.write(":\n")
               hit_start = True
           case_pos = line.find("@case=") + 7
           case_string = line[case_pos]
           new_file.write("    case: %s\n" % case_string)

        if " @exact=" in line:
            if not hit_start:
                new_file.write(":\n")
                hit_start = True
            exact_pos = line.find("@exact=") + 8
            exact_string = line[exact_pos]
            new_file.write("    exact: %s\n" % exact_string)

#accept
    elif "@accept=" in line:
        accept_string = line.strip().replace('@accept="', '')
        accept_string = accept_string[:accept_string.rindex('"')]
        accept_string = accept_string.replace('\'', '\'\'')
        accept_string = '\'' + accept_string + '\''
        if accept_start:
            #new_file.seek(hit_end_position)
            if not hit_start:
                new_file.write(":\n")
                hit_start = True
            new_file.write("    accept:\n        - %s\n" % accept_string)
            accept_start = False
        else:
            new_file.write("        - %s\n" % accept_string)

#section-level modifiers
    elif (line.startswith("@case=") and not empty_prev_line):
        new_file.write(line.replace("@case=", "case: ").replace('"', ''))
    elif line.startswith("@exact="):
        new_file.write(line.replace("@exact=", "exact: ").replace('"', ''))
    elif line.startswith("@first="):
        new_file.write(line.replace("@first=", "first: ").replace('"', ''))        

    elif line.lstrip().startswith("@case=") and not empty_prev_line:        
        if not hit_start:
            new_file.write(":\n")
            hit_start = True
        new_file.write(line.lstrip().replace("@case=", "    case: ").replace('"', ''))

#sectiondef
    elif line.startswith("$sectiondef"):
        sd_list = line.strip().split(" ")
        sd_items = "[" + ", ".join(sd_list[2:]) + "]"
        new_file.write("sectiondef:\n     %s: %s\n" % (sd_list[1], sd_items))

#empty line
    elif line.isspace():
        new_file.write("\n")


# key-value pair
    else:
        new_line = line.replace("$sectiondef", "sectiondef:")
        new_line = new_line.replace("$section ", "---\nsection: ")
        new_line = new_line.replace("$guidance ", "guidance: ")
        if line == new_line:
            print "could not parse line %d: %s" % (line_number, line)
        else:
            section_start = True
            new_line = new_line.replace('"', '')
            new_file.write(new_line)

    if not line.isspace():
        empty_prev_line = False
    else:
        empty_prev_line = True

