#include "mpi.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <cmath>
using namespace std;

bool tieneRaizEntera(double x) {
    if (x >= 0) {
        int raiz = sqrt(x);
        return (raiz * raiz == x);
    } else {
        return false;
    }
}

int main(int argc, char *argv[]) {
    int rank, size, n;
    int rank_2d;
    int rank_fila, rank_columna, rank_diagonal, size_fila, size_columna, size_diagonal;
    int **A, *x, *y, *miBloque, *comprueba, *subFinal, *buf_envio;
    double tInicio, tFin, tInicioSec, tFinSec;
    bool modoGraficas = false;
    MPI_Status estado;
    MPI_Comm comm_2d, comm_fila, comm_columna, comm_diagonal;
    MPI_Datatype MPI_BLOQUE;

    // Inicialización del entorno MPI
    MPI_Init(&argc, &argv); 
    // Obtenemos el número de procesos en el comunicador global
    MPI_Comm_size(MPI_COMM_WORLD, &size); 
    // Obtenemos la identificación de nuestro proceso en el comunicador global
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Comprobar que el número de procesos p tiene raíz entera
    if (!tieneRaizEntera(size)) {
        if (rank == 0)
            cout << "Número de procesos p=" << size << " inválido. "
                 << "El número de procesos debe tener raíz entera (4, 9, 16...)." << endl;
        
        exit(-1);
    }

    // Ajustar precisión de los datos
    cout << fixed;
    cout << setprecision(10);

    // Crear topología cartesiana de procesos 2d
    int ndims = 2;
    if (rank == 0)
        cout << "Creando topología cartesiana de 2 dimensiones" << endl;
    int dims[ndims];
    int period[ndims];
    dims[0] = dims[1] = sqrt(size);
    period[0] = period[1] = 1;
    MPI_Cart_create(MPI_COMM_WORLD, ndims, dims, period, 0, &comm_2d);
    // Obtenemos el id de proceso en el comunicador cartesiano
    MPI_Comm_rank(comm_2d, &rank_2d);
    // Obtenemos coordenadas del proceso dentro del comunicador cartesiano
    int coords[2];
    MPI_Cart_coords(comm_2d, rank_2d, 2, coords);

    // Comunicador para procesos de las filas, columnas y diagonal
    MPI_Comm_split(comm_2d, coords[0], rank, &comm_fila);
    // Comunicador para procesos de las columnas
    MPI_Comm_split(comm_2d, coords[1], rank, &comm_columna);
    // Comunicador para procesos de la diagonal
    MPI_Comm_split(MPI_COMM_WORLD, coords[0] == coords[1], rank, &comm_diagonal);

    // Obtener el nuevo rango asignado 
    MPI_Comm_rank(comm_fila, &rank_fila);
    MPI_Comm_rank(comm_columna, &rank_columna);
    MPI_Comm_rank(comm_diagonal, &rank_diagonal);
    // Obtener el nuevo número de procesos en el comunicador
    MPI_Comm_size(comm_fila, &size_fila);
    MPI_Comm_size(comm_columna, &size_columna);
    MPI_Comm_size(comm_diagonal, &size_diagonal);


    // Leer parámetros
    /*if (argc < 2) {
        cout << "Número de parámetros incorrecto, es necesario indicar el tamaño del vector" << endl;
        exit (-1);
    } else {
        // Se asume que n es múltiplo de raiz de size
        n = atoi(argv[1]);

        // Comprobar si se cambia el modo de mostrar los resultados
        if (argc == 3) {
            modoGraficas = atoi(argv[2]);
        }
    }

    // Reserva de vectores y matriz
    A = new int *[n];
    x = new int [n];

    // El proceso P0 genera inicialmente la matriz A
    if (rank == 0) {
        A[0] = new int [n * n];
        for (unsigned int i = 1 ; i < n ; i++) {
            A[i] = A[i - 1] + n;
        }

        // Reservamos espacio para el resultado
        y = new int [n];

        // Rellenamos A y x con valores aleatorios
        srand(time(0));

        if (!modoGraficas)
            cout << "La matriz y el vector generados son " << endl;
        for (unsigned int i = 0 ; i < n ; i++) {
            for (unsigned int j = 0 ; j < n ; j++) {
                if (j == 0) {
                    if (!modoGraficas)
                        cout << "[";
                }

                A[i][j] = rand() % 1000;

                if (!modoGraficas) {
                    cout << A[i][j];

                    if (j == n - 1) {
                        cout << "]";
                    } else {
                        cout << " ";
                    }
                }  
            }

            x[i] = rand() % 100;

            if (!modoGraficas)
                cout << "\t [" << x[i] << "]" << endl;
        }

        // Reservamos espacio para la comprobación
        comprueba = new int [n];

        tInicioSec = MPI_Wtime();

        // Lo calculamos de forma secuencial
        for (unsigned int i = 0 ; i < n ; i++) {
            comprueba[i] = 0;
            
            for (unsigned int j = 0 ; j < n ; j++) {
                comprueba[i] += A[i][j] * x[j];
            }
        }

        tFinSec = MPI_Wtime();
    }

    // Número de elementos y filas de la matriz que tiene cada proceso
    int nfilas = n / size;
    int nelementos = (n * n) / size;

    // Reservamos espacio para el bloque de filas local de cada proceso
    miBloque = new int [nelementos];
    // Reservamos espacio para los resultados que calcula cada proceso
    // Uno por fila que tiene
    subFinal = new int [nfilas];

    // Repartimos n/p filas por cada proceso
    MPI_Scatter(A[0], nelementos, MPI_INT, miBloque, nelementos, MPI_INT, 0, MPI_COMM_WORLD);

    // Compartimos el vector entre todos los procesos
    MPI_Bcast(x, n, MPI_INT, 0, MPI_COMM_WORLD);

    // Barrera para asegurar que todos los procesos comiencen a la vez
    MPI_Barrier(MPI_COMM_WORLD);
    tInicio = MPI_Wtime();

    // Se tiene más de un subvector final, se calcula cada vez un resultado
    for (int i = 0 ; i < nfilas ; ++i) {
        subFinal[i] = 0;

        for (int j = 0 ; j < n ; j++) {
            subFinal[i] += miBloque[j + (n * i)] * x[j];
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    tFin = MPI_Wtime();

    // Recogemos los escalares de la multiplicación en un vector
    // Se hace en el mismo orden que el scatter
    MPI_Gather(&subFinal[0], nfilas, MPI_INT, y, nfilas, MPI_INT, 0, MPI_COMM_WORLD);

    MPI_Finalize();

    if (rank == 0) {
        unsigned int errores = 0;

        // Comprobar si hay diferencia entre el resultado secuencial y paralelo
        if (!modoGraficas)
            cout << "El resultado obtenido y el esperado son: " << endl;
        
        for (unsigned int i = 0 ; i < n ; i++) {
            if (!modoGraficas)
                cout << "\t" << y[i] << "\t|\t" << comprueba[i] << endl;
            
            if (comprueba[i] != y[i]) {
                errores++;
            }
        }

        delete [] y;
        delete [] comprueba;
        delete [] A[0];

        if (errores) {
            cout << "Hubo " << errores << " errores." << endl;
        } else {
            if (modoGraficas) {
                // Tiempo del algoritmo secuencial
                cout << tFinSec - tInicioSec << " ";
                // Tiempo del algoritmo paralelo
                cout << tFin - tInicio << " ";
                // Ganancia secuencial/paralelo
                cout << (tFinSec - tInicioSec) / (tFin - tInicio) << " ";
            } else {
                cout << "No hubo errores" << endl;
                cout << "El tiempo tardado para el algoritmo secuencial ha sido " 
                     << tFinSec - tInicioSec << " segundos" << endl;
                cout << "El tiempo tardado para el algoritmo paralelo ha sido " 
                     << tFin - tInicio << " segundos" << endl;
                cout << "La ganancia es: " << (tFinSec - tInicioSec) / (tFin - tInicio) << endl;
            }
        }
    }

    delete [] x;
    delete [] A;
    delete [] miBloque;*/

    return 0;
}