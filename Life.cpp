// --------------------------------------------------------------------
// Implementation of Life Class for simulating Game of Life in Parallel
// 
// Author: Karim Bacchus (kb514), CID: 000942807
// --------------------------------------------------------------------
#include "Life.h"
#include <iostream>
#include <mpi.h>
#include <sstream>
#include <fstream>

void Life::Set_Size(int i, int j)
{
    rows = i; 
    columns = j;

    // set up matrix to contain grid for Game of Life, m_data
    data = new bool[rows*columns];
    m_data = new bool*[rows];
    for (int i = 0; i < rows; i++)
        m_data[i] = &data[i*columns];
    for (int i =0 ; i < rows*columns; i++)
        data[i] = 0;
    
    // set up matrix to count neighbours, m_nbr
    nbr = new int[(rows-2)*(columns-2)];
    m_nbr = new int*[rows-2];
    for (int i = 0; i < rows-2; i++)
        m_nbr[i] = &nbr[i*(columns-2)];

}

void Life::Print_Matrix(void)
{
    for (int i = 0; i < rows; i++){
        for (int j = 0; j < columns; j++){
            std::cout << m_data[i][j] << ";";
            std::cout.flush();
        }
        std::cout << std::endl << std::endl;
        std::cout.flush();
    }
}

void Life::setup_MPI_type_row(int row_index)
{
	if (MPI_matrix_type == nullptr)
	{
		int block_length = columns-2;
		MPI_Datatype type = MPI_CXX_BOOL;

        // stores the address of the first element in row, excluding `ghost`
        // element at index 0
		MPI_Aint temp;
		MPI_Get_address(&m_data[row_index][1], &temp);
		MPI_Aint address = temp;

		MPI_matrix_type = new MPI_Datatype;
		MPI_Type_create_struct(1, &block_length, &address, &type, MPI_matrix_type);
		MPI_Type_commit(MPI_matrix_type);
	}
}

void Life::setup_MPI_type_column(int col_index)
{
	if (MPI_matrix_type == nullptr)
	{

        int block_length[rows-2];
        MPI_Aint addresses[rows-2];
        MPI_Datatype typelist[rows-2];

        // stores the address of each element in column
        for (int i = 0; i < rows-2; i++)
        {
        	block_length[i] = 1;
        	MPI_Aint temp;
            // i+1 to exclude the `ghost` element at index 0
        	MPI_Get_address(&m_data[i+1][col_index], &temp);
        	addresses[i] = temp;
        	typelist[i] = MPI_CXX_BOOL;
        }

		MPI_matrix_type = new MPI_Datatype;
		MPI_Type_create_struct(rows-2 , &block_length[0], &addresses[0], &typelist[0], MPI_matrix_type);
		MPI_Type_commit(MPI_matrix_type);
	}
}


//required if swapping type of send use
void Life::Clear_MPI_Type()
{
    if (MPI_matrix_type != nullptr)
        MPI_Type_free(MPI_matrix_type);
    delete MPI_matrix_type;
    MPI_matrix_type = nullptr;
}

void Life::send_matrix_row(int destination, int tag_num, int row_index, MPI_Request *request)
{
    setup_MPI_type_row(row_index);
    MPI_Isend(MPI_BOTTOM, 1, *MPI_matrix_type, destination, tag_num, MPI_COMM_WORLD, request);
    Clear_MPI_Type();
}

void Life::receive_matrix_row(int source, int tag_num, int row_index, MPI_Request *request)
{
    setup_MPI_type_row(row_index);
    MPI_Irecv(MPI_BOTTOM, 1, *MPI_matrix_type, source, tag_num, MPI_COMM_WORLD, request);
    Clear_MPI_Type();
}

void Life::send_matrix_column(int destination, int tag_num, int col_index, MPI_Request *request)
{
    setup_MPI_type_column(col_index);
    MPI_Isend(MPI_BOTTOM, 1, *MPI_matrix_type, destination, tag_num, MPI_COMM_WORLD, request);
    Clear_MPI_Type();
}

void Life::receive_matrix_column(int source, int tag_num, int col_index, MPI_Request *request)
{
    setup_MPI_type_column(col_index);
    MPI_Irecv(MPI_BOTTOM, 1, *MPI_matrix_type, source, tag_num, MPI_COMM_WORLD, request);
    Clear_MPI_Type();
}


void Life::Fill_Random()
{
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < columns; j++)
            if (rand() % 2 == 0)
                m_data[i][j] = 1;
}

void Life::Fill_Ones()
{
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < columns; j++)
                m_data[i][j] = 1;
}


