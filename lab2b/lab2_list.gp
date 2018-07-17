#! /usr/bin/gnuplot

# general plot parameters
set terminal png
set datafile separator ","

set title "List-1: Throughput of Synchronized Lists"
set xlabel "Threads"
set logscale x 2
set ylabel "Throughput (ops/sec)"
set logscale y 10
set output 'lab2b_1.png'

plot \
     "< grep 'list-none-m,.*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'list w/mutex' with linespoints lc rgb 'red', \
     "< grep 'list-none-s,.*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'list w/spin' with linespoints lc rgb 'green'


set title "List-2: Waiting and Completion time for Mutex"
set xlabel "Threads"
set logscale x 2
set ylabel "mean time (ns)"
set logscale y 10
set output 'lab2b_2.png'
plot \
     "< grep 'list-none-m,.*,1000,1,' lab2b_list.csv" using ($2):($7) \
	title 'completion time' with linespoints lc rgb 'green', \
     "< grep 'list-none-m,.*,1000,1,' lab2b_list.csv" using ($2):($8) \
	title 'waiting time' with linespoints lc rgb 'red'
     
set title "List-3: Successful Implementations with Sublists"
set logscale x 2
set xrange [0.75:]
set xlabel "Threads"
set ylabel "successful iterations"
set logscale y 10
set output 'lab2b_3.png'
plot \
    "< grep 'list-id-none,' lab2b_list.csv" using ($2):($3) \
	with points lc rgb "green" title "unprotected", \
    "< grep 'list-id-m,' lab2b_list.csv" using ($2):($3) \
	with points lc rgb "red" title "Mutex", \
    "< grep 'list-id-s,' lab2b_list.csv" using ($2):($3) \
	with points lc rgb "blue" title "Spin_Lock"


set title "List-4: Throughput with Mutex"
set xlabel "Threads"
set logscale x 2
set ylabel "Throughput"
set logscale y 10
set output 'lab2b_4.png'
plot \
     "< grep 'list-none-m,.*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '--lists=1' with linespoints lc rgb 'blue', \
     "< grep 'list-none-m,.*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title '--lists=4' with linespoints lc rgb 'green', \
     "< grep 'list-none-m,.*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title '--lists=8' with linespoints lc rgb 'red', \
     "< grep 'list-none-m,.*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title '--lists=16' with linespoints lc rgb 'purple'

set title "List-5: Throughput with Spin_Lock"
set xlabel "Threads"
set logscale x 2
set ylabel "Throughput"
set logscale y 10
set output 'lab2b_5.png'
plot \
     "< grep 'list-none-s,.*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title '--lists=1' with linespoints lc rgb 'blue', \
     "< grep 'list-none-s,.*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title '--lists=4' with linespoints lc rgb 'green', \
     "< grep 'list-none-s,.*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title '--lists=8' with linespoints lc rgb 'red', \
     "< grep 'list-none-s,.*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title '--lists=16' with linespoints lc rgb 'purple'
