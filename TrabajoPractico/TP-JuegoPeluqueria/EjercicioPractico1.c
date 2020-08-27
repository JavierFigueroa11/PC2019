/*EjercicioPractico1
Profesor : Ing. Maximiliano Eschoyez
Alumno : Figueroa Javier*/

#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int cerrar = 0, Game = 1, Money = 0;

typedef struct {
    sem_t *sem1;
    sem_t *sem2;
    int pedido;
    int id;
}SplitSemaphore;

typedef struct {
    int pago;
    int libre[3];
    SplitSemaphore sem_pedido;
    SplitSemaphore sem_pagando;
    SplitSemaphore sem_atencion;
    pthread_mutex_t mtx_peluqueros[3];
    sem_t *sem_pelu;
    sem_t *sem_espera;
    sem_t *sem_caja;
    
}Peluqueria;

typedef struct {
    int id_cliente;
    Peluqueria *pelu;
}Clientes;

typedef struct {
    int id_peluquero;
    Peluqueria *pelu;
}Peluqueros;

//iniciar y cerrar comunicacion
int initCom (Peluqueria *args);
void closeCom (Peluqueria *args);

void procesoEncargado (Peluqueria *args, int pid1, int pid2);
void procesoCalle (Peluqueria *args);
void procesoPeluqueros (Peluqueria *args);

void fin (int sig_num);

void *MenuJuego (void *);
void *HiloCliente (void *);
void *HiloPeluquero (void *);

void ingresarPedido (SplitSemaphore *sem, int pedido, int id);
void ingresarID (SplitSemaphore *sem, int id);
int sacarPedido (SplitSemaphore *sem, int *id);
int sacarID (SplitSemaphore *sem);


int main (int argc, char *argv[]) {
    int error = 0, pid1 = 0, pid2 = 0, memoria = 0;
    Peluqueria *peluqueria = NULL;

    srand(time(NULL));

    peluqueria = (Peluqueria *)(calloc(1, sizeof(Peluqueria)));

    //Aca inicializo la memoria compartida con toda la estructura para pasarsela a los procesos
    memoria = shm_open("/MCompartida", O_CREAT | O_RDWR, 0660);
    
    if(memoria < 0) {
        perror("shm_open()");
        error = -1;
    }

    if(error != -1) {
        error = ftruncate(memoria, sizeof(Peluqueria));
        if (error)
            perror("ftruncate()");
    }

    if(error != -1) {
        peluqueria = mmap(NULL, sizeof(Peluqueria), PROT_READ | PROT_WRITE, MAP_SHARED, memoria, 0);    
    }
    
    error = initCom(peluqueria);

    if(error == 0) {
        pid1 = fork();

        if (pid1 == 0) {
            procesoCalle(peluqueria);
        }
        else if (pid1 > 0) {
            pid2 = fork();

            if(pid2 == 0) {
                procesoPeluqueros(peluqueria);
            }
            else if (pid2 > 0) {
                procesoEncargado(peluqueria, pid1, pid2);
            }
            else {
                error = pid2;
            }
        }
    
        if (error) {
            perror(argv[0]);
        }
    }

    wait(NULL);
    wait(NULL);

    closeCom(peluqueria);

    //free(peluqueria);
    error = munmap((void*)(peluqueria), 2*sizeof(Peluqueria));
    error = shm_unlink("/MCompartida");

    return error;
}
void ingresarPedido (SplitSemaphore *sem, int pedido, int id) {
    sem_wait(sem->sem1);
    sem->pedido = pedido;
    sem->id = id;
    sem_post(sem->sem2);
}

int sacarPedido (SplitSemaphore *sem, int *id) {
    int pedido;
    sem_wait(sem->sem2);
    pedido = sem->pedido;
    *id = sem->id;
    sem_post(sem->sem1);
    return pedido;
}

void ingresarID (SplitSemaphore *sem, int id) {
    sem_wait(sem->sem1);
    sem->id = id;
    sem_post(sem->sem2);
}

int sacarID (SplitSemaphore *sem) {
    int id;
    sem_wait(sem->sem2);
    id = sem->id;
    sem_post(sem->sem1);
    return id;
}
void fin (int sig_num) {
    printf("\nUltimo cliente!\n");
    cerrar = 1;
}

