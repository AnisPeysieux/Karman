/********************  HEADERS  *********************/
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "lbm_comm.h"


/*******************  FUNCTION  *********************/
int lbm_helper_pgcd(int a, int b)
{
	int c;
	while(b!=0)
	{
		c = a % b;
		a = b;
		b = c;
	}
	return a;
}

/*******************  FUNCTION  *********************/
/**
 * Affiche la configuation du lbm_comm pour un rank donné
 * @param mesh_comm Configuration à afficher
**/
void  lbm_comm_print( lbm_comm_t *mesh_comm )
{
	int rank ;
	MPI_Comm_rank( MPI_COMM_WORLD, &rank );
	printf( " RANK %d ( LEFT %d RIGHT %d TOP %d BOTTOM %d CORNER %d, %d, %d, %d ) ( POSITION %d %d ) (WH %d %d ) \n", rank,
									    mesh_comm->left_id,
									    mesh_comm->right_id,
										mesh_comm->top_id,
									    mesh_comm->bottom_id,
										mesh_comm->corner_id[0],
		 								mesh_comm->corner_id[1],
		 								mesh_comm->corner_id[2],
		 		 						mesh_comm->corner_id[3],
									    mesh_comm->x,
									    mesh_comm->y,
									    mesh_comm->width,
									    mesh_comm->height );
}

/*******************  FUNCTION  *********************/
int helper_get_rank_id(int nb_x,int nb_y,int rank_x,int rank_y)
{
	if (rank_x < 0 || rank_x >= nb_x)
		return -1;
	else if (rank_y < 0 || rank_y >= nb_y)
		return -1;
	else
		return (rank_x + rank_y * nb_x);
}


