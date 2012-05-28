#!/bin/sh
ls *.refactored.* | perl -ne 'chomp; $a = $_; $b = $_; $a =~ s/\.refactored/\.backup/g; $b =~ s/\.refactored//g; print "mv $b $a\nmv $_ $b\n"' | sh