int initCom (Peluqueria *args) {
    int error = 0, i;

    /*Inicializo todos los semaforos*/
    args->sem_pelu = sem_open("/semPelu", O_CREAT, O_RDWR, 0);

    if(args->sem_pelu == SEM_FAILED) 
    {
        perror("sem_open()");
        error = -1;
    }

    if(error != -1)
    {
        args->sem_espera = sem_open("/semEspera", O_CREAT, O_RDWR, 0);
    }

    if(args->sem_espera == SEM_FAILED) 
    {
        perror("sem_open()");
        error = -1;
    }

    if(error != -1) 
    {
        args->sem_pedido.sem1 = sem_open("/semPedido1", O_CREAT, O_RDWR, 1);
    }

    if(args->sem_pedido.sem1 == SEM_FAILED) 
    {
        perror("sem_open()");
        error = -1;
    }

    if(error != -1) 
    {
        args->sem_pedido.sem2 = sem_open("/semPedido2", O_CREAT, O_RDWR, 0);
    }

    if(args->sem_pedido.sem2 == SEM_FAILED) 
    {
        perror("sem_open()");
        error = -1;
    }

    if(error != -1) 
    {
        args->sem_atencion.sem1 = sem_open("/semAtencion1", O_CREAT, O_RDWR, 1);
    }

    if(args->sem_atencion.sem1 == SEM_FAILED)
    {
        perror("sem_open()");
        error = -1;
    }

    if(error != -1) 
    {
        args->sem_atencion.sem2 = sem_open("/semAtencion2", O_CREAT, O_RDWR, 0);
    }

    if(args->sem_atencion.sem2 == SEM_FAILED) 
    {
        perror("sem_open()");
        error = -1;
    }

    if(error != -1) 
    {
        args->sem_pagando.sem1 = sem_open("/semPagando1", O_CREAT, O_RDWR, 0);
    }

    if(args->sem_pagando.sem1 == SEM_FAILED) 
    {
        perror("sem_open()");
        error = -1;
    }

    if(error != -1) 
    {
        args->sem_pagando.sem2 = sem_open("/semPagando2", O_CREAT, O_RDWR, 0);
    }

    if(args->sem_pagando.sem2 == SEM_FAILED) 
    {
        perror("sem_open()");
        error = -1;
    }

    if(error != -1) 
    {
        args->sem_caja = sem_open("/semCaja", O_CREAT, O_RDWR, 0);
    }

    if(args->sem_caja == SEM_FAILED) 
    {
        perror("sem_open()");
        error = -1;
    }

    if (error != -1) 
    {
    /*Inicializo los mutexes de los peluqueros y les doy un estado inicial*/
        for(i = 0; i < 3; i++) 
        {
            pthread_mutex_init(&args->mtx_peluqueros[i], NULL);
            pthread_mutex_lock(&args->mtx_peluqueros[i]);
        }
    }
    
    return error;
}

void closeCom (Peluqueria *args) {
    int error = 0, i;

    error = sem_close(args->sem_pelu);
    error = sem_close(args->sem_espera);
    error = sem_close(args->sem_pedido.sem1);
    error = sem_close(args->sem_pedido.sem2);
    error = sem_close(args->sem_atencion.sem1);
    error = sem_close(args->sem_atencion.sem2);
    error = sem_close(args->sem_caja);
    error = sem_close(args->sem_pagando.sem1);
    error = sem_close(args->sem_pagando.sem2);

    error = sem_unlink("/semPelu");
    error = sem_unlink("/semEspera");
    error = sem_unlink("/semPedido1");
    error = sem_unlink("/semPedido2");
    error = sem_unlink("/semAtencion1");
    error = sem_unlink("/semAtencion2");
    error = sem_unlink("/semCaja");
    error = sem_unlink("/semPagando1");
    error = sem_unlink("/semPagando2");

    for(i = 0; i < 3; i++) {
        pthread_mutex_destroy(&args->mtx_peluqueros[i]);
    }
}


void *MenuJuego (void *args) {
    Peluqueria *menu = (Peluqueria *)(args);
    int clientesEsperando = 0, clientesParaPagar = 0, i;

    printf("\n\nPeluqueria\n");
    while(Game) {
        printf("\n*****************************************\n");
        sem_getvalue(menu->sem_pelu, &clientesEsperando);
        printf("\n-Clientes esperando ser atendidos: %d\n", clientesEsperando);
        printf("\n*****************************************\n");
        printf("\n-Peluqueros desocupados:\n");
        for(i = 0; i < 3; i++) {
            if(menu->libre[i]) {
                printf(" *Peluquero %d\n", i+1);
            }
        }
        printf("\n********************************************\n");
        sem_getvalue(menu->sem_caja, &clientesParaPagar);
        printf("\n*******************************************\n");
        printf("\nIngrese una opción: \n");
        printf("1. Asignar Peluquero 1 al próximo cliente\n");
        printf("2. Asignar Peluquero 2 al próximo cliente\n");
        printf("3. Asignar Peluquero 3 al próximo cliente\n");
        printf("4. Cobrar a cliente\n");
        printf("5. Salir del juego\n");
        printf("\n*****************************************\n\n");
        printf("\n-Clientes esperando para pagar: %d\n", clientesParaPagar);
        sleep(4);
    }

    pthread_exit(NULL);
}

void procesoCalle (Peluqueria *args) {
    signal(SIGALRM, fin);
    Clientes clientes[30];
    pthread_t hilosCliente[30];
    int i = 0, j, error, ultimo = 20;

    alarm(ultimo);
    
    while(!cerrar) {
        sleep(1+rand()%2);
        clientes[i].id_cliente = i;
        clientes[i].pelu = args;
        pthread_create(&hilosCliente[i], NULL, HiloCliente, &clientes[i]);
        i++;
    }

    clientes[i].id_cliente = i;
    clientes[i].pelu = args;
    pthread_create(&hilosCliente[i], NULL, HiloCliente, &clientes[i]);

    for(j = 0; j <= i; j++) {
        pthread_join(hilosCliente[j], NULL);
    }

    if(args != NULL) {
        error = munmap((void*)(args), 2*sizeof(Peluqueria));
    }
    printf("\n\nPROCESO CALLE TERMINA\n\n");
}

