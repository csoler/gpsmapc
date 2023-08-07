#!/bin/sh

echo This script converts and renames all png files in the current directory into jpg files as properly numbered file_\?\?\?.jpg
echo Default does nothing. Use -f to actually do the job.

currentnum="0"

# iterate until we find a name that is available
while : 
do
	padded=`seq -f "%03g" $currentnum $currentnum`

	if ! test -f file_"$padded".jpg ; then
		break
	fi
	currentnum=`expr $currentnum + 1`
done

echo First ID available: $currentnum
files=`ls *.png`

#find . -name "*.png" -print | while read file
ls -1 *.png | while read file
do 
	padded=`seq -f "%03g" $currentnum $currentnum`
	newname=file_"$padded".jpg

	echo padded is '"'$padded'"'
	echo file is '"'$file'"'
	echo new name is '"'$newname'"'

	if test "$1" = "-f" ; then
		convert "$file" $newname
	else
		echo convert \"$file\" $newname
	fi

	currentnum=`expr $currentnum + 1`
done
