# Parallel Game of Life with MPI

Please see `analysis.pdf` for the analysis of my programs's performance. 

To compile and run, use `make`, and then exectute the program with `mpicxx my_code`.

## Using Game of Life

The Class `Life.h` implements a Game of Life solver. `main.cpp` makes use of this in its `Simulate_Life` method, 
which generates a grid of with size determined by user, and outputs the subgrid of each node `n` in iteration `i` 
as the file `subgrid_n_i.txt`, along with a `subgrid.txt` file that contains the shape of each subgrid and a `grid.txt`
file that contains the total dimensions of the Game and the number of processors used in the simulation, to be 
used for post processing.

### Changing Game of Life parameters

You can change the parameters of the Simulate_Life function in the main.cpp with self-explanatory parameters:

``Simulate_Life(int iterations, int width, int height, const bool periodic)``

`periodic` is a bool; if set to `true`, periodic boundaries will be used for our Game of Life simulation, 
else if false the boundaries will not be periodic.

## Displaying the Results

After compiling and running `main.cpp` as above, running the first cell in the notebook `showGame.ipynb` will
diplay the grid of Game of Life for each iteration.
