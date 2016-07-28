#!/bin/bash
../radio/testwriter .1 1000 >> /dev/null &
../radio/testreader 5000
