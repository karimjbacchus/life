// --------------------------------------------------------------
// main.cpp for simulating Game of Life in Parallel with MPI
// 
// Author: Karim Bacchus (kb514), CID: 000942807
// --------------------------------------------------------------
#include <mpi.h>
#include <cmath>
#include <iostream>
#include <cstdlib>
#include <iomanip>
#include "Life.h"
#include "Life.cpp"
#include <sstream>
#include <fstream>
#include <chrono>

// Uncomment below if doing timings on HPC for report analysis.pdf
//#define DO_TIMING

using namespace std;

int id, p;

// i_max, j_max are the dimensions the processors will be laid out in
// sub_rows, sub_cols are the number of rows, cols each processor is 
// responsible for
int i_max, j_max, sub_rows, sub_cols;

// This computes subrows, subcols for each processor
// and i_max, j_max the closest square representation of our processors. 
void Setup_Life_Grid(const int& width, const int& height);

// This stores each subgrid dimension in subgrid.txt:
// We store the number of rows in the ith node in the first row of subgrid.txt
// and the number of columns in the second row, overall this is a 2 x N matrix.
// Additionally  we store the overall width, height of our Game of Life,
// and the numebr of iterations in grid.txt for postprocessing.
void Store_Grid(const int& iterations, const int& width, const int& height);

// This simulates life for num. of iterations, of domain of size width x height
// Parameter periodic sets whether the domain boundaries are periodic or not.
void Simulate_Life(int iterations, int width, int height, const bool periodic);


// MAIN FUNCTION!
int main(int argc, char *argv[])
{
	MPI_Init(&argc, &argv);

	MPI_Comm_rank(MPI_COMM_WORLD, &id);
	MPI_Comm_size(MPI_COMM_WORLD, &p);

	// Simulate 100 Game Steps with a grid of 18x12
	Simulate_Life(100, 18, 12, true);

	MPI_Finalize();
}


void Simulate_Life(int iterations, int width, int height, const bool periodic){	

	// Setup the dimensions of our grid and subgrid for each processor
	Setup_Life_Grid(width, height);

	// And store it as subgrid.txt
	#ifndef DO_TIMING // skip writing data when using timing	
	Store_Grid(iterations, width, height);	
	#endif

	// Set up the initial conditions of our Game of Life
	Life A; 	
	A.Set_Size(sub_rows+2, sub_cols+2);

	// Set each submatrix to random 
	//A.Fill_Random();

	// initialize glider; nice pattern for testing
	if (id == 0){
		// glider 
		A.m_data[1][1] = 1; 
		A.m_data[2][2] = 1; 
		A.m_data[2][3] = 1; 
		A.m_data[3][1] = 1; 
		A.m_data[3][2] = 1;
	}

	MPI_Barrier(MPI_COMM_WORLD);

	#ifndef DO_TIMING // skip writing data when using timing
	// save first grid to file	
	A.grid_to_file(id, 0);
	MPI_Barrier(MPI_COMM_WORLD);
	#endif

	#ifdef DO_TIMING
	//The timing starts here
	auto start = chrono::high_resolution_clock::now();
	#endif

	for (int n = 1; n <= iterations; n++){
		
		A.Update_Edges(i_max, j_max, id, p, periodic);

		MPI_Barrier(MPI_COMM_WORLD);

		A.Update_Game();
	
		#ifndef DO_TIMING // skip writing data when using timing

		A.grid_to_file(id, n);
		
		#endif
	}


	#ifdef DO_TIMING
	// Print out the time taken if we are doing timing to the console. 
	// N.B. we don't need an additional barrier as the preceding line A.Update_Game();
	// ends with a MPI Barrier in its definition, so we will get accurate timings.
	auto finish = chrono::high_resolution_clock::now();
	if (id == 0)
	{
		std::chrono::duration<double> elapsed = finish - start;
		cout << setprecision(5);
		cout << elapsed.count() << endl;
	}
	#endif

}


void Setup_Life_Grid(const int& width, const int& height)
{

	// work out the closest square to p
	for (int n = 1; n <= sqrt(p); n++){
		if (p % n == 0)
			i_max = n;
	}
	j_max = p/i_max;

	sub_rows = (int) height/i_max;	
	sub_cols = (int) width/j_max;

	// we need to add extra rows, cols if we have a remainder when dividing above
	int width_remainder = width % j_max;
	for (int i = 0; i < width_remainder; i++){
		for (int j = 0; j < i_max; j++){
			if (id == j*j_max + i){
				sub_cols += 1;
			}
		}
	}

	int height_remainder  = height % i_max;
	for (int i = 0; i < height_remainder; i++){
		for (int j = 0; j < j_max; j++){
			if (id == i*j_max + j){
				sub_rows += 1;
			}
		}
	}	

	MPI_Barrier(MPI_COMM_WORLD);
}


void Store_Grid(const int& iterations, const int& width, const int& height)
{
	int *x_mat = nullptr; 
	int *y_mat = nullptr; 

	if (id == 0){
		x_mat = new int[p];
		y_mat = new int[p];
	}

	MPI_Gather(&sub_rows, 1, MPI_INT, x_mat, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Gather(&sub_cols, 1, MPI_INT, y_mat, 1, MPI_INT, 0, MPI_COMM_WORLD);

	// We store the number of rows and columns each processor is responsible 
	// for in subgrid.txt for postprocessing, and dimensions of complete game
	// in grid.txt
	if (id == 0){
		stringstream output;
		fstream file;
		output << "subgrid.txt";
		//open file
		file.open(output.str().c_str(), ios_base::out);
		
		// save each subgrid dimensions to subgrid.txt
		for (int i = 0; i < p; i++){
			file << x_mat[i];
			if (i != p-1)
				file << ",";
		}
		file << endl;

		for (int i = 0; i < p; i++){
			file << y_mat[i]; 
			if (i != p-1)
				file << ",";
		}

		//close file
		file.close();


		// save height, width, number of processors and iterations in grid.txt file
		stringstream output_2;
		fstream file_2;
		output_2 << "grid.txt";
		//open file
		file_2.open(output_2.str().c_str(), ios_base::out);
		
		// save grid to file
		file_2 << height << "," << width << "," << p << "," << iterations;

		//close file
		file_2.close();

	}

	delete [] x_mat;
	delete [] y_mat;

}