#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <mqueue.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>


#define TAMMSG 8192

int pel_abierto;

typedef struct{
    pthread_mutex_t clienteEsperando;
    pthread_mutex_t peluqueros[3];
    pthread_mutex_t atencionMutex;
    sem_t *pedido1;
    sem_t *pedido2;
    sem_t *atencion;
    sem_t *cobrar;
    sem_t *clientePagando;
    sem_t *cantClientes;
    sem_t *cantCaja;
    sem_t *obtenerID;
    
    int dinero;
    int open;
    int pedido;
    int idCliente;
    int idPeluquero;
    int estadoPeluqueros[3];
    int clientesAtendidos;
    int clientesSeFueron;
    int clientesSiendoAtendidos;
    
}Peluqueria;

void *Peluqueros(void *args);
void *Clientes(void *args);
void *peluquerosLibres(void *A);
void *Atender(void *A);
void *readFifo(void *A);
void *readQueue(void *A);
void setMutexes(Peluqueria *A);
void deleteSemaphoreUnlink();
void ultimoCliente();
void datos(Peluqueria *A);



int  Calle(int memoria);
int  Peluquero(int memoria);
int  createSemaphore(Peluqueria *A);
int  initVariable(Peluqueria *A);
int  deleteSemaphore(Peluqueria *A);
int deleteMutex(Peluqueria *A);
int deleteQueue();
int  Menu();

int main()
{
    shm_unlink("/MCompartida");
    int memoria=0;
    int error=0;
    int opcion=1;
    int pid, pid1;
    opcion=Menu();

    if(opcion==1){
        Peluqueria *p;
        memoria = shm_open("/MCompartida", O_CREAT | O_RDWR, 0660);
        if (memoria<0){
            perror("shm_open()");
            error=-1;
        }
        if(!error){
            error=ftruncate(memoria,sizeof(Peluqueria));
            if(error)
                perror("ftruncate()");
        }
        p=mmap(NULL, sizeof(Peluqueria), PROT_READ | PROT_WRITE, MAP_SHARED, memoria, 0);
        //Inicializo el valor de las variables, creo los semaforos y los mutex
        initVariable(p);
        pid=fork();
        if(pid == 0){
            Peluquero(memoria);
        }
        else if(pid > 0)
        {
            pid1=fork();
            if(pid1 == 0){
                Calle(memoria);
            }
            //Proceso Encargado
            else if(pid1 > 0){
                pthread_t pLibres;
                pthread_create(&pLibres,NULL,peluquerosLibres,p);
                pthread_t atender;
                pthread_create(&atender,NULL,Atender,p);
                pthread_t leerFIFO;
                pthread_create(&leerFIFO,NULL,readFifo,p);
                pthread_t leerCola;
                pthread_create(&leerCola,NULL,readQueue,p);

                pthread_join(pLibres,NULL);
                kill(pid1,SIGKILL);
                kill(pid ,SIGKILL);

                datos(p);
                deleteSemaphore(p);
                deleteMutex(p);
                munmap((void*)(p),2*sizeof(Peluqueria));
                exit(0);
            }
        }
    }
    else{
        printf("Fin del Juego\n");
    }
}
/*Funcion que se encarga de mostrar al final del juego, el resultado del mismo*/
void datos(Peluqueria *A){
    system("clear");
    printf("\t*******************************************************\n");
    printf("\t Datos del Juego:\n");
    printf("\t*******************************************************\n");
    printf("\t\tClientes Atendidos:       %d                   \n",A->clientesAtendidos);
    printf("\t\tClientes que se Fueron:   %d                   \n",A->clientesSeFueron);
    printf("\t\tDinero Recaudado:       $ %d                   \n",A->dinero);
    printf("\t*******************************************************\n");
}

int Menu(){
    int opcion=0;
    system("clear");
    do{
        printf("*********************************************************************************\n");
        printf("\t\t\tJuego de la Peluqueria\n\n");
        printf("\t\t 1- Start Game \n");
        printf("\t\t 2- Exit Game \n");
        printf("*********************************************************************************\n");
        printf("\t\t*******************Select Option******************* \n");
        printf ("\t\t                  : ");
        

        scanf("%d",&opcion);
        if (opcion==1 || opcion==2){
            system("clear");
            return opcion;
        }
        else{
            printf("Invalid Option\n");
        }
        
    } while (opcion!=1 || opcion!=2);
    return 0;
}

