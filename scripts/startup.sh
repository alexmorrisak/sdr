#!/bin/bash
../radio/testwriter .1 1000 >> /dev/null &
sleep 1
../radio/testreader 5000
