#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lcgrand.cpp" /* Generador de números aleatorios */

#define LIMITE_COLA 1000
#define MAX_SERVIDORES 100
#define OCUPADO 1
#define LIBRE 0

int sig_tipo_evento,                            /* 1. Llegada. 2+. Salidas */
    num_clientes_espera,                        /* Numero de clientes atendidos */
    num_esperas_requerido,                      /* Numero de clientes a atender */
    num_servidores,                             /**/
    num_eventos,                                /**/
    num_entra_cola,                             /* Numero de clientes en la cola */
    estado_servidores[MAX_SERVIDORES + 1];      /**/
float area_num_entra_cola,                      /**/
    area_estado_servidores[MAX_SERVIDORES + 1], /**/
    media_entre_llegadas,                       /* Lambda(λ) */
    media_atencion,                             /* Miu(μ) */
    tiempo_simulacion,                          /**/
    tiempo_llegada[LIMITE_COLA + 1],            /* Cola */
    tiempo_ultimo_evento,                       /**/
    tiempo_sig_evento[MAX_SERVIDORES + 2],      /**/
    total_de_esperas;                           /**/
FILE *parametros,                               /**/
    *resultados,                                /**/
    *tiempo_atencion,                           /**/
    *tiempo_entre_llegadas;                     /**/

void inicializar(void);
void controltiempo(void);
void llegada(void);
void salida(void);
void reportes(void);
void actualizar_estad_prom_tiempo(void);
float expon(float mean);

int main(void)
{
    /* Abre los archivos de entrada y salida */
    parametros = fopen("param.txt", "r");
    resultados = fopen("result.txt", "w");
    tiempo_atencion = fopen("atencion.txt", "w");
    tiempo_entre_llegadas = fopen("llegadas.txt", "w");

    inicializar();

    /* Escribe en el archivo de salida los encabezados del reporte y los parametros iniciales */
    fprintf(resultados, "Sistema de Colas Simple\n\n");
    fprintf(resultados, /* */
            "Tiempo promedio de llegada%11.3f minutos\n\n", media_entre_llegadas);
    fprintf(resultados, /* */
            "Tiempo promedio de atencion%16.3f minutos\n\n", media_atencion);
    fprintf(resultados, /* */
            "Numero de clientes%14d\n\n", num_esperas_requerido);
    fprintf(resultados, /* */
            "Numero de servidores%14d\n\n", num_servidores);

    /* Corre la simulacion mientras se hayan completado un número de esperas requeridas como parametro */
    while (num_clientes_espera < num_esperas_requerido)
    {
        /* Determina el siguiente evento */
        controltiempo();

        /* Actualiza los acumuladores estadisticos */
        actualizar_estad_prom_tiempo();

        /* Invoca la funcion del evento adecuado */
        switch (sig_tipo_evento)
        {
        case 1:
            llegada();
            break;
        default:
            salida();
            break;
        }
    }

    reportes();

    fclose(parametros);
    fclose(resultados);
    fclose(tiempo_atencion);
    fclose(tiempo_entre_llegadas);

    return 0;
}

void inicializar(void)
{
    /* Lee los parametros de entrada */
    fscanf(parametros,
           "%f %f %d %d",          /**/
           &media_entre_llegadas,  /**/
           &media_atencion,        /**/
           &num_esperas_requerido, /**/
           &num_servidores);

    if (num_servidores == 0)
    {
        fprintf(resultados, "\nNo hay servidores en el sistema!");
        exit(1);
    }

    num_eventos = num_servidores + 2;
    tiempo_simulacion = 0.0;

    /* Inicializa las variables de estado */
    for (int i = 0; i <= num_servidores; i++)
    {
        estado_servidores[i] = LIBRE;
    }
    num_entra_cola = 0;
    tiempo_ultimo_evento = 0.0;

    /* Inicializa los contadores estadisticos */
    num_clientes_espera = 0;
    total_de_esperas = 0.0;
    area_num_entra_cola = 0.0;

    for (int i = 0; i <= num_servidores; i++)
    {
        area_estado_servidores[i] = 0.0;
    }

    /* Inicializa la lista de eventos */
    tiempo_sig_evento[1] = tiempo_simulacion + expon(media_entre_llegadas);
    for (int i = 2; i < num_eventos; i++)
    {
        tiempo_sig_evento[i] = 1.0e+30;
    }
}

void controltiempo(void)
{
    float min_tiempo_sig_evento = 1.0e+29;

    sig_tipo_evento = 0;

    /*  Determina el tipo de evento del evento que debe ocurrir */
    for (int i = 1; i < num_eventos; i++)
    {
        if (tiempo_sig_evento[i] < min_tiempo_sig_evento)
        {
            min_tiempo_sig_evento = tiempo_sig_evento[i];
            sig_tipo_evento = i;
        }
    }

    if (sig_tipo_evento == 0)
    {
        /* La lista de eventos esta vacia, se detiene la simulacion */
        fprintf(resultados, "\nLa lista de eventos esta vacia %f", tiempo_simulacion);
        exit(1);
    }

    /* La lista de eventos no esta vacia, adelanta el reloj de la simulacion */
    tiempo_simulacion = min_tiempo_sig_evento;
}

