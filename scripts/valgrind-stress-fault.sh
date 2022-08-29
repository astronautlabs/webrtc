#!/bin/bash

while true; do
    valgrind node dist/nodejs/test/test.js
    exit_code="$?"
    if [ "$exit_code" != 0 ]; then
        exit $exit_code
    fi
done