void *readFifo(void *args){
    Peluqueria *p = (Peluqueria*) args;
    int fifoIn = 0;
    int error = 0;
    char dinero[4];
    char *endptr;

    fifoIn = open("/tmp/dinero", O_RDONLY);
    if (fifoIn < 0){
        error = fifoIn;
        perror("fifo open");
    }
    while(p->open==1){
        error = read(fifoIn, dinero,4);
        if (error < 0){
            perror("fifo read");
        }
        printf("Cobre: $ %ld\n",strtol(dinero,&endptr,10));
        p->dinero+=strtol(dinero,&endptr,10);
    }

}

void *readQueue(void *args){
    Peluqueria *p = (Peluqueria*) args;
    mqd_t peluqueroLibre;
    char libre[TAMMSG];
    char *endptr;
    int error = 0;

    peluqueroLibre = mq_open("/peluqueroLibre",O_RDWR | O_CREAT,0777,NULL);
    if (peluqueroLibre==-1){
        perror("mq_open()");
    }
    while (p->open==1)
    {
        error=mq_receive(peluqueroLibre,libre,TAMMSG,NULL);
        if(error==-1){
            perror("mq_receive");
        }
        p->estadoPeluqueros[strtol(libre,&endptr,10)-1]=1;
        sleep(1);
    }

}

/*Hilo que espera la orden del usuario*/
void *Atender(void *args){
    Peluqueria *A=(Peluqueria*) args;
    int opc;
    int cantClientes=0;
    int cantClienteCaja;
    while(A->open==1){
        /*Espero la accion del usuario*/
        scanf("%d",&opc);
        sem_getvalue(A->cantClientes,&cantClientes);
        sem_getvalue(A->cantCaja,&cantClienteCaja);
        /*Si la peluqueria sigue abierta*/
        if (A->open!=0){
            if (opc<=3 && opc>=1){
                /*1= Libre 0= Ocupado*/
                if (A->estadoPeluqueros[opc-1]==0){
                    printf("Peluquero Ocupado\n");
                }
                else if((cantClientes>0 && cantClientes>cantClienteCaja) && (cantClientes-cantClienteCaja)>A->clientesSiendoAtendidos){
                    A->clientesSiendoAtendidos+=1;                 
                    printf("Peluquero %d enviado\n",opc);
                    pthread_mutex_unlock(&A->peluqueros[opc-1]);
                }
                else{
                    printf("No hay Clientes para Atender\n");
                }
            }
            //Si Opcion == 4 le cobro al cliente
            else if (opc==4){
                sem_getvalue(A->cantCaja,&cantClienteCaja);
                //Si hay un cliente en la caja esperando para pagar
                if (cantClienteCaja>0){
                    sem_post(A->clientePagando);
                    printf("Cobrandole al Cliente\n");
                    sleep(1);
                    printf("Se le cobro al Cliente\n");
                    sem_wait(A->cobrar);
                    sem_wait(A->cantCaja);
                    sem_wait(A->cantClientes);
                    A->clientesAtendidos+=1;
                }
                else{
                    printf("No hay Clientes a los Cuales Cobrar\n");
                }
            }
            else{
                printf("No existe ese Peluquero\n");
            }
            
            if(A->open==0){
                return 0;
            }
        }
    }
    return 0;
}
void *peluquerosLibres(void *args){
    Peluqueria *A=(Peluqueria*)args;
    int cantClientes;
    int cantClientesCaja;
    while (A->open==1){
        sem_getvalue(A->cantClientes,&cantClientes);
        sem_getvalue(A->cantCaja,&cantClientesCaja);
        printf("Cantidad de Clientes:%d\n",cantClientes);
        printf("Cantidad de Clientes siendo Atendidos:%d\n",A->clientesSiendoAtendidos);
        printf("Cantidad de Clientes Esperando para Pagar:%d\n",cantClientesCaja);
        printf("***************************************************\n");
        printf("Peluqueros Libres:\n");
        if (A->estadoPeluqueros[0]==1){
            printf("1-Peluquero 1\n");
        }
        if (A->estadoPeluqueros[1]==1){
            printf("2-Peluquero 2\n");
        }
        if (A->estadoPeluqueros[2]==1){
            printf("3-Peluquero 3\n");
        }
        //Si hay al menos un cliente en la caja, la opcion pasa a estar disponible y la muestro
        sem_getvalue(A->cantCaja,&cantClientesCaja);
        if (cantClientesCaja>0){
            printf("4-Cobrar a Cliente en Caja\n");
        }
        printf("***************************************************\n");
        sleep(2);
    }
    return 0;
}

