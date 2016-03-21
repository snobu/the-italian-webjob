#!/usr/bin/env bash

killall -9 python

until ./readserial.py; do
        echo "Listener crashed with exit code $?. Respawning..." >&2
        sleep 2
done
