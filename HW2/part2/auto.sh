rm View2.txt
rm View1.txt
for i in {1..10}
do
    ./mandelbrot -t $i --view 2 2>&1 >> View2.txt
done

# tiv ./madelbrot-thread.ppm
# tiv ./madelbrot-serial.ppm
for i in {1..10}
do
    ./mandelbrot -t $i --view 1 2>&1 >> View1.txt
done

# tiv ./madelbrot-thread.ppm
# tiv ./madelbrot-serial.ppm