void recv_size(int rank, int comm_size, int* recv_width, int* recv_height)
{
    int nb_y = lbm_helper_pgcd(comm_size,MESH_WIDTH);
    //printf("nb_y = %d\n", nb_y);
	int nb_x = comm_size / nb_y;
    //printf("nb_x = %d\n", nb_x);
	*recv_width = MESH_WIDTH / nb_x + 2;
    //printf("recv_width = %d\n", *recv_width);
	*recv_height = MESH_HEIGHT / nb_y + 2;
    //printf("recv_height = %d\n", *recv_height);
    int remain_x = MESH_WIDTH % nb_x;
    //printf("remain_x = %d\n", remain_x);
    int remain_y = MESH_HEIGHT % nb_y;
    //printf("remain_y = %d\n", remain_y);
    //printf("rank = %d\n", rank);
    int rank_x = rank % nb_x;
	int rank_y = rank / nb_x;
    if(rank_x < remain_x)
    {
      //  printf("recv_width++\n");
        (*recv_width)++;
    }
    if(rank_y < remain_y)
    {
        //printf("recv_height++\n");
        (*recv_height)++;
    }
    //printf("recv_width = %d\n", *recv_width);
    //printf("recv_height = %d\n", *recv_height);

}
/*******************  FUNCTION  *********************/
/**
 * Initialise un lbm_comm :
 * - Voisins
 * - Taille du maillage local
 * - Position relative
 * @param mesh_comm MeshComm à initialiser
 * @param rank Rank demandant l'initalisation
 * @param comm_size Taille totale du communicateur
 * @param width largeur du maillage
 * @param height hauteur du maillage
**/
void lbm_comm_init( lbm_comm_t * mesh_comm, int rank, int comm_size, int width, int height )
{
	//vars
	int nb_x;
	int nb_y;
	int rank_x;
	int rank_y;
    int remain_x;
    int remain_y;

	//compute splitting
	nb_y = lbm_helper_pgcd(comm_size,width);
	nb_x = comm_size / nb_y;

	//nb_x = lbm_helper_pgcd(comm_size,width);
	//nb_y = comm_size / nb_x;

	//check
	assert(nb_x * nb_y == comm_size);
	if (height % nb_y != 0)
		fatal("Can't get a 2D cut for current problem size and number of processes.");

	//calc current rank position (ID)
	rank_x = rank % nb_x;
	rank_y = rank / nb_x;

	//setup nb
	mesh_comm->nb_x = nb_x;
	mesh_comm->nb_y = nb_y;

	//setup size (+2 for ghost cells on border)
	mesh_comm->width = width / nb_x + 2;
	mesh_comm->height = height / nb_y + 2;
    //printf("ABC rank = %d 2.mesh_comm->width = %d\n", rank, mesh_comm->width);
    //printf("ABC rank = %d 2.mesh_comm->height = %d\n", rank, mesh_comm->height);
    remain_x = width % nb_x;
    remain_y = height % nb_y;
    //printf("ABC rank = %d remain_x = %d\n", rank, remain_x);
    //printf("ABC rank = %d remain_y = %d\n", rank, remain_y);
    if(rank_x < remain_x)
    {
        mesh_comm->width++;
      //  printf("ABC rank = %d mesh_comm->width++ = %d\n", rank, mesh_comm->width);
    }
    if(rank_y < remain_y)
    {
        mesh_comm->height++;
        //printf("ABC rank = %d 2.mesh_comm->height++ = %d\n", rank, mesh_comm->height);
    }
    //printf("ABC rank = %d 2.mesh_comm->width = %d\n", rank, mesh_comm->width);
    //printf("ABC rank = %d 2.mesh_comm->height = %d\n", rank, mesh_comm->height);

	//setup position
	mesh_comm->x = rank_x * width / nb_x;
	mesh_comm->y = rank_y * height / nb_y;
	
	// Compute neighbour nodes id
	mesh_comm->left_id  = helper_get_rank_id(nb_x,nb_y,rank_x - 1,rank_y);
	mesh_comm->right_id = helper_get_rank_id(nb_x,nb_y,rank_x + 1,rank_y);
	mesh_comm->top_id = helper_get_rank_id(nb_x,nb_y,rank_x,rank_y - 1);
	mesh_comm->bottom_id = helper_get_rank_id(nb_x,nb_y,rank_x,rank_y + 1);
	mesh_comm->corner_id[CORNER_TOP_LEFT] = helper_get_rank_id(nb_x,nb_y,rank_x - 1,rank_y - 1);
	mesh_comm->corner_id[CORNER_TOP_RIGHT] = helper_get_rank_id(nb_x,nb_y,rank_x + 1,rank_y - 1);
	mesh_comm->corner_id[CORNER_BOTTOM_LEFT] = helper_get_rank_id(nb_x,nb_y,rank_x - 1,rank_y + 1);
	mesh_comm->corner_id[CORNER_BOTTOM_RIGHT] = helper_get_rank_id(nb_x,nb_y,rank_x + 1,rank_y + 1);

	//if more than 1 on y, need transmission buffer
	if (nb_y > 1)
	{
		mesh_comm->buffer = malloc(sizeof(double) * DIRECTIONS * width / nb_x);
	} else {
		mesh_comm->buffer = NULL;
	}

	//if debug print comm
	#ifndef NDEBUG
	lbm_comm_print( mesh_comm );
	#endif
}


/*******************  FUNCTION  *********************/
/**
 * Libere un lbm_comm
 * @param mesh_comm MeshComm à liberer
**/
void lbm_comm_release( lbm_comm_t * mesh_comm )
{
	mesh_comm->x = 0;
	mesh_comm->y = 0;
	mesh_comm->width = 0;
	mesh_comm->height = 0;
	mesh_comm->right_id = -1;
	mesh_comm->left_id = -1;
	if (mesh_comm->buffer != NULL)
		free(mesh_comm->buffer);
	mesh_comm->buffer = NULL;
}