//-----------------------------------------------------------------------------------------------------------------------
int Peluquero(int memoria){
    Peluqueria *p;
    p=mmap(NULL, sizeof(Peluqueria), PROT_READ | PROT_WRITE, MAP_SHARED, memoria, 0);
    pthread_t peluqueros[3];
    for(int i = 0; i < 3; i++){   
        p->idPeluquero=i+1;
        pthread_create(&peluqueros[i],NULL,(void*)Peluqueros,p);
        usleep(5000);
    }
    for(int i = 0; i < 3; i++){
        pthread_join(peluqueros[i],NULL);
    }
    munmap((void*)(p),2*sizeof(Peluqueria));
    return 1;
}
int Calle(int memoria){
    Peluqueria *p;
    signal(SIGALRM,ultimoCliente);
    alarm(15);

    p=mmap(NULL, sizeof(Peluqueria), PROT_READ | PROT_WRITE, MAP_SHARED, memoria, 0);
    srand(time(NULL));

    pthread_t clientes[100];
    int cantClientes;
    int cantCaja;
    for(int i = 0; i < 100; i++){
        if(pel_abierto){
            p->idCliente=i+1;
            pthread_create(&clientes[i],NULL,(void*)Clientes,p);
            sleep(rand()%5+1);
        }
        else{
            break;
        }
    }
    sem_getvalue(p->cantClientes,&cantClientes);
    sem_getvalue(p->cantCaja,&cantCaja);
    /*Mientras haya clientes en la peluqueria o en caja espero a que sean atendidos*/
    while (cantClientes>0 || cantCaja>0)
    {
        sem_getvalue(p->cantClientes,&cantClientes);
        sem_getvalue(p->cantCaja,&cantCaja);
        usleep(10000);
    }
    p->open=-1;

    munmap((void*)(p),2*sizeof(Peluqueria));
    return 1;
}
void *Peluqueros(void *args){
    Peluqueria *p=(Peluqueria*) args;
    sem_wait(p->obtenerID);
    int id=p->idPeluquero;
    sem_post(p->obtenerID);

    mqd_t notificarLibre;
    notificarLibre = mq_open("/peluqueroLibre",O_RDWR | O_CREAT,0777,NULL);

    char idPelLibre[2];
    int enviado;
    int pedido;
    char *pedidos[10]={" ","Barba","Corte de Pelo","Corte y Barba","Tintura","Peinado","","","","Peinado y Tintura"};
    //Espero a cliente
    while((p->pedido)!=0){
        if(p->pedido==0 && p->idCliente==-1){
            pthread_mutex_unlock(&p->peluqueros[id-1]);
        }
        pthread_mutex_lock(&p->peluqueros[id-1]);
        p->estadoPeluqueros[id-1]=0;

        pthread_mutex_unlock(&p->clienteEsperando);

        sem_wait(p->pedido2);

        pedido=p->pedido;

        sem_post(p->pedido1);

        if(p->idCliente!=-1){
            printf("************************************\n");
            printf("Cliente %d\n",p->idCliente);
            printf("Pedido:%d\n",pedido);
            printf("%s\n",pedidos[pedido]);
            printf("Peluquero:%d\n",id);
            
            printf("Trabajando...\n");
            printf("************************************\n");
        }

        /*Manipulo el tiempo para que se me puedan dar los diferentes casos*/
        /*(mas de 5 clientes y cliente se va por no recibir atencion)*/
        usleep(pedido*450000);

        pthread_mutex_unlock(&p->atencionMutex);
        p->clientesSiendoAtendidos-=1;

        sem_post(p->cantCaja);

        printf("Cliente espera para Pagar\n");

        if(p->idCliente!=-1){
            printf("*****************************************\n");
            printf("\tCliente Atendido por el Peluquero %d\n",id);
            printf("*****************************************\n");
        }
        /*Guardo el ID del peluquero que se libero*/
        snprintf(idPelLibre,2,"%1d",id);
        enviado=mq_send(notificarLibre,idPelLibre,2,0);
        if (enviado==-1){
            perror("mq_send()");
        }
    }
    return 0;
}

void *Clientes(void *args){
    Peluqueria *c=(Peluqueria*)args;
    int idCliente=c->idCliente;

    #pragma region timespec
    struct timespec tiempo;
    clock_gettime(CLOCK_REALTIME,&tiempo);
    
    //Corte de pelo: 2 Segundos
    tiempo.tv_sec  += 5;
    tiempo.tv_nsec += 0;
    #pragma endregion

    int cantidad;
    int pedido;

    char dinero[4];
    int fifoOut = 0;

    fifoOut = open("/tmp/dinero", O_WRONLY);
    sem_getvalue(c->cantClientes,&cantidad);

    if(cantidad<5){
        sem_post(c->cantClientes);
        printf("\tLlegó un Nuevo Cliente\n");
        if(pthread_mutex_timedlock(&c->clienteEsperando,&tiempo)==0){

            sem_wait(c->pedido1);

            c->pedido=rand()%10+1;
            c->idCliente=idCliente;

            if(c->pedido>5 && c->pedido<8){
                c->pedido=rand()%5+1;
            }

            if(c->pedido>7 && c->pedido<11){
                c->pedido=9;
            }

            if(c->idCliente==-1){
                c->pedido=0;
            }

            pedido=c->pedido;

            sem_post(c->pedido2);

            pthread_mutex_lock(&c->atencionMutex);

            sem_wait(c->clientePagando);
                
            snprintf(dinero,4,"%3d",pedido*10);
            write(fifoOut,dinero,4);

            sem_post(c->cobrar);
        }
        else{
            printf("************************************\n");
            printf("\tMe voy a otra peluqueria porque no me atienden\n");
            printf("\tSe fue un Cliente\n");
            printf("-***********************************\n");
            sem_wait(c->cantClientes);
            c->clientesSeFueron+=1;
        }
    }
        /*Si hay más de 4 clientes en el local, el cliente se va*/
    else{
        printf("****************************************\n");
        printf("Me voy porque hay mas de 4 clientes ya en el local\n");
        printf("****************************************\n");
        c->clientesSeFueron+=1;
    }
    return 0;
}
//-----------------------------------------------------------------------------------------------------------------------
/*Funcion que se ejecuta al recibir la signal SIGARLM y hace que calle no envie mas clientes y luego se cierre la peluqueria*/
void ultimoCliente(int sig_num){
    printf("**********LA PELUQUERIA ESTÁ POR CERRAR**********\n");
    printf("SOLAMENTE SE VAN A ATENDER LOS CLIENTES QUE YA ESTAN EN EL NEGOCIO\n");
    pel_abierto=0;
}

int deleteMutex(Peluqueria *p){
    int error = 0;
    error = pthread_mutex_destroy(&p->peluqueros[0]);
    if (error==0)
        error = pthread_mutex_destroy(&p->peluqueros[1]);
    else if(error==0)
        error = pthread_mutex_destroy(&p->peluqueros[2]);
    else if(error==0)
        error = pthread_mutex_destroy(&p->atencionMutex);
    else if(error==0)
        error = pthread_mutex_destroy(&p->atencionMutex);
    else if(error==0)
        error = pthread_mutex_destroy(&p->clienteEsperando);
    return error;
}

