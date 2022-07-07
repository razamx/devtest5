#!/bin/bash
echo "Patching script in html output files"
for fname in $(find -type f -name '*.html' -not -name 'header.html')
do
    title=$(cat $fname | grep -iPo '(?<=<title>)(.*)(?=</title>)')
    if [ -z "$title" ]; then
        continue
    fi
    hash=$(echo -n $(basename $fname) | md5sum | awk '{print $1}')
    REPLACE="wa_ssg_data={\"wa_ssg_title\":\"$title\",\"wa_ssg_nid\":\"$hash\",\"wa_ssg_local_code\":\"en_US\"}"
    sed -i "s/wa_ssg_bs_placeholder/$(echo $REPLACE | sed -e 's/\\/\\\\/g; s/\//\\\//g; s/&/\\\&/g')/" $fname
done
