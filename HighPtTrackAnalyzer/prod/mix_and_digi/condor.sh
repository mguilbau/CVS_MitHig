#!/bin/bash

cfg=$2
dir=`pwd`
outfile=$2-output-`date +"%s"`

subfile=subfile_$outfile

echo $subfile
mkdir -p /tmp/edwenger

cat > $subfile <<EOF

Universe     = vanilla

Notification = Error
Executable   = $dir/$1
Arguments    = $2 $3 $4 $5 $6 $7 $8 $9
Requirements = (Mips > 900) && (ARCH=="X86_64")
Rank         = Mips
GetEnv       = True

Initialdir   = $dir
Input        = /dev/null
Output       = /tmp/edwenger/$outfile.out
Error        = /tmp/edwenger/$outfile.err
Log          = /tmp/edwenger/$outfile.log

transfer_input_files = $dir/$cfg
should_transfer_files   = YES
when_to_transfer_output = ON_EXIT

Queue
EOF

sleep 0
cat $subfile

echo Executable   = $dir/$1
echo Arguments    = $2 $3 $4 $5 $6 $7 $8 $9


condor_submit $subfile
sleep 0
rm $subfile

