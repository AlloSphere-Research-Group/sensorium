#!/bin/bash

rm -rf build
rm -rf out

# currently not purging bin directory in case you need dynamic libraries or other config files with the binary
# rm -rf bin