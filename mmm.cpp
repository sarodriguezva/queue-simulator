/* Definiciones externas para el sistema de colas simple */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lcgrand.cpp" /* Encabezado para el generador de numeros aleatorios */

#define LIMITE_COLA 1000 /* Capacidad maxima de la cola */
#define MAX_SERVIDORES 1 /* Número de servidores en el sistema */
#define OCUPADO 1        /* Indicador de Servidor Ocupado */
#define LIBRE 0          /* Indicador de Servidor Libre */

int sig_tipo_evento, num_clientes_espera, num_esperas_requerido, num_servidores, num_eventos,
    num_entra_cola, estado_servidores[MAX_SERVIDORES + 1], servidor_llegada[MAX_SERVIDORES + 1];
float area_num_entra_cola, area_estado_servidores[MAX_SERVIDORES + 1], media_entre_llegadas, media_atencion,
    tiempo_simulacion, tiempo_llegada[LIMITE_COLA + 1], tiempo_ultimo_evento, tiempo_sig_evento[MAX_SERVIDORES + 2],
    total_de_esperas;
FILE *parametros, *resultados, *tiempo_atencion, *tiempo_entre_llegadas;

void inicializar(void);
void controltiempo(void);
void llegada(void);
void salida(void);
void reportes(void);
void actualizar_estad_prom_tiempo(void);
float expon(float mean);

