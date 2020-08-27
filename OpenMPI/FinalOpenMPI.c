/* FINAL Programacion Concurrente
Profesor : Ing. Maximiliano Eschoyez
Alumno : Figueroa Javier
Tema: Final OpenMPI-La primer versión del programa debe ejecutar cada método de integración en un hilo diferente para
OpenMP o en un proceso para MPI.
*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <mpi.h>
#include <omp.h>

double trapezoidalRule(double x1, double x2, int n);
double simpsonRule (double x1,double x2,int n);
double rectangleRule(double x1, double x2, int n);
double middlepointRule(double x1, double x2, int n);
double f (double x);

void main(int argc,char *argv[]) {
   
    MPI_Init(&argc, &argv);
    //MPI_Init(NULL, NULL);
    int rank;
    int size = 4;
    int a = 2;
    int b = 10;
    int amountIterations=1000;
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&size);
    printf("\n-------------------------------------------");
    printf("\n***************Final OPENMPI***************");
    printf("\n-------------------------------------------");
    
   
    if(rank == 0)
    {
        printf("\n1- Rectangule Rule : %f\n ",rectangleRule(a,b,amountIterations));

    }
    if(rank ==1)
    {
        printf("\n2- Middlepoint Rule : %f\n ",middlepointRule(a,b,amountIterations));
    }
    if (rank == 2)
    {
        printf("\n3- Trapezoide Rule : %f\n ",trapezoidalRule(a,b,amountIterations));
    }
    if(rank == 3)
    {
        printf("\n4- Simpson 1/3 Rule : %f\n ",simpsonRule(a,b,amountIterations));
    }
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

}

double f(double x)
{
    return x*x;
}

/*Metodos de integracion*/
double trapezoidalRule (double x1,double x2,int n){
    double tm=MPI_Wtime();
    int i;
    double ttm;
    double dx = ((x2-x1) / n );
    double result = 0;
    for (i = 0; i < n; i++)
    {
        result += f( x1 + i * dx);
    }
    result = (( result +(f(x1)+f(x2)/2))*dx);
    ttm =MPI_Wtime() - tm;
    printf("\n3- Trapezoidal Rule - Execution time : %f seconds\n",ttm);
    return result;

    
}
double simpsonRule(double x1,double x2,int n){
    double tm = MPI_Wtime();
    int i;
    double h = ((x2-x1)/n);
    double ttm;
    double suma=0;
    double aux=0;
    double xi=0;
    for(i=0;i<n;i++){
        xi = (x1 * i * h);
        if(i == 0 || i == n ){
            aux = f(xi) ;
        }
        else if(i%2 != 0)
        {
            aux = 4 * f(xi);   
        }
        else if (i%2 == 0)
        {
            aux = 2 * f(xi);
        }
        suma += aux;
    }
    suma = ((suma * h)/ 3);
    ttm =MPI_Wtime() - tm;
    printf("\n4- Simpson Rule - Execution time : %f seconds\n",ttm);
    return suma;
}
double rectangleRule(double x1, double x2, int n){
    double tm = MPI_Wtime();
    int i;
    double ttm;
    double dx = ((x2 -x1)/ n); 
    int result= 0;

    for(i=0; i<n; i++){
        result += f(x1 + i * dx);
    }

    result *= dx;
     ttm = MPI_Wtime() - tm;
    printf("\n1- Rectangule Rule - Execution time : %f seconds\n",ttm);

    return result;
}

double middlepointRule(double x1, double x2, int n){
    double tm = MPI_Wtime();
    int i;
    double ttm;
    double suma = 0;
    double dx = ((x2-x1)/n);
    for ( i = 0; i < n; i++)
    {
        suma +=  f(((x1+ dx * i) + (x1+dx * (i+1)))/2)*dx;

    }
    ttm = MPI_Wtime() - tm;
    printf("\n2- Middlepoint Rule - Execution time : %f seconds\n",ttm);
    return suma;
    
}

