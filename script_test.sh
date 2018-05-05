#!/bin/bash

NB_FRAMES=$1
shift 1
TIMEFORMAT="real:\t%lE\nuser:\t%lU\nsys:\t%lS\nusage:\t%P"
MPIEXEC_PREFIX_DEFAULT=

echo "Nombre de frames: $NB_FRAMES"

echo "Executions prevues:"
for NP in $@
do
	echo "$NP processus"
done

for NP in $@
do
    time (mpirun -np $NP ./lbm) 2> tmp_stats_${NP}
	STATS=$(cat tmp_stats_${NP})
	echo -e $STATS > stats_${NP}
	wait $!
	mv resultat.raw resultat_$NP.raw
#	./gen_animate_gif.sh ./resultat_$NP.raw ./output_$NP.gif
	
#	./gen_animate_gif_legacy.sh ./resultat_$NP.raw ./output_legacy_$NP.gif
done

for NP in $@
do
    CHECKSUMS=
	for FRAME in `seq 0 $NB_FRAMES`
	do
		CHECKSUMS="${CHECKSUMS}\nFrame ${FRAME}: $(./display --checksum resultat_$NP.raw $FRAME)"
	done
	echo -e $CHECKSUMS > checksums_${NP}_${NB_FRAMES}
done
