#! /bin/bash

DIRECTORY="tests"
EXT="error"

#echo "Installing test suites to $DIRECTORY" 

mkdir $DIRECTORY > /dev/null 2>&1

PLATFORM="$(uname)"
if [[ $PLATFORM ==  "Linux" ]]; then
	EXT="so"
elif [[ $PLATFORM ==  "Darwin" ]]; then
	EXT="dylib"
fi

# TODO: put the test in an array
for suite in "testdummy" "testplatform" "testrect" "testaudio" "testsurface" "testrwops" "testrender"
do
	cp -f "$DIRECTORY/$suite/.libs/lib$suite.$EXT" $DIRECTORY
done

echo "Test suites installed."
