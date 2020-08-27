/* FINAL Programacion Concurrente
Profesor : Ing. Maximiliano Eschoyez
Alumno : Figueroa Javier
Tema: Final Secuencial
*/
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

//Prototipo de funciones
double trapezoidalRule(double x1, double x2, int n);
double simpsonRule (double x1,double x2,int n);
double rectangleRule(double x1, double x2, int n);
double middlepointRule(double x1, double x2, int n);
double f (double x);

void main (){
    int a = 2;
    int b = 10;
    int amountIterations=1000;
    printf("--------------------------------------------");
    printf("\nPROGRAMA SECUENCIAL");
    printf("\n--------------------------------------------");
    printf("\n1- Regla del Rectangulo : %f\n ",rectangleRule(a,b,amountIterations));
    printf("\n2- Regla del Punto Medio : %f\n ",middlepointRule(a,b,amountIterations));
    printf("\n3- Regla del Trapecio : %f\n\n ",trapezoidalRule(a,b,amountIterations));
    printf("\n4- Regla de simpson 1/3 : %f\n ",simpsonRule(a,b,amountIterations));
}

double f(double x)
{
    return x*x;
}

/*Metodos de integracion*/
double trapezoidalRule (double x1,double x2,int n){
    int i;
    double dx = ((x2-x1) / n );
    double result = 0;
    for (i = 0; i < n; i++)
    {
        result += f( x1 + i * dx);
    }
    result = (( result +(f(x1)+f(x2)/2))*dx);
    return result;
    
}
double simpsonRule(double x1,double x2,int n){
    int i;
    double h = ((x2-x1)/n);
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
    return suma;
}
double rectangleRule(double x1, double x2, int n){
    int i;
    double dx = ((x2 -x1)/ n); 
    int result= 0;

    for(i=0; i<n; i++){
        result += f(x1 + i * dx);
    }

    result *= dx;
    return result;
}

double middlepointRule(double x1, double x2, int n){
    int i;
    double suma = 0;
    double dx = ((x2-x1)/n);
    for ( i = 0; i < n; i++)
    {
        suma +=  f(((x1+ dx * i) + (x1+dx * (i+1)))/2)*dx;

    }
    return suma;
    
}