void llegada(void)
{
    float espera;

    /* Programa la siguiente llegada y reporta el tiempo entre llegadas */
    tiempo_sig_evento[1] = tiempo_simulacion + expon(media_entre_llegadas);
    fprintf(tiempo_entre_llegadas, "%f\n", tiempo_sig_evento[1] - tiempo_simulacion);

    /* Revisa si hay servidores libres */
    bool todos_ocupados = true;
    for (int i = 1; i <= num_servidores; i++)
    {
        if (estado_servidores[i] == LIBRE)
        {
            espera = 0.0;
            total_de_esperas += espera;

            ++num_clientes_espera;
            estado_servidores[i] = OCUPADO;

            /* Programa una salida y reporta el tiempo de atencion del cliente */
            tiempo_sig_evento[i + 1] = tiempo_simulacion + expon(media_atencion);
            fprintf(tiempo_atencion, "%f\n", tiempo_sig_evento[i + 1] - tiempo_simulacion);
            todos_ocupados = false;
            break;
        }
    }

    if (todos_ocupados)
    {
        /* Servidor OCUPADO, aumenta el numero de clientes en cola */
        ++num_entra_cola;

        /* Verifica si hay condicion de desbordamiento */
        if (num_entra_cola > LIMITE_COLA)
        {
            fprintf(resultados, "\nDesbordamiento del arreglo tiempo_llegada a la hora");
            fprintf(resultados, "%f", tiempo_simulacion);
            exit(2);
        }

        /* Se almacena el tiempo de llegada del cliente en el fin de tiempo_llegada */
        tiempo_llegada[num_entra_cola] = tiempo_simulacion;
    }
}

void salida(void)
{
    float espera;

    if (num_entra_cola == 0)
    {
        /* Servidor LIBRE y no considera el evento de salida */
        estado_servidores[sig_tipo_evento - 1] = LIBRE;
        tiempo_sig_evento[sig_tipo_evento] = 1.0e+30;
    }
    else
    {
        /* Disminuye el numero de clientes en cola */
        --num_entra_cola;

        /* Calcula la espera en cola del cliente que esta siendo atendido y
        actualiza el acumulador de espera */
        espera = tiempo_simulacion - tiempo_llegada[1];
        total_de_esperas += espera;

        /* Incrementa el numero de clientes en espera, y programa la salida */
        ++num_clientes_espera;
        tiempo_sig_evento[sig_tipo_evento] = tiempo_simulacion + expon(media_atencion);

        /* Reporta el tiempo de atencion del cliente */
        fprintf(tiempo_atencion, "%f\n", tiempo_sig_evento[sig_tipo_evento] - tiempo_simulacion);

        /* Mueve cada cliente en la cola (si los hay) una posicion hacia adelante */
        for (int i = 1; i <= num_entra_cola; ++i)
            tiempo_llegada[i] = tiempo_llegada[i + 1];
    }
}

void reportes(void)
{
    /* Calcula y estima las medidas deseadas de desempeño */
    fprintf(resultados, "\n\nEspera promedio en la cola%11.3f minutos\n\n",
            total_de_esperas / num_clientes_espera);
    fprintf(resultados, "Numero promedio en cola%10.3f\n\n",
            area_num_entra_cola / tiempo_simulacion);

    for (int i = 1; i <= num_servidores; i++)
    {
        fprintf(resultados, "Uso del servidor %d: %15.3f\n\n", i,
                area_estado_servidores[i] / tiempo_simulacion);
    }

    fprintf(resultados, "Tiempo de terminacion de la simulacion%12.3f minutos", tiempo_simulacion);
}

void actualizar_estad_prom_tiempo(void)
{
    float time_since_last_event;

    /* Calcula el tiempo desde el ultimo evento, y actualiza el marcador del ultimo evento */
    time_since_last_event = tiempo_simulacion - tiempo_ultimo_evento;
    tiempo_ultimo_evento = tiempo_simulacion;

    /* Actualiza el area bajo la funcion de numero_en_cola */
    area_num_entra_cola += num_entra_cola * time_since_last_event;

    /* Actualiza el area bajo la funcion indicadora de servidor ocupado */
    for (int i = 1; i <= num_servidores; i++)
    {
        area_estado_servidores[i] += estado_servidores[i] * time_since_last_event;
    }
}

float expon(float media)
{
    /* Retorna una variable aleatoria exponencial con media "media" */
    return -media * log(lcgrand(1));
}