void procesoPeluqueros (Peluqueria *args) {
    pthread_t peluquero[3];
    Peluqueros peluqueros[3];
    int i, error;

    for(i = 0; i < 3; i++) {
        peluqueros[i].id_peluquero = i;
        args->libre[i] = 1;
        peluqueros[i].pelu = args;
        pthread_create(&peluquero[i], NULL, HiloPeluquero, &peluqueros[i]);
    }

    for(i = 0; i < 3; i++) {
        pthread_join(peluquero[i], NULL);
    }

    if(args != NULL) {
        error = munmap((void*)(args), 2*sizeof(Peluqueria));
    }
    printf("\n\nPROCESO PELUQUERO TERMINA\n\n");
}

void *HiloCliente (void *args) {
    Clientes *cliente = (Clientes *)(args);
    int tipoPedido = 2+rand()%(7-2);
    int res = 0, id_peluquero = 0, semaforo = 0;

    sem_post(cliente->pelu->sem_pelu);

    struct timespec espera;
    clock_gettime(CLOCK_REALTIME, &espera);
    
    espera.tv_sec += 20;
    
    res = sem_timedwait(cliente->pelu->sem_espera, &espera);
    

    if(res == 0) {
        //Ya no esta esperando porque lo atendieron
        sem_wait(cliente->pelu->sem_pelu);

        ingresarPedido(&cliente->pelu->sem_pedido, tipoPedido, cliente->id_cliente);
        
        id_peluquero = sacarID(&cliente->pelu->sem_atencion);
        pthread_mutex_lock(&cliente->pelu->mtx_peluqueros[id_peluquero]);
        //Esto tiene que ser con memoria compartida
        sem_post(cliente->pelu->sem_caja);

        sem_wait(cliente->pelu->sem_pagando.sem1);
        cliente->pelu->pago = tipoPedido*100;
        sem_post(cliente->pelu->sem_pagando.sem2);
        
        printf("El cliente %d paga y se va\n", cliente->id_cliente);
    }
    else {
        printf("El cliente %d se canso de esperar\n", cliente->id_cliente);
        sem_wait(cliente->pelu->sem_pelu);
    }
    pthread_exit(NULL);
}

void *HiloPeluquero (void *args) {
    Peluqueros *peluquero = (Peluqueros *)(args);
    int pedido = 0, pago = 0, id_cliente = 0, semaforo = 0;

    while(Game) {
        if(peluquero->pelu->libre[peluquero->id_peluquero] == 0) {

            sem_post(peluquero->pelu->sem_espera);
            
            pedido = sacarPedido(&peluquero->pelu->sem_pedido, &id_cliente);
            
            ingresarID(&peluquero->pelu->sem_atencion, peluquero->id_peluquero);
                
            sleep(pedido);


            pthread_mutex_unlock(&peluquero->pelu->mtx_peluqueros[peluquero->id_peluquero]);

            peluquero->pelu->libre[peluquero->id_peluquero] = 1;
            pthread_mutex_lock(&peluquero->pelu->mtx_peluqueros[peluquero->id_peluquero]);
            
        }
    }
    pthread_exit(NULL);
}

void procesoEncargado (Peluqueria *args, int pid1, int pid2) {
    int opcion = 0, cantidadClientes = 0, clientesCaja = 0;
    pthread_t menu;
    pthread_create(&menu, NULL, MenuJuego, args);

    do {
        fflush(stdin);
        do {
            scanf("%d", &opcion);
        } while(opcion < 1 || opcion > 5);

        if(opcion >= 1 && opcion <= 3) {
            sem_getvalue(args->sem_pelu, &cantidadClientes);
            if(cantidadClientes > 0) {
                if(args->libre[opcion-1]) {
                    printf("Peluquero %d asignado\n", opcion);
                    pthread_mutex_unlock(&args->mtx_peluqueros[opcion-1]);
                    args->libre[opcion-1] = 0;
                }
                else {
                    printf("El Peluquero %d está ocupado\n", opcion);    
                }
            }
            else {
                printf("Ya no hay clientes para atender\n");
            }
            
        }
        
        if(opcion == 4) {
            sem_getvalue(args->sem_caja, &clientesCaja);
            sem_post(args->sem_pagando.sem1);
            if(clientesCaja > 0) {
                sem_wait(args->sem_pagando.sem2);
                Money = args->pago;
                sem_post(args->sem_pagando.sem1);
                sem_wait(args->sem_caja);
            }
            else {
                printf("No hay clientes para cobrarles\n");
            }
        }

        if(opcion == 5) {
            printf("Juego terminado\n");
            Game = 0;
            kill(pid1, SIGTERM);
            kill(pid2, SIGTERM);
        }
    } while(opcion != 5);
}
