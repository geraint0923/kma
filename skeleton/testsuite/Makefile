###############################################################################
#
# File:         Makefile
# RCS:          $Id: Makefile,v 1.2 2005/10/14 03:52:59 sbirrer Exp $
# Description:  Guess
# Author:       Fabian E. Bustamante
#               Northwestern Systems Research Group
#               Department of Computer Science
#               Northwestern University
# Created:      Fri Sep 12, 2003 at 15:56:30
# Modified:     Wed Sep 24, 2003 at 18:31:43 fabianb@cs.northwestern.edu
# Language:     Makefile
# Package:      N/A
# Status:       Experimental (Do Not Distribute)
#
# (C) Copyright 2003, Northwestern University, all rights reserved.
#
###############################################################################

# handin info
TEAM = dkj755
VERSION = `date +%Y%m%d%H%M%S`
PROJ = kma

COMPETITION = KMA_DUMMY

CC = gcc
MV = mv
CP = cp
RM = rm
MKDIR = mkdir
TAR = tar cvf
COMPRESS = gzip
CFLAGS = -g -Wall -O2 -D HAVE_CONFIG_H

DELIVERY = Makefile *.h *.c DOC
PROGS = kma_dummy kma_rm kma_p2fl kma_mck2 kma_bud kma_lzbud
SRCS = kma.c kma_page.c kma_dummy.c kma_rm.c kma_p2fl.c kma_mck2.c kma_bud.c kma_lzbud.c
OBJS = ${SRCS:.c=.o}

VM_NAME = "Ubuntu_1404"
VM_PORT = "3022"

SHELL_ARCH = "64"


all: ${PROGS} competition

competition:
	echo "Using ${COMPETITION} for competition"
	${CC} ${CFLAGS} -DCOMPETITION -D${COMPETITION} -o kma_competition ${SRCS}

competitionAlgorithm:
	echo ${COMPETITION}

analyze:
	gnuplot kma_output.plt

test-reg: handin
	HANDIN=`pwd`/${TEAM}-${VERSION}-${PROJ}.tar.gz;\
	cd testsuite;\
	bash ./run_testcase.sh $${HANDIN} ${SHELL_ARCH};

start-vm:
	VBoxManage startvm ${VM_NAME} --type headless

kill-vm:
	VBoxManage controlvm ${VM_NAME} poweroff

test-vm:
	scp -r -i id_aqualab -P 3022 * aqualab@localhost:~/.aqualab/project2/.
	ssh -i id_aqualab -p 3022 aqualab@localhost 'bash -s' < vm_test.sh

handin: clean
	${TAR} ${TEAM}-${VERSION}-${PROJ}.tar ${DELIVERY}
	${COMPRESS} ${TEAM}-${VERSION}-${PROJ}.tar

.o:
	${CC} *.c

kma_dummy: ${SRCS}
	${CC} ${CFLAGS} -DKMA_DUMMY -o $@ ${SRCS}

kma_rm: ${SRCS}
	${CC} ${CFLAGS} -DKMA_RM -o $@ ${SRCS}

kma_p2fl: ${SRCS}
	${CC} ${CFLAGS} -DKMA_P2FL -o $@ ${SRCS}

kma_mck2: ${SRCS}
	${CC} ${CFLAGS} -DKMA_MCK2 -o $@ ${SRCS}

kma_bud: ${SRCS}
	${CC} ${CFLAGS} -DKMA_BUD -o $@ ${SRCS}

kma_lzbud: ${SRCS}
	${CC} ${CFLAGS} -DKMA_LZBUD -o $@ ${SRCS}

leak: $(TARGET)
	for exec in ${PROGS}; do \
		echo "Checking $${exec} (press ENTER to start)";\
		read;\
		valgrind -v --show-reachable=yes --leak-check=yes $${exec}; \
	done

clean:
	${RM} -f ${PROGS} kma_competition kma_output.dat kma_output.png kma_waste.png
	${RM} -f *.o *~ *.gch ${TEAM}*.tar ${TEAM}*.tar.gz

