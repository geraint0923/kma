set term png
set output "kma_output.png"
plot "kma_output.dat" using 1:2 with lines title "Requested", \
     "kma_output.dat" using 1:3 with lines title "Allocated"

set output "kma_waste.png"
plot "kma_output.dat" using 1:($3-$2) with lines title "Waste"
     