/*******************  FUNCTION  *********************/
/**
 * Debut de communications asynchrones
 * @param mesh_comm MeshComm à utiliser
 * @param mesh_to_process Mesh a utiliser lors de l'échange des mailles fantomes
**/
int lbm_comm_sync_ghosts_horizontal( lbm_comm_t * mesh, Mesh *mesh_to_process, lbm_comm_type_t comm_type, int target_rank, int x, MPI_Request* req)
{
	//vars
	//MPI_Status status;
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	//if target is -1, no comm
	if (target_rank == -1)
    {
        //if(rank ==5 || rank ==2)
        //printf("rank %d horizontal target = %d return 0\n", rank, target_rank);
		return 0;
    }

	//int y;

	switch (comm_type)
	{
		case COMM_SEND:
			/*for( y = 0 ; y < mesh->height-2 ; y++ )
            {
				MPI_Send( &Mesh_get_col( mesh_to_process, x )[y*DIRECTIONS], DIRECTIONS, MPI_DOUBLE, target_rank, 0, MPI_COMM_WORLD);
            }*/
            
            //MPI_Isend( Mesh_get_cell( mesh_to_process, x, 0), DIRECTIONS*(mesh->height), MPI_DOUBLE, target_rank, 0, MPI_COMM_WORLD, req);
            MPI_Isend( mesh_to_process->cells+x*mesh_to_process->height*DIRECTIONS, DIRECTIONS*(mesh->height), MPI_DOUBLE, target_rank, 0, MPI_COMM_WORLD, req);
			break;
		case COMM_RECV:
			/*for( y = 0 ; y < mesh->height-2 ; y++ )
            {
				MPI_Recv(  &Mesh_get_col( mesh_to_process, x )[y*DIRECTIONS], DIRECTIONS, MPI_DOUBLE, target_rank, 0, MPI_COMM_WORLD,&status);
            }*/
            //MPI_Irecv(  Mesh_get_cell( mesh_to_process, x, 0), DIRECTIONS*(mesh->height), MPI_DOUBLE, target_rank, 0, MPI_COMM_WORLD, req);
            MPI_Irecv(  mesh_to_process->cells+x*mesh_to_process->height*DIRECTIONS, DIRECTIONS*(mesh->height), MPI_DOUBLE, target_rank, 0, MPI_COMM_WORLD, req);
			break;
		default:
			fatal("Unknown type of communication.");
	}
	//if(rank ==5 || rank ==2)
	//printf("rank %d horizontal target = %d return 1\n", rank, target_rank);
	return 1;
}

