#!/bin/bash

NB_FRAMES=$1
NB_THREADS_MIN=$2
NB_THREADS_MAX=$3

shift 2
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
    for NB_THREADS in `seq ${NB_THREADS_MIN} ${NB_THREADS_MAX}`
    do
        time (export OMP_NUM_THREADS=${NB_THREADS}; mpirun -np ${NP} ./lbm) 2> tmp_stats_np${NP}_nt${NB_THREADS}
        STATS=$(cat tmp_stats_np${NP}_nt${NB_THREADS})
        echo -e $STATS > stats_np${NP}_nt${NB_THREADS}
        mv resultat.raw resultat_np${NP}_nt${NB_THREADS}.raw
    #	./gen_animate_gif.sh ./resultat_$NP.raw ./output_$NP.gif
        
    #	./gen_animate_gif_legacy.sh ./resultat_$NP.raw ./output_legacy_$NP.gif
    done
done

for NP in $@
do
    for NB_THREADS in `seq ${NB_THREADS_MIN} ${NB_THREADS_MAX}`
    do
        CHECKSUMS=
        for FRAME in `seq 0 $NB_FRAMES`
        do
            CHECKSUMS="${CHECKSUMS}\nFrame ${FRAME}: $(./display --checksum resultat_np${NP}_nt${NB_THREADS}.raw $FRAME)"
        done
        echo -e $CHECKSUMS > checksums_np${NP}_nt${NB_THREADS}_${NB_FRAMES}
    done
done
