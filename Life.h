// ---------------------------------------------------------------
// Interface of Life Class for simulating Game of Life in Parallel
// 
// Author: Karim Bacchus (kb514), CID: 000942807
// ---------------------------------------------------------------
#pragma once

#include <iostream>
#include <mpi.h>
#include <sstream>
#include <fstream>

// The Class Life for Parallel stores the subgrid of a Game of Life grid as a 2D matrix 
// of bools, m_data. It additionally stores the neighbour count in the 2D matrix m_nbr, used
// for playing the Game of Life, Update_Game()
// 
// After each Game of Life Iteration, we must communicate to each node the new boundary of its
// neighbours, in Do_Exchanges() which does the appropriate sending and recieving of rows/cols.
class Life
{
public:

    // 2D Matrix m_data stores the actual Game of Life grid (which points to a 1D
    // array of bools, data). 
    // Public as we want to be able to change data when setting it outside of class    
    bool **m_data;
    bool *data;

    // constructor
    Life()
    {
        MPI_matrix_type = nullptr;
    }

    // destructor
    ~Life()
    {
        int flag;
        MPI_Finalized(&flag);
        if (!flag && MPI_matrix_type != nullptr)
            MPI_Type_free(MPI_matrix_type);
        delete MPI_matrix_type;
        if (rows > 0){
            delete[] m_data; 
            delete[] data; 
        }
    }

    // Set Size of Matrix to i,j 
    void Set_Size(int i, int j);

    // Fill matrix with random bools
    void Fill_Random();

    // Fill matrix with ones
    void Fill_Ones();

    // Prints our matrix to display
    void Print_Matrix(void); 

    // Updates ghost edges from neighbouring subgrids; 
    // Optional: periodic = true uses periodic boundaries
    void Update_Edges(int& domain_rows, int& domain_cols, int& id, int& p, const bool periodic);

    // Updates all subgrids values by game of life rules
    void Update_Game(void);

    // Save each subgrid as a file for iteration n
    void grid_to_file(const int& id, int n);

private:
    // We never need to change these outside of class methods, so set to private
    int rows, columns; 

    // Stores neighbour count for each grid point
    int **m_nbr; 
    int *nbr;

    MPI_Datatype *MPI_matrix_type;

    // Sets up our MPI type row or column in preparation for sending
    void setup_MPI_type_row(int row_index);
    void setup_MPI_type_column(int col_index);

    // Required if swapping type of send use
    void Clear_MPI_Type();

    // These methods send and recieve entire rows and columns between processor nodes
    // For our communication step to update the ghost boundaries in each subgrid
    void send_matrix_row(int destination, int tag_num, int row_index, MPI_Request *request);
    void receive_matrix_row(int source, int tag_num, int row_index, MPI_Request *request);

    void send_matrix_column(int destination, int tag_num, int col_index, MPI_Request *request);
    void receive_matrix_column(int destination, int tag_num, int col_index, MPI_Request *request);

    // Counts and updates neighbour matrix, as part of Update_Game 
    void Count_nbr(void);

    // Actually do our ghost boundary exchanges from current node to com_id, where  
    // i, j are in [-1, 0, 1] describe the relation between current node and node com_id
    void Do_Exchanges(const int &i, const int &j, const int& com_id, const int cnt, MPI_Request *requests);
};