/*******************  FUNCTION  *********************/
/**
 * Debut de communications asynchrones
 * @param mesh_comm MeshComm à utiliser
 * @param mesh_to_process Mesh a utiliser lors de l'échange des mailles fantomes
**/
int lbm_comm_sync_ghosts_diagonal( lbm_comm_t * mesh, Mesh *mesh_to_process, lbm_comm_type_t comm_type, int target_rank, int x ,int y, MPI_Request* req)
{
	//vars
	//MPI_Status status;
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	//if target is -1, no comm
	if (target_rank == -1)
    {
      //  if(rank ==5 || rank ==2)
      //  printf("rank %d diagonal target = %d return 0\n", rank, target_rank);
		return 0;
    }

	switch (comm_type)
	{
		case COMM_SEND:
            //printf("%d envoi vers %d\n", rank, target_rank);
			MPI_Isend( Mesh_get_cell( mesh_to_process, x, y ), DIRECTIONS, MPI_DOUBLE, target_rank, 0, MPI_COMM_WORLD, req);
			break;
		case COMM_RECV:
            //printf("%d recoit de %d\n", rank, target_rank);
			MPI_Recv( Mesh_get_cell( mesh_to_process, x, y ), DIRECTIONS, MPI_DOUBLE, target_rank, 0, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
			break;
		default:
			fatal("Unknown type of communication.");
	}
	//if(rank ==5 || rank ==2)
	//printf("rank %d diagonal target = %d return 1\n", rank, target_rank);
	return 1;
}


/*******************  FUNCTION  *********************/
/**
 * Debut de communications asynchrones
 * @param mesh_comm MeshComm à utiliser
 * @param mesh_to_process Mesh a utiliser lors de l'échange des mailles fantomes
**/
int lbm_comm_sync_ghosts_vertical( lbm_comm_t * mesh, Mesh *mesh_to_process, lbm_comm_type_t comm_type, int target_rank, int y, MPI_Request* req)
{
	//vars
	//MPI_Status status;
	//int x, k;
    int x;
    int count = 0;
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	//if target is -1, no comm
	if (target_rank == -1)
    {
      //  if(rank ==5 || rank ==2)
        //printf("rank %d vertical target = %d return 0\n", rank, target_rank);
		return 0;
    }

	switch (comm_type)
	{
		case COMM_SEND:
			for ( x = 0 ; x < mesh_to_process->width ; x++)
            {
				/*for ( k = 0 ; k < DIRECTIONS ; k++)
                {
					MPI_Send( &Mesh_get_cell(mesh_to_process, x, y)[k], 1, MPI_DOUBLE, target_rank, 0, MPI_COMM_WORLD);
                }*/
                MPI_Isend( Mesh_get_cell(mesh_to_process, x, y), DIRECTIONS, MPI_DOUBLE, target_rank, 0, MPI_COMM_WORLD, &req[count]);
                count++;
            }
			break;
		case COMM_RECV:
			for ( x = 0 ; x < mesh_to_process->width ; x++)
            {
				/*for ( k = 0 ; k < DIRECTIONS ; k++)
                {
					MPI_Recv( &Mesh_get_cell(mesh_to_process, x, y)[k], DIRECTIONS, MPI_DOUBLE, target_rank, 0, MPI_COMM_WORLD,&status);
                }*/
                MPI_Irecv( Mesh_get_cell(mesh_to_process, x, y), DIRECTIONS, MPI_DOUBLE, target_rank, 0, MPI_COMM_WORLD, &req[count]);
                count++;
                
            }
			break;
		default:
			fatal("Unknown type of communication.");
	}
	//printf("rank %d vertical return %d\n", rank, count);
	//return mesh_to_process->width-2;
	//if(rank ==5 || rank ==2)
	//printf("rank %d vertical target = %d return %d\n", rank, target_rank, count);
    return count;
}

/*******************  FUNCTION  *********************/
void lbm_comm_ghost_exchange(lbm_comm_t * mesh, Mesh *mesh_to_process, MPI_Request* req)
{
	//vars
	int rank, count = 0;

	//get rank
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);

    //printf("%d A\n", rank);
	//Left to right phase : on reçoit à droite et on envoie depuis la gauche
	count += lbm_comm_sync_ghosts_horizontal(mesh,mesh_to_process,COMM_SEND,mesh->right_id,mesh->width - 2, req);
	count += lbm_comm_sync_ghosts_horizontal(mesh,mesh_to_process,COMM_RECV,mesh->left_id,0, &req[count]);
	//prevend comm mixing to avoid bugs
	//MPI_Barrier(MPI_COMM_WORLD);
    //printf("%d B\n", rank);
	// Right to left phase : on reçoit à gauche et on envoie depuis la droite
	count += lbm_comm_sync_ghosts_horizontal(mesh,mesh_to_process,COMM_SEND,mesh->left_id,1,&req[count]);
	count += lbm_comm_sync_ghosts_horizontal(mesh,mesh_to_process,COMM_RECV,mesh->right_id,mesh->width - 1,&req[count]);
	//prevend comm mixing to avoid bugs
	//MPI_Barrier(MPI_COMM_WORLD);
    //printf("%d C\n", rank);	
	//top to bottom : on reçoit en bas et on envoie depuis le hauteur
	count += lbm_comm_sync_ghosts_vertical(mesh,mesh_to_process,COMM_SEND,mesh->bottom_id,mesh->height - 2,&req[count]);
    //printf("%d D\n", rank);
	count += lbm_comm_sync_ghosts_vertical(mesh,mesh_to_process,COMM_RECV,mesh->top_id,0, &req[count]);

	//prevend comm mixing to avoid bugs
	//MPI_Barrier(MPI_COMM_WORLD);
    //printf("%d E\n", rank);
	// Right to left phase : on reçoit en haut et on envoie depuis le bas
	count += lbm_comm_sync_ghosts_vertical(mesh,mesh_to_process,COMM_SEND,mesh->top_id,1, &req[count]);
    //printf("%d F\n", rank);
	count += lbm_comm_sync_ghosts_vertical(mesh,mesh_to_process,COMM_RECV,mesh->bottom_id,mesh->height - 1,&req[count]);
    //printf("%d G\n", rank);
	//prevend comm mixing to avoid bugs
	//MPI_Barrier(MPI_COMM_WORLD);
    //printf("%d H\n", rank);
	//top left
	count += lbm_comm_sync_ghosts_diagonal(mesh,mesh_to_process,COMM_SEND,mesh->corner_id[CORNER_TOP_LEFT],1,1,&req[count]);
	

	//prevend comm mixing to avoid bugs
	//MPI_Barrier(MPI_COMM_WORLD);
    //printf("%d I\n", rank);
	//bottom left
	count += lbm_comm_sync_ghosts_diagonal(mesh,mesh_to_process,COMM_SEND,mesh->corner_id[CORNER_BOTTOM_LEFT],1,mesh->height - 2,&req[count]);
	

	//prevend comm mixing to avoid bugs
	//MPI_Barrier(MPI_COMM_WORLD);
    //printf("%d J\n", rank);
	//top right
	count += lbm_comm_sync_ghosts_diagonal(mesh,mesh_to_process,COMM_SEND,mesh->corner_id[CORNER_TOP_RIGHT],mesh->width - 2,1,&req[count]);
	

	//prevend comm mixing to avoid bugs
	//MPI_Barrier(MPI_COMM_WORLD);
    //printf("%d K\n", rank);
	//bottom left
	//lbm_comm_sync_ghosts_diagonal(mesh,mesh_to_process,COMM_SEND,mesh->corner_id[CORNER_BOTTOM_LEFT],1,mesh->height - 2);
	//lbm_comm_sync_ghosts_diagonal(mesh,mesh_to_process,COMM_RECV,mesh->corner_id[CORNER_TOP_RIGHT],mesh->width - 1,0);

	//prevend comm mixing to avoid bugs
	//MPI_Barrier(MPI_COMM_WORLD);
    //printf("%d L\n", rank);
	//bottom right
	count += lbm_comm_sync_ghosts_diagonal(mesh,mesh_to_process,COMM_SEND,mesh->corner_id[CORNER_BOTTOM_RIGHT],mesh->width - 2,mesh->height - 2,&req[count]);
	

	//prevend comm mixing to avoid bugs
    //MPI_Barrier(MPI_COMM_WORLD);
    //printf("%d M\n", rank);
	// Right to left phase : on reçoit à gauche et on envoie depuis la droite
	//lbm_comm_sync_ghosts_horizontal(mesh,mesh_to_process,COMM_SEND,mesh->left_id,1);
	//lbm_comm_sync_ghosts_horizontal(mesh,mesh_to_process,COMM_RECV,mesh->right_id,mesh->width - 1);
	
	//wait for IO to finish, VERY important, do not remove.
	//FLUSH_INOUT();
    //printf("rank %d count = %d\n", rank, count);
    
    //MPI_Barrier(MPI_COMM_WORLD);
    MPI_Waitall(count, req, MPI_STATUSES_IGNORE);
    lbm_comm_sync_ghosts_diagonal(mesh,mesh_to_process,COMM_RECV,mesh->corner_id[CORNER_BOTTOM_RIGHT],mesh->width - 1,mesh->height - 1,&req[count]);
    lbm_comm_sync_ghosts_diagonal(mesh,mesh_to_process,COMM_RECV,mesh->corner_id[CORNER_TOP_RIGHT],mesh->width - 1,0,&req[count]);
    lbm_comm_sync_ghosts_diagonal(mesh,mesh_to_process,COMM_RECV,mesh->corner_id[CORNER_BOTTOM_LEFT],0,mesh->height - 1,&req[count]);
    lbm_comm_sync_ghosts_diagonal(mesh,mesh_to_process,COMM_RECV,mesh->corner_id[CORNER_TOP_LEFT],0,0,&req[count]);
    //MPI_Barrier(MPI_COMM_WORLD);
}