void Life::Do_Exchanges(const int &i, const int &j, const int& com_id, const int cnt, MPI_Request *requests)
{
    // Up/Down Exchanges 
    if (i == 1 && j == 0){
        send_matrix_row(com_id, 1, rows-2, &requests[cnt * 2]);
        receive_matrix_row(com_id, 2, rows-1, &requests[cnt * 2 + 1]);
    }
    else if (i == -1 && j == 0){
        receive_matrix_row(com_id, 1, 0, &requests[cnt * 2]);
        send_matrix_row(com_id, 2, 1, &requests[cnt * 2 + 1]);
    }

    // Left/Right Exchanges 
    else if (i == 0 && j == 1){
        send_matrix_column(com_id, 3, columns-2, &requests[cnt * 2]);
        receive_matrix_column(com_id, 4, columns-1, &requests[cnt * 2 + 1]);
    }
    else if (i == 0 && j == -1){
        receive_matrix_column(com_id, 3, 0, &requests[cnt * 2]);
        send_matrix_column(com_id, 4, 1, &requests[cnt * 2 + 1]);
    }          

    // Negative Diagonal Exchanges 
    else if (i == 1 && j == 1){
        MPI_Isend(&m_data[rows-2][columns-2], 1, MPI_CXX_BOOL, com_id, 5, MPI_COMM_WORLD,  &requests[cnt * 2]);
        MPI_Irecv(&m_data[rows-1][columns-1], 1, MPI_CXX_BOOL, com_id, 6, MPI_COMM_WORLD,  &requests[cnt * 2+1]);
    }
    else if (i == -1 && j == -1){
        MPI_Irecv(&m_data[0][0], 1, MPI_CXX_BOOL, com_id, 5, MPI_COMM_WORLD,  &requests[cnt * 2+1]);
        MPI_Isend(&m_data[1][1], 1, MPI_CXX_BOOL, com_id, 6, MPI_COMM_WORLD,  &requests[cnt * 2]);
    }                

    // Positive Diagonal Exchanges 
    else if (i == -1 && j == 1){
        MPI_Isend(&m_data[1][columns-2], 1, MPI_CXX_BOOL, com_id, 7, MPI_COMM_WORLD,  &requests[cnt * 2]);
        MPI_Irecv(&m_data[0][columns-1], 1, MPI_CXX_BOOL, com_id, 8, MPI_COMM_WORLD,  &requests[cnt * 2+1]);
    }
    else if (i == 1 && j == -1){
        MPI_Irecv(&m_data[rows-1][0], 1, MPI_CXX_BOOL, com_id, 7, MPI_COMM_WORLD,  &requests[cnt * 2+1]);
        MPI_Isend(&m_data[rows-2][1], 1, MPI_CXX_BOOL, com_id, 8, MPI_COMM_WORLD,  &requests[cnt * 2]);
    }
}


void Life::Update_Edges(int& domain_rows, int& domain_cols, int& id, int& p, const bool periodic)
{
    // potentially 8 sends and 8 recieves for edges
    MPI_Request *requests = new MPI_Request[8*2];

    int cnt = 0;

    // row and column of current processor
    int id_row = (int) id/domain_cols; 
    int id_column = id % domain_cols;

    // variables to tell us which processor to communicate with
    int com_id, com_i, com_j; 

    for (int i = -1; i <=1; i++){
        for (int j = -1; j <=1; j++){

            // skip communication with itself
            if (i == 0 && j == 0)
                continue;
            
            // row and column of processor to communiate with
            com_i = id_row + i; 
            com_j = id_column + j;

            // Cases when we've gone outside height of domain
            // If periodic, we set com_i to relevant row
            // else we skip this i,j as we're outside the domain
            if (com_i >= domain_rows){
                if (periodic){ 
                    com_i = 0;
                }
                else continue;
            }
            else if (com_i < 0){
                if (periodic){ 
                    com_i = domain_rows - 1;
                }
                else continue;
            }
            
            // Cases when we've gone outside width of domain
            // If periodic, we set com_j to relevant column
            // else we skip this i,j as we're outside the domain
            if (com_j >= domain_cols){
                if (periodic) {
                    com_j = 0;
                }
                else continue;
            }
            else if (com_j < 0){
                if (periodic) { 
                    com_j = domain_cols - 1; 
                }
                else continue;
            }
            
            // Calculate id of processor to communicate with
            com_id = com_i * domain_cols + com_j;

            // DO OUR EXCHANGES! 
            Do_Exchanges(i, j, com_id, cnt, requests);

            // Increment counter of number of requests/2
            cnt++;
        }

    }

    MPI_Waitall(cnt * 2, requests, MPI_STATUS_IGNORE);

    delete[] requests;

}

void Life::grid_to_file(const int& id, int n)
{
    std::stringstream output;
    std::fstream file;
    output << "subgrid" << "_" << id << "_" << n << ".txt";
    //open file
    file.open(output.str().c_str(), std::ios_base::out);
    
    // save grid to file
    for (int i = 1; i < rows-1; i++)
    {
        for (int j = 1; j < columns-1; j++){
            file << m_data[i][j];
            if (j != columns -2)
                file << ",";
        }
        file << std::endl;
    }

    //close file
    file.close();
}


void Life::Count_nbr(void)
{
    for (int i = 1; i < rows -1; i++){
        for (int j = 1; j < columns-1; j++){
            m_nbr[i-1][j-1] = 0;
            m_nbr[i-1][j-1] += m_data[i-1][j]; 
            m_nbr[i-1][j-1] += m_data[i+1][j]; 
            m_nbr[i-1][j-1] += m_data[i][j-1]; 
            m_nbr[i-1][j-1] += m_data[i][j+1]; 
            m_nbr[i-1][j-1] += m_data[i-1][j-1]; 
            m_nbr[i-1][j-1] += m_data[i-1][j+1]; 
            m_nbr[i-1][j-1] += m_data[i+1][j-1]; 
            m_nbr[i-1][j-1] += m_data[i+1][j+1]; 
        }
    }
}

void Life::Update_Game(void)
{   
    Count_nbr();
    for (int i = 1; i < rows-1; i++){
        for (int j = 1; j < columns-1; j++){

            // dead cell lives if exactly 3 neighbours
            if (m_data[i][j] == 0) {
                if (m_nbr[i-1][j-1] == 3)
                    m_data[i][j] = 1;
            }
            // under/overcrowded living cell dies
            else {
                if (m_nbr[i-1][j-1] < 2 || m_nbr[i-1][j-1] > 3)
                    m_data[i][j] = 0;
            }
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
}
