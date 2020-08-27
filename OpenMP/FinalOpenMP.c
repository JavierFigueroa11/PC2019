/* FINAL Programacion Concurrente
Profesor : Ing. Maximiliano Eschoyez
Alumno : Figueroa Javier
Tema: Final OpenMP-La segunda versión debe dividir la tarea de integración de cada método en n hilos OpenMP o
procesos MPI y, además, hacer que se ejecuten los cuatro métodos de integración en paralelo. Es
decir, si n = 4 se tienen que ejecutar los 4 métodos con 4 hilos/procesos cada uno, totalizando 16
hilos/procesos en la CPU.
*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <omp.h>

//using namespace std;

double trapezoidalRule(double x1, double x2, int n,int nthreads);
double simpsonRule (double x1,double x2,int n,int nthreads);
double rectangleRule(double x1, double x2, int n,int nthreads);
double middlepointRule(double x1, double x2, int n,int nthreads);
double f (double x);

void main()
{
    
    int nthreads = 10;
    int np;
    int a = 2;
    int b = 10;
    int amountIterations=1000;
    omp_set_nested (1); // 1 - habilita el paralelismo anidado; 0: deshabilita el paralelismo anidado.
    printf("\n------------------------------------------------------------------");
    printf("\n*************************Final OPENMP*****************************");
    printf("\n------------------------------------------------------------------\n");
    

    //printf("\nIngresar numeros de Hilos : ");
    //scanf("%d",&nthreads)
    
    #pragma omp parallel default (none) shared (a,b , amountIterations, nthreads)
    //Especifica que el bloque estructurado asociado se ejecuta por solo uno de los hilos del equipo.
        #pragma omp single
        {
            #pragma omp task
            printf("Result about Trapezoide Rule : %f\n",trapezoidalRule(a,b,amountIterations,nthreads));
            #pragma omp task
            printf("Result about Simspson Rule : %f\n ",simpsonRule(a,b,amountIterations,nthreads));
            #pragma omp task
            printf("Result about Rectange Rule : %f\n ",rectangleRule(a,b,amountIterations,nthreads));
            #pragma omp task
            printf("Result about Middlepoint Rule : %f\n",middlepointRule(a,b,amountIterations,nthreads));
        }
}

double f (double x)
{
    return x*x;
}

/*Metodos de integracion*/
double trapezoidalRule (double x1,double x2,int n,int nthreads){
    int i;
    double tm = omp_get_wtime();
    double ttm;
    double dx = ((x2-x1) / n );
    double result = 0;
    omp_set_num_threads(nthreads);

    #pragma omp parallel for private (i) reduction (+:result)
    for (i = 0; i < n; i++)
    {
        result += f( x1 + i * dx);
    }
    ttm = omp_get_wtime()-tm;
    printf("\n------------------------------------------------------------------\n");
    printf("Execution time bout the Trapezoidal Rule : %f seconds\n ",ttm);
    result = (( result + (f(x1)+f(x2)/2)) *dx);
    return result;
    
}
double simpsonRule(double x1,double x2,int n,int nthreads){
    int i;
    double tm = omp_get_wtime();
    double ttm;
    double h = ((x2-x1)/n);
    double suma=0;
    double aux=0;
    double xi=0;
    omp_set_num_threads(nthreads);

    #pragma omp parallel for private (i) reduction (+:suma)
    for(i=0 ; i<n ; i++)
    {
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
    ttm = omp_get_wtime() - tm;
    printf("\n------------------------------------------------------------------\n");
    printf("Execution time about the Simpson Rule : %f seconds\n ",ttm);

    suma = ((suma * h)/ 3);
    return suma;
}
double rectangleRule(double x1, double x2, int n,int nthreads){
    int i;
    double ttm;
    double tm = omp_get_wtime();
    double dx = ((x2 -x1)/ n); 
    int result= 0;
    omp_set_num_threads(nthreads);

    #pragma omp parallel for private (i) reduction (+:result)
    for(i=0; i<n; i++)
    {
        result += f(x1 + i * dx);
    }
    ttm = omp_get_wtime() -tm;
    printf("\n------------------------------------------------------------------\n");
    printf("Execution time about the Rectangule Rule : %f seconds\n ",ttm);
    result *= dx;
    return result;
}

double middlepointRule(double x1, double x2, int n,int nthreads){
    int i;
    double ttm;
    double suma = 0;
    double tm= omp_get_wtime();
    double dx = ((x2-x1)/n);
    omp_set_num_threads(nthreads);
    #pragma omp parallel for private (i) reduction (+: suma)
    for ( i = 0; i < n; i++)
    {
        suma +=  f(((x1+ dx * i) + (x1+dx * (i+1)))/2)*dx;

    }
    ttm = omp_get_wtime()-tm;
    printf("\n------------------------------------------------------------------\n");
    printf("Execution time about the middlepoint Rule : %f seconds\n ",ttm);
    return suma;
    
}