/*******************  FUNCTION  *********************/
/**
 * Rendu du mesh en effectuant une réduction a 0
 * @param mesh_comm MeshComm à utiliser
 * @param temp Mesh a utiliser pour stocker les segments
**/
void save_frame_all_domain( FILE * fp, Mesh *source_mesh, Mesh *temp )
{
	//vars
	int i = 0;
	int comm_size, rank ;
    int recv_width, recv_height;
	MPI_Status status;

	//get rank and comm size
	MPI_Comm_size( MPI_COMM_WORLD, &comm_size );
	MPI_Comm_rank( MPI_COMM_WORLD, &rank );

	/* If whe have more than one process */
	if( 1 < comm_size )
	{
		if( rank == 0 )
		{
			/* Rank 0 renders its local Mesh */
			save_frame(fp,source_mesh);
			/* Rank 0 receives & render other processes meshes */
			for( i = 1 ; i < comm_size ; i++ )
			{
                /*MPI_Recv( &(temp->width), 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status );
                MPI_Recv( &(temp->height), 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status );
				MPI_Recv( temp->cells, temp->width  * temp->height * DIRECTIONS, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, &status );*/

				recv_size(i, comm_size, &recv_width, &recv_height);
                //printf("rang %d reception %d de %d elments (W:%d H:%d)\n", rank, i, recv_width  * recv_height * DIRECTIONS, recv_width, recv_height);fflush(stdout);
                temp->width = recv_width;
                temp->height = recv_height;
				MPI_Recv( temp->cells, recv_width  * recv_height * DIRECTIONS, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, &status );
				save_frame(fp,temp);
			}
		} else {
			/* All other ranks send their local mesh */
			/*MPI_Send( &(source_mesh->width), 1, MPI_INT, 0, 0, MPI_COMM_WORLD );
            MPI_Send( &(source_mesh->height), 1, MPI_INT, 0, 0, MPI_COMM_WORLD );*/
            //printf("rang %d envoi %d de %d elments (W:%d H:%d)\n", rank, 0, source_mesh->width * source_mesh->height * DIRECTIONS, source_mesh->width, source_mesh->height);fflush(stdout);
			MPI_Send( source_mesh->cells, source_mesh->width * source_mesh->height * DIRECTIONS, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD );
		}
	} else {
		/* Only 0 renders its local mesh */
		save_frame(fp,source_mesh);
	}

}

