#!/bin/bash

#Script to invoke creation of configure script
touch COPYING NEWS README AUTHORS ChangeLog
autoreconf -fi
