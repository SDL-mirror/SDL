#! /bin/bash

DIRECTORY="tests"
EXT="error"

#echo "Installing test suites to $DIRECTORY" 

rm -Rfv $DIRECTORY > /dev/null 2>&1
mkdir $DIRECTORY > /dev/null 2>&1

PLATFORM="$(uname)"
if [[ $PLATFORM ==  "Linux" ]]; then
	EXT="so"
elif [[ $PLATFORM ==  "Darwin" ]]; then
	EXT="dylib"
fi

# TODO: put the test in an array
for suite in "testdummy" "testplatform" "testrect" "testaudio" "testsurface" "testrwops"
do
	cp -f "$suite/.libs/lib$suite.$EXT" $DIRECTORY
done

#sudo cp .libs/libtest.0.dylib /usr/local/lib/libtest.0.dylib

echo "Test suites installed."
