#ifndef DEFAULTS_H
#define DEFAULTS_H

#define BUFFERSIZE                  1024

/**
  * @defgroup Configs Valores de Configuracion
  * @{
*/

#define CONFIG_LISENT_IFACE         "0.0.0.0" /*< Interfaz para escucar */
#define CONFIG_MAX_CLIENT           5       /*< Maximo de Conexiones Simultaneas */
#define CONFIG_DEFAULT_PORT         5000    /*< Puerto de trabajo */

#define CONFIG_DEFAULT_COUNT        5       /*< Numero de Mensajes a enviar */
#define CONFIG_DEFAULT_MSGSIZE      64      /*< Tamaño de Mensaje en KB  */

#define CONFIG_DEFAULT_VERBOSE      1       /*< Nivel de Verbosity  */
#define MAXBYTESREAD                1      /*< Cantidad de bits que serán recibidos */
#define BROADCAST_IFACE				"0.0.0.0"
#define BROADCAST_DEFAULT_PORT		4000
/** @}*/


/**
  * @defgroup Constante Constantes
  * @{
*/

#define MINPORT                     80    /*< Menor puerto a Utilizar */
#define MAXPORT                     USHRT_MAX /*< Mayor puerto a Utilizar */
/** @}*/

#endif
   