int main(void) /* Funcion Principal */
{
    /* Abre los archivos de entrada y salida */

    parametros = fopen("param.txt", "r");
    resultados = fopen("result.txt", "w");
    tiempo_atencion = fopen("atencion.txt", "w");
    tiempo_entre_llegadas = fopen("llegadas.txt", "w");

    /* Escribe en el archivo de salida los encabezados del reporte y los parametros iniciales */

    fprintf(resultados, "Sistema de Colas Simple\n\n");
    fprintf(resultados, "Tiempo promedio de llegada%11.3f minutos\n\n",
            media_entre_llegadas);
    fprintf(resultados, "Tiempo promedio de atencion%16.3f minutos\n\n", media_atencion);
    fprintf(resultados, "Numero de clientes%14d\n\n", num_esperas_requerido);

    /* iInicializa la simulacion. */

    inicializar();

    /* Corre la simulacion mientras no se llegue al numero de clientes especificaco en el archivo de entrada*/

    while (num_clientes_espera < num_esperas_requerido)
    {

        /* Determina el siguiente evento */

        controltiempo();

        /* Actualiza los acumuladores estadisticos de tiempo promedio */

        actualizar_estad_prom_tiempo();

        /* Invoca la funcion del evento adecuado. */
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

    /* Invoca el generador de reportes y termina la simulacion. */

    reportes();

    fclose(parametros);
    fclose(resultados);
    fclose(tiempo_atencion);
    fclose(tiempo_entre_llegadas);

    return 0;
}

void inicializar(void) /* Funcion de inicializacion. */
{
    /* Lee los parametros de enrtrada. */
    fscanf(parametros, "%f %f %d %d", &media_entre_llegadas, &media_atencion,
           &num_esperas_requerido, &num_servidores);
    if (num_servidores == 0)
    {
        fprintf(resultados, "\nNo hay servidores en el sistema!");
        exit(1);
    }

    /* Especifica el numero de eventos para la funcion controltiempo. */
    num_eventos = num_servidores + 2;

    /* Inicializa el reloj de la simulacion. */
    tiempo_simulacion = 0.0;

    /* Inicializa las variables de estado */
    for (int i = 0; i <= num_servidores; i++)
    {
        estado_servidores[i] = LIBRE;
        servidor_llegada[i] = i;
    }
    num_entra_cola = 0;
    tiempo_ultimo_evento = 0.0;

    /* Inicializa los contadores estadisticos. */
    num_clientes_espera = 0;
    total_de_esperas = 0.0;
    area_num_entra_cola = 0.0;

    for (int i = 0; i <= num_servidores; i++)
    {
        area_estado_servidores[i] = 0.0;
    }

    /* Inicializa la lista de eventos. Ya que no hay clientes, el evento salida
       (terminacion del servicio) no se tiene en cuenta */
    tiempo_sig_evento[1] = tiempo_simulacion + expon(media_entre_llegadas);
    for (int i = 2; i < num_servidores + 2; i++)
    {
        tiempo_sig_evento[i] = 1.0e+30;
    }
}

void controltiempo(void) /* Funcion controltiempo */
{
    int i;
    float min_tiempo_sig_evento = 1.0e+29;

    sig_tipo_evento = 0;

    /*  Determina el tipo de evento del evento que debe ocurrir. */
    for (i = 1; i < num_eventos; i++)
    {
        if (tiempo_sig_evento[i] < min_tiempo_sig_evento)
        {
            min_tiempo_sig_evento = tiempo_sig_evento[i];
            sig_tipo_evento = i;
        }
    }

    /* Revisa si la lista de eventos esta vacia. */
    if (sig_tipo_evento == 0)
    {

        /* La lista de eventos esta vacia, se detiene la simulacion. */
        fprintf(resultados, "\nLa lista de eventos esta vacia %f", tiempo_simulacion);
        exit(1);
    }

    /* La lista de eventos no esta vacia, adelanta el reloj de la simulacion. */
    tiempo_simulacion = min_tiempo_sig_evento;
}

void llegada(void) /* Funcion de llegada */
{
    float espera;

    /* Programa la siguiente llegada. */
    tiempo_sig_evento[1] = tiempo_simulacion + expon(media_entre_llegadas);
    fprintf(tiempo_entre_llegadas, "%f\n", tiempo_sig_evento[1] - tiempo_simulacion);

    /* Revisa si el servidor esta OCUPADO. */
    bool all_busy = true;
    for (int i = 1; i <= num_servidores; i++)
    {
        if (estado_servidores[i] == LIBRE)
        {
            /*  El servidor esta LIBRE, por lo tanto el cliente que llega tiene tiempo de eespera=0
               (Las siguientes dos lineas del programa son para claridad, y no afectan
               el reultado de la simulacion ) */
            espera = 0.0;
            total_de_esperas += espera;

            /* Incrementa el numero de clientes en espera, y pasa el servidor a ocupado */
            ++num_clientes_espera;
            estado_servidores[i] = OCUPADO;

            /* Programa una salida ( servicio terminado ). */
            tiempo_sig_evento[i + 1] = tiempo_simulacion + expon(media_atencion);
            fprintf(tiempo_atencion, "%f\n", tiempo_sig_evento[i + 1] - tiempo_simulacion);
            all_busy = false;
            break;
        }
    }

    if (all_busy)
    {
        /* Servidor OCUPADO, aumenta el numero de clientes en cola */
        ++num_entra_cola;

        /* Verifica si hay condicion de desbordamiento */
        if (num_entra_cola > LIMITE_COLA)
        {
            /* Se ha desbordado la cola, detiene la simulacion */
            fprintf(resultados, "\nDesbordamiento del arreglo tiempo_llegada a la hora");
            fprintf(resultados, "%f", tiempo_simulacion);
            exit(2);
        }

        /* Todavia hay espacio en la cola, se almacena el tiempo de llegada del
            cliente en el ( nuevo ) fin de tiempo_llegada */
        tiempo_llegada[num_entra_cola] = tiempo_simulacion;
    }
}

void salida(void) /* Funcion de Salida. */
{
    float espera;

    /* Revisa si la cola esta vacia */
    if (num_entra_cola == 0)
    {
        /* La cola esta vacia, pasa el servidor a LIBRE y
        no considera el evento de salida*/
        estado_servidores[sig_tipo_evento - 1] = LIBRE;
        tiempo_sig_evento[sig_tipo_evento] = 1.0e+30;
    }

    else
    {

        /* La cola no esta vacia, disminuye el numero de clientes en cola. */
        --num_entra_cola;

        /*Calcula la espera del cliente que esta siendo atendido y
        actualiza el acumulador de espera */
        espera = tiempo_simulacion - tiempo_llegada[1];
        total_de_esperas += espera;

        /*Incrementa el numero de clientes en espera, y programa la salida. */
        ++num_clientes_espera;
        tiempo_sig_evento[sig_tipo_evento] = tiempo_simulacion + expon(media_atencion);
        fprintf(tiempo_atencion, "%f\n", tiempo_sig_evento[sig_tipo_evento] - tiempo_simulacion);

        /* Mueve cada cliente en la cola ( si los hay ) una posicion hacia adelante */
        for (int i = 1; i <= num_entra_cola; ++i)
            tiempo_llegada[i] = tiempo_llegada[i + 1];
    }
}

void reportes(void) /* Funcion generadora de reportes. */
{
    /* Calcula y estima los estimados de las medidas deseadas de desempe�o */
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

void actualizar_estad_prom_tiempo(void) /* Actualiza los acumuladores de
                                                       area para las estadisticas de tiempo promedio. */
{
    float time_since_last_event;

    /* Calcula el tiempo desde el ultimo evento, y actualiza el marcador
        del ultimo evento */
    time_since_last_event = tiempo_simulacion - tiempo_ultimo_evento;
    tiempo_ultimo_evento = tiempo_simulacion;

    /* Actualiza el area bajo la funcion de numero_en_cola */
    area_num_entra_cola += num_entra_cola * time_since_last_event;

    /* Actualiza el area bajo la funcion indicadora de servidor ocupado*/
    for (int i = 1; i <= num_servidores; i++)
    {
        area_estado_servidores[i] += estado_servidores[i] * time_since_last_event;
    }
}

float expon(float media) /* Funcion generadora de la exponencias */
{
    /* Retorna una variable aleatoria exponencial con media "media"*/

    return -media * log(lcgrand(1));
}
