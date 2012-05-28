#!/bin/sh
ls *.backup.* | perl -ne 'chomp; $a = $_; $b = $_; $a =~ s/\.backup/\.refactored/g; $b =~ s/\.backup//g; print "mv $b $a\nmv $_ $b\n"' | sh
