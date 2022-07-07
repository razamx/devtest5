#!/bin/bash

CHECKER="$(dirname $0)/rules_checker.py"
CHANGED_FILES=$(git --no-pager diff --name-status master | grep -v D | awk '{print $NF}')
IS_ERROR=0
DIRNAME=$(basename $(pwd))

if [[ "$DIRNAME" != "tccsdk" ]]; then
	echo "Warning: current directory isn't \"tccsdk\""
	echo "Rule checker may work incorrectly"
fi

echo "Checking rules in changed files..."
for file in ${CHANGED_FILES}; do
	if ! python3 "$CHECKER" "../$DIRNAME/$file"; then
		IS_ERROR=1
	fi
done

if [[ $IS_ERROR = 0 ]]; then
	echo "All files OK"
fi

exit $IS_ERROR