int deleteQueue(){
    int error=0;
    error=mq_unlink("/peluqueroLibre");
    if (error==-1){
        perror("sem_unlink()");
    }
}

#pragma region Iniciar Variables, Semaforos y Mutex
/*Funcion que se encarga de iniciar los valores de las variables compartidas*/
int initVariable(Peluqueria *A){
    /*elimino cualquier semaforo que haya quedado de alguna ejecucion previa*/
    int error=0;
    deleteSemaphoreUnlink();
    A->pedido=1;
    pel_abierto=1;
    A->open=1;
    A->estadoPeluqueros[0]=1;
    A->estadoPeluqueros[1]=1;
    A->estadoPeluqueros[2]=1;
    A->clientesAtendidos=0;
    A->clientesSeFueron=0;
    A->dinero=0;
    A->clientesSiendoAtendidos=0;

    error = mkfifo("/tmp/dinero",0777);
    if (error<0){
        perror("mkfifo");
    }

    createSemaphore(A);
    setMutexes(A);
    return 0;
}
/*Funcion que inicia los mutexes*/
void setMutexes(Peluqueria *A){
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    /*Inicializo los Mutex con ese Attribute*/
    pthread_mutex_init(&A->peluqueros[0],&attr);
    pthread_mutex_init(&A->peluqueros[1],&attr);
    pthread_mutex_init(&A->peluqueros[2],&attr);
    pthread_mutex_init(&A->clienteEsperando,&attr);
    /*Los inicializo lockeados*/
    pthread_mutex_lock(&A->peluqueros[0]);
    pthread_mutex_lock(&A->peluqueros[1]);
    pthread_mutex_lock(&A->peluqueros[2]);
    pthread_mutex_lock(&A->clienteEsperando);
    
    pthread_mutex_init(&A->atencionMutex,&attr);
    pthread_mutex_lock(&A->atencionMutex);
}
/*Funciones utilizadas para crear y borrar los semaforos*/
#pragma region Crear y Borrar Semaforos

/*Funcion para Iniciar todos los semaforos*/
int createSemaphore(Peluqueria *pel){
    int error=0;
    /*Semaforo Atencion*/
    pel->atencion=sem_open("/semAtencion",O_CREAT,O_RDWR,0);
    if (pel->atencion==SEM_FAILED){
        perror("sem_open()");
        error=-1;
    }
    /*Semaforo Pedido 1*/
    if (error!=-1){
        pel->pedido1=sem_open("/semPedido1",O_CREAT,O_RDWR,1);
    }
    if(pel->pedido1==SEM_FAILED){
        perror("sem_open()");
        error=-1;
    }
    /*Semaforo Pedido 2*/
    if(error!=-1){
        pel->pedido2=sem_open("/semPedido2",O_CREAT,O_RDWR,0);
    }
    if(pel->pedido2==SEM_FAILED){
        perror("sem_open()");
        error=-1;
    }
    /*Semaforo ClientePagando*/
    if(error!=-1){
        //pel->clientePagando=sem_open("/semClientePagando",O_CREAT,O_RDWR,1);
        pel->clientePagando=sem_open("/semClientePagando",O_CREAT,O_RDWR,0);
    }
    if(pel->clientePagando==SEM_FAILED){
        perror("sem_open()");
        error=-1;
    }
    /*Semaforo Cobrar*/
    if(error!=-1){
        pel->cobrar=sem_open("/semCobrar",O_CREAT,O_RDWR,0);
    }
    if(pel->cobrar==SEM_FAILED){
        perror("sem_open()");
        error=-1;
    }
    /*Semaforo Cantidad Clientes*/
    if(error!=-1){
        pel->cantClientes=sem_open("/semCantClientes",O_CREAT,O_RDWR,0);
    }
    if(pel->cantClientes==SEM_FAILED){
        perror("sem_open()");
        error=-1;
    }
    /*Semaforo Obtener ID*/
    if(error!=-1){
        pel->obtenerID=sem_open("/semObtenerID",O_CREAT,O_RDWR,1);
    }
    if(pel->obtenerID==SEM_FAILED){
        perror("sem_open()");
        error=-1;
    }
    /*Semaforo Cantidad de Clientes en Caja*/
    if(error!=-1){
        pel->cantCaja=sem_open("/semCantCaja",O_CREAT,O_RDWR,0);
    }
    if(pel->cantCaja==SEM_FAILED){
        perror("sem_open()");
        error=-1;
    }
    return error;
}

