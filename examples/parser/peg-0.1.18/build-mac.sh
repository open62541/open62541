#!/bin/bash

xcodebuild -project peg.xcodeproj -configuration Release

cp build/Release/peg ./
cp build/Release/leg ./
