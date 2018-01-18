#!/bin/bash

aclocal
automake -a
autoreconf --force --install -I config -I m4
