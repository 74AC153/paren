#!/bin/bash
set -e
PAREN=../paren

if [ "$1" = "" ]
then
	for i in *.expect;
	do
		BASE=`echo $i | sed 's/\(.*\).expect/\1/g'`
		echo $BASE

		if [ -e "$BASE.sh" ]
		then
			./$BASE.sh
		else
			diff $i <( LD_LIBRARY_PATH=../ PAREN_LEAK_CHECK=1 ${PAREN} $BASE )
		fi

	done
else
	if [ -e $1.sh ]
	then
		./$1.sh
	else
		diff $1.expect <( LD_LIBRARY_PATH=../ PAREN_LEAK_CHECK=1 ${PAREN} $1 )
	fi
fi