void save_frame_all_domain2( FILE * fp, Mesh *source_mesh )
{
	//vars
	int comm_size, rank ;
	MPI_Status status;
    int nb_y;
	int nb_x;
    int rank_x = 1;
    int rank_y = 0;
    int recv_rank;
    Mesh temp;
	//get rank and comm size
	MPI_Comm_size( MPI_COMM_WORLD, &comm_size );
	MPI_Comm_rank( MPI_COMM_WORLD, &rank );
	nb_y = lbm_helper_pgcd(comm_size, MESH_WIDTH);
	nb_x = comm_size / nb_y;
    Mesh_init( &temp, MESH_WIDTH + 2, MESH_HEIGHT + 2 );
    //printf("temp.width = %d temp.height = %d\n", temp.width, temp.height);
    
	/* If whe have more than one process */
	if( 1 < comm_size )
	{
      //  printf("rank %d (source_mesh -> height - 2) * DIRECTIONS = %d\n", rank, (source_mesh -> height - 2) * DIRECTIONS); fflush(stdout);
        int size = (source_mesh -> height - 2) * DIRECTIONS;
		if( rank == 0 )
		{

			/* Rank 0 receives & render other processes meshes */
			for( rank_x = 0 ; rank_x < nb_x ; rank_x++ )
			{
                for( rank_y = 0; rank_y < nb_y; rank_y++)
                {
                    recv_rank = helper_get_rank_id(nb_x,nb_y,rank_x, rank_y);
                    if(recv_rank == 0)
                    {
                        for(int i = 1; i < source_mesh -> width - 1; ++i)
                        {
                            for(int j = 1; j < source_mesh -> height - 1; ++j)
                            {
                                for(int k = 0; k < DIRECTIONS; ++k)
                                {
                                    Mesh_get_cell(&temp, i, j)[k] = Mesh_get_cell(source_mesh, i, j)[k];
                                }
                            }
                        }
                        continue;
                    }
                    for(int j = 0; j < source_mesh -> width - 2; ++j)
                    {
                        int x = rank_x * (source_mesh -> width - 2) + j + 1;
                        int y = rank_y * (source_mesh -> height - 2) + 1;
        //                printf("rang %d reception %d de %d a l'incide (%d, %d)\n", 0, j, recv_rank, x, y);fflush(stdout);
                    
                        MPI_Recv( Mesh_get_cell(&temp, x, y), size, MPI_DOUBLE, recv_rank, 0, MPI_COMM_WORLD, &status );
                    }
                }
			}
			save_frame(fp,&temp);
		} else {
			/* All other ranks send their local mesh */
            for(int j = 1; j < source_mesh -> width - 1; ++j)
            {
          //      printf("rang %d envoi %d a %d\n", rank, j, 0);fflush(stdout);
                MPI_Send( Mesh_get_col(source_mesh, j), size, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD );
            }
		}
	} else {
		/* Only 0 renders its local mesh */
		save_frame(fp,source_mesh);
	}

}

