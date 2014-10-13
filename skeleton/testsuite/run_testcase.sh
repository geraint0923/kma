#!/bin/bash

source config.test;

if [[ ! "$#" -eq 1 ]]; then
	echo -e "usage: $0 targz_file";
	exit 1;
fi;

TARGZ=$1;
TC_DIR=`pwd`;
TMP=`mktemp -d /tmp/cs343.tests.XXXXXX`;
OUTPUT=`mktemp -d /tmp/cs343.tests.XXXXXX`;
chmod go-rwx ${TMP} ${OUTPUT} || exit 1;

function cleanUp()
{
	rm -Rf ${TMP} ${OUTPUT};
}

echo "Testing ${TARGZ}";
echo;

# Untar sources
echo "UNTAR";
#cd ${TMP} || { cleanUp; exit 1; }
tar xvfz ${TARGZ} || { cleanUp; exit 1; }

echo;

if [[ `ls *.c |wc -w` -eq 0 ]]; then
    echo "error: no source files (*.c) found";
	echo "Please follow the submission instructions carefully.";
	cleanUp;
    exit 1;
fi;

# Setup the environment
echo "SETUP";

for f in ${ORIG_FILES}; do
	cp -f ${TC_DIR}/$f .;
done	

if [ ! -f Makefile -a ! -f makefile ]; then
	echo "warning: Makefile is missing";
	echo;
fi;

# Compile the code
echo "COMPILE"
for f in ${PROGS}; do
	echo "compiling $f";
	FILES="";
	for src in ${SRCS}; do
		if [ -f ${src} ]; then
			FILES="$FILES ${src}";
		fi;
	done;
	${CC} ${CFLAGS} -D${f} -o $f ${FILES} >> ${OUTPUT}/gcc.output 2>&1;
	echo "----------" >> ${OUTPUT}/gcc.output;
	if [ ! -f ${f} ]; then
		${CC} ${CFLAGS} -D${f} -o $f ${FILES};
	fi;
done

# Make the competition file
# We'll see all compile errors/warnings in the compile step above
make -s competition

WARNING=`grep -c warning ${OUTPUT}/gcc.output`
ERROR=`grep -c error ${OUTPUT}/gcc.output`

echo "${WARNING} warning(s) found while compiling"
echo "${ERROR} error(s) found while compiling"

if [ ${WARNING} -gt 0 -o ${ERROR} -gt 0 ]; then
	echo;
	echo "GCC OUTPUT";
	cat ${OUTPUT}/gcc.output;
fi;

if [ ${ERROR} -gt 0 ]; then
	echo "error: failed to create executable";
	cat ${OUTPUT}/gcc.output;
	cleanUp;
	exit 1;
fi;

echo;

# Testin
echo "TESTING REQUIRED ALGORITHMS";

for f in ${BASIC_PROGS}; do
	echo $f;
	OK=1
	for g in ${TRACES}; do
	    ./$f $g > $f.$g.out 2>&1
	    if [[ ` cat $f.$g.out | grep -c "Test: PASS"` -eq 0 ]]; then
            # failed
            echo "Trace $g failed. Tail of output follows"
            echo "..."
            tail $f.$g.out
            OK=0
            break
	    else
            echo "Trace $g: PASSED"
	    fi
	done
	if [[ $OK -eq "1" ]]; then
	    echo "Algorithm $f: PASSED"
	else
	    echo "Algorithm $f: FAILED"
	fi
	echo;
done

echo "TESTING EXTRA CREDIT ALGORITHMS";

for f in ${EC_PROGS}; do
	echo $f;
	OK=1
	for g in ${TRACES}; do
	    ./$f $g > $f.$g.out 2>&1
	    if [[ ` cat $f.$g.out | grep -c "Test: PASS"` -eq 0 ]]; then
            # failed
            echo "Trace $g failed. Tail of output follows"
            echo "..."
            tail $f.$g.out
            OK=0
            break
	    else
            echo "Trace $g: PASSED"
	    fi
	done
	if [[ $OK -eq "1" ]]; then
	    echo "Algorithm $f: PASSED"
	else
	    echo "Algorithm $f: FAILED"
	fi
	echo;
done

# Malloc
echo "MALLOC USAGE";
grep -H malloc *_*.c --exclude kma_rm.c --exclude kma_page.c | grep -v kma_malloc;
grep -H calloc *_*.c --exclude kma_rm.c --exclude kma_page.c;

echo;

# Competition
COMPETITION_ALGORITHM=`make -s competitionAlgorithm`
echo "COMPETITION: running ${COMPETITION_ALGORITHM} on ${COMPETITION_TRACE}"

BEST_TIME=-1
OK=1
for i in {1..5}; do

    bash -c "time -p ./${COMPETITION_BIN} ${COMPETITION_TRACE} > competition.out 2>&1" > competition.time 2>&1
    
    if [[ `cat competition.out | grep -c "Test: PASS"` -eq 0 ]]; then
	OK=0
	break
    fi

    RUNTIME=`cat competition.time | grep "real" | awk '{ print $2 }'`

    BEST_TIME=`echo "if ( ( ${BEST_TIME} == -1) || ( ${RUNTIME} < ${BEST_TIME} )) { ${RUNTIME} } else { ${BEST_TIME} }" | bc`
    
done

if [[ ${OK} -eq 1 ]]; then
    echo "Competition binary successfully completed the trace"

    cat competition.out

    INEFFICIENCY=`cat competition.out | grep "Competition average ratio" | awk '{ print $4 }' `

    PERFORMANCE=`echo "${BEST_TIME} * (1 + ${INEFFICIENCY})" | bc`

    echo
    echo "Best time (out of 5 runs): ${BEST_TIME}"
    echo "Competition score: ${PERFORMANCE}"
else
    echo "Competition binary failed to complete the trace. Tail of output follows..."
    echo
    tail competition.out
    echo
fi

#Clean up
cleanUp;