/*Funcion para Borrar todos los Semaforos*/
int deleteSemaphore(Peluqueria *A){
    int error=0;
    /*Close Cantidad Clientes*/
    error=sem_close(A->cantClientes);
    if(error){
        perror("sem_close()");
    }
    else{
        //printf("Semaforo Cerrado\n");
    }
    if(!error){
        error=sem_unlink("/semCantClientes");
        if (error){
            perror("sem_unlink()");
        }
        else{
            //printf("Semaforo Borrado\n");
        }
    }
    /*Close Atencion*/
    error=sem_close(A->atencion);
    if(error){
        perror("sem_close()");
    }
    else{
        //printf("Semaforo Cerrado\n");
    }
    if(!error){
        error=sem_unlink("/semAtencion");
        if (error){
            perror("sem_unlink()");
        }
        else{
            //printf("Semaforo Borrado\n");
        }
    }
    if(!error){
        error=sem_unlink("/semClienteEsperando");
        if (error){
            perror("sem_unlink()");
        }
        else{
            //printf("Semaforo Borrado\n");
        }
    }
    /*Close Cliente Pagando*/
    error=sem_close(A->clientePagando);
    if(error){
        perror("sem_close()");
    }
    else{
        //printf("Semaforo Cerrado\n");
    }
    if(!error){
        error=sem_unlink("/semClientePagando");
        if (error){
            perror("sem_unlink()");
        }
        else{
            //printf("Semaforo Borrado\n");
        }
    }
    /*Close Cobrar*/
    error=sem_close(A->cobrar);
    if(error){
        perror("sem_close()");
    }
    else{
        //printf("Semaforo Cerrado\n");
    }
    if(!error){
        error=sem_unlink("/semCobrar");
        if (error){
            perror("sem_unlink()");
        }
        else{
            //printf("Semaforo Borrado\n");
        }
    }
    /*Close Pedido 1*/
    error=sem_close(A->pedido1);
    if(error){
        perror("sem_close()");
    }
    else{
        //printf("Semaforo Cerrado\n");
    }
    if(!error){
        error=sem_unlink("/semPedido1");
        if (error){
            perror("sem_unlink()");
        }
        else{
            //printf("Semaforo Borrado\n");
        }
    }
    /*Close Pedido 2*/
    error=sem_close(A->pedido2);
    if(error){
        perror("sem_close()");
    }
    else{
        //printf("Semaforo Cerrado\n");
    }
    if(!error){
        error=sem_unlink("/semPedido2");
        if (error){
            perror("sem_unlink()");
        }
        else{
            //printf("Semaforo Borrado\n");
        }
    }
    /*Close Obtener ID*/
    error=sem_close(A->obtenerID);
    if(error){
        perror("sem_close()");
    }
    else{
        //printf("Semaforo Cerrado\n");
    }
    if(!error){
        error=sem_unlink("/semObtenerID");
        if(error){
            perror("sem_unlink()");
        }
        else{
            //printf("Semaforo Borrado\n");
        }   
    }
    if(!error){
        error=sem_unlink("/semCantCaja");
        if(error){
            perror("sem_unlink()");
        }
        else{
            //printf("Semaforo Borrado\n");
        }   
    }

    return error;
}
/*Funcion que se utiliza para borrar semaforos que hayan quedado de erroneas previas*/
void deleteSemaphoreUnlink(){
    sem_unlink("/semAtencion");
    sem_unlink("/semCantClientes");
    sem_unlink("/semClienteEsperando");
    sem_unlink("/semClientePagando");
    sem_unlink("/semCobrar");
    sem_unlink("/semPedido1");
    sem_unlink("/semPedido2");
    sem_unlink("/semObtenerID");
    sem_unlink("/semCantCaja");
}
#pragma endregion
#pragma endregion