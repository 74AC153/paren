#!/bin/bash
set -e
PAREN=../paren

if [ $1 = "all" ]
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
	BASE=`echo $0 | sed 's/\(.*\).expect/\1/g'`
	echo $BASE

	if [ -e $BASE.sh ]
	then
		./$BASE.sh
	else
		diff $1 <( LD_LIBRARY_PATH=../ PAREN_LEAK_CHECK=1 ${PAREN} $BASE )
	fi
fi
