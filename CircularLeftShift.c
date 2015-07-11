#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define N 20

int MPI_Check_Error( int status ) {
#if defined(DEBUG) || defined(_DEBUG)
//    printf( "Checking...\n" ); 
    if ( status != MPI_SUCCESS )
        exit( EXIT_FAILURE );
#endif

    return status;
}

int main ( int argc, char *argv[] ) {
    MPI_Check_Error( MPI_Init( NULL, NULL ) );

    // evaluate the size of the communicator and 
    // who I am
    int np;
    MPI_Comm_size( MPI_COMM_WORLD, &np );
    int rank;
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    // Initialize the ring topology
    int  leftNeighbour = ( rank + np - 1 ) % np;
    int rightNeighbour = ( rank + 1 ) % np;

    int shift;
    // master thread sends the value of the shift
    if( ! rank ) {
        shift = atoi( argv[1] );
//        printf( "shift %d\n", shift );
    }

    MPI_Bcast( &shift, 1, MPI_INT, 0, MPI_COMM_WORLD );

    short unsigned *value = (short unsigned *) calloc( N / np, sizeof( value[0] ) );
    if( ! value )
        exit( EXIT_FAILURE );

    // toggle first element
    if( ! rank )
        value[0] = 1;

    // (left) shift the array
    int tempBuffer;
    MPI_Status status;
    // XXX  this may lead to a deadlock !!
//    for ( unsigned int i = 0; i < N / np; ++ i ){
//        MPI_Send( &value[0], 1, MPI_UNSIGNED_SHORT_INT, leftNeighbour, 0, MPI_COMM_WORLD );
//        MPI_Recv( &tempBuffer, 1, MPI_UNSIGNED_SHORT_INT, rightNeighbour, 0, MPI_COMM_WORLD, &status );
//
//        unsigned int j;
//        for( j = 1; j < N / np; ++ j )
//            value[j-1] = value[j];
//
//        value[j] = tempBuffer;
//    }

    // solution
    for ( unsigned int i = 0; i < shift; ++ i ) {
        if( !rank ) { // master process

            MPI_Send( &value[0], 1, MPI_UNSIGNED_SHORT, leftNeighbour, 0, MPI_COMM_WORLD );

            unsigned int j;
            for( j = 0; j < N / np - 1; ++ j )
                value[j] = value[j+1];

            MPI_Recv( &value[j], 1, MPI_UNSIGNED_SHORT, rightNeighbour, 0, MPI_COMM_WORLD, &status );
            
        } else { // other processes
            // save last value
            tempBuffer = value[0];

            // shift other values
            unsigned int j;
            for( j = 0; j < N / np - 1; ++ j )
                value[j] = value[j+1];

            // receive value in last element
            MPI_Recv( &value[j], 1, MPI_UNSIGNED_SHORT, rightNeighbour, 0, MPI_COMM_WORLD, &status );

            // send the buffer
            MPI_Send( &tempBuffer, 1, MPI_UNSIGNED_SHORT, leftNeighbour, 0, MPI_COMM_WORLD );
        }
    }

    // output the result
    int flag;
    if( ! rank ) {
        // XXX that's just use to sequentialize the printing
        flag = 1;
        for ( unsigned int i = 0; i < N / np; ++ i )
            printf( "  %2d: %4d\n", i + rank * ( N / np ), value[i] );

        MPI_Send( &flag, 1, MPI_INT, rightNeighbour, 0, MPI_COMM_WORLD );
    }
    else {
        MPI_Recv( &flag, 1, MPI_INT, leftNeighbour, MPI_ANY_SOURCE, MPI_COMM_WORLD, &status );
        for ( unsigned int i = 0; i < N / np; ++ i )
            printf( "  %2d: %4d\n", i + rank * ( N / np ), value[i] );
        
        // last send won't be received by any process
        MPI_Send( &flag, 1, MPI_INT, rightNeighbour, 0, MPI_COMM_WORLD );
    }

    MPI_Check_Error( MPI_Finalize() );

    return 0;
}
