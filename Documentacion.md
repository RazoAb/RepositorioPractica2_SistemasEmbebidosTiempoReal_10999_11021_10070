# Conclusiones finales – Práctica 2: Panel de instrumentos multiparámetro con FreeRTOS Queues

**Equipo:** Abraham Razo, Silvano Payan y Fátima Jiménez
**Plataforma:** ESP32-S3, ESP-IDF y PlatformIO

## 1. Decisiones de diseño del equipo

1. **Comunicación entre tareas mediante Queues.** Para organizar el programa, utilizamos tres tareas productoras (`TaskIMU`, `TaskTemp` y `TaskDistance`), que envían los datos de sus sensores a `SensorQueue`. Esta cola tiene capacidad para 10 mensajes. Por otro lado, los botones envían comandos a `CommandQueue`, con capacidad para 5 elementos. `TaskDataManager` recibe los mensajes de ambas colas, actualiza los datos y publica el estado del panel en `DisplayQueue`. De esta manera, la comunicación entre tareas se realiza mediante colas, sin utilizar semáforos, mutex, Event Groups, Task Notifications ni variables globales como medio de comunicación.

2. **Uso diferente de las colas según la información que manejan.** `SensorQueue` almacena las muestras que envían los sensores, por lo que puede tener varios mensajes pendientes. En cambio, `DisplayQueue` solo necesita conservar el estado más reciente del panel. Por eso tiene una longitud de un elemento y utilizamos `xQueueOverwrite()`, que reemplaza el contenido anterior cuando llega una actualización nueva. Así evitamos acumular información que ya quedó desactualizada.

3. **Tiempos de espera y envío de mensajes.** Las tareas de los sensores utilizan `xQueueSend(..., 0)` para enviar sus datos sin esperar si la cola está llena. Elegimos esta opción para evitar que una tarea se quede bloqueada y altere su periodo de ejecución con `vTaskDelayUntil()`. `TaskDataManager` revisa las colas sin tiempo de espera y después realiza una pausa de 50 ms. En cada ciclo atiende como máximo un comando y una muestra de sensor. Por su parte, `TaskDisplay` espera mediante `portMAX_DELAY`, ya que solo necesita ejecutarse cuando hay una actualización disponible.

4. **Procesamiento de los botones mediante interrupciones.** Configuramos los botones en los GPIO 16, 17 y 18 con resistencias pull-up. Las interrupciones únicamente identifican qué botón se presionó y envían el comando correspondiente mediante `xQueueSendFromISR()`. El procesamiento de cada comando se realiza después en `TaskDataManager`, evitando cargar las rutinas de interrupción con otras operaciones.

5. **Cambio de frecuencia de adquisición.** Las tres tareas de los sensores reciben mediante `pvParameters` un puntero a la variable que indica el modo de adquisición (`acq_mode_t *`). De acuerdo con su valor, trabajan con un periodo de 200 ms en NORMAL o de 50 ms en HIGH LOAD. Esto corresponde a una frecuencia configurada de 5 o 20 muestras por segundo, respectivamente. El cambio de modo se realiza desde `TaskDataManager` cuando recibe el comando LOAD.

6. **Control de la pantalla desde una sola tarea.** Decidimos que `TaskDisplay` fuera la única tarea encargada de generar los gráficos y comunicarse por SPI con los cuatro módulos MAX7219 conectados en cascada. Los sensores únicamente envían sus datos, mientras que la actualización de las matrices queda a cargo de la tarea de visualización.

7. **Organización del código.** Dividimos el programa en archivos dentro de `include/` y `src/`, procurando que cada módulo tuviera una función específica. Entre los archivos principales están `task_imu.c`, `task_temp.c`, `task_distance.c`, `task_manager.c`, `buttons.c`, `display.c`, `max7219.c` y `queues.c`. Los tipos de datos compartidos se definieron en `data_types.h`. Antes de desarrollar los módulos, acordamos las estructuras y la organización general para facilitar la integración del trabajo de los tres integrantes.

## 2. Comportamiento observado y análisis de resultados

- **Diferencias entre NORMAL y HIGH LOAD.** En NORMAL, la frecuencia de producción configurada es de aproximadamente 15 mensajes por segundo entre los tres sensores, mientras que en HIGH LOAD aumenta a unos 60 mensajes por segundo. Como `TaskDataManager` procesa como máximo un mensaje de sensor cada 50 ms, su capacidad de atención es de aproximadamente 20 mensajes por segundo. En el primer modo, la cola puede manejar el tráfico esperado; en el segundo, los mensajes llegan más rápido de lo que se procesan y `SensorQueue` puede llenarse en poco tiempo. Cuando esto sucede, los nuevos mensajes se descartan porque las tareas utilizan un tiempo de espera de cero. 

- **Limitaciones de la lectura de temperatura.** Aunque en HIGH LOAD el periodo configurado es de 50 ms, el DS18B20 necesita tiempo para completar cada conversión de temperatura. Dependiendo de la resolución utilizada, esta operación puede tardar hasta aproximadamente 750 ms. Por esta razón, la tarea de temperatura no puede alcanzar necesariamente las 20 muestras por segundo. Esta limitación depende del funcionamiento del sensor y no únicamente de la configuración de FreeRTOS.

- **Funcionamiento del HC-SR04 en HIGH LOAD.** Al configurar un periodo de 50 ms, el sensor de distancia puede realizar mediciones con una separación menor a los aproximadamente 60 ms recomendados en determinadas condiciones para evitar interferencias entre ecos. Esto puede provocar lecturas incorrectas, especialmente cuando las condiciones de medición no son favorables.

- **Modo FREEZE.** Durante las pruebas observamos que FREEZE detiene la actualización visual, pero no la adquisición de los sensores ni el procesamiento de `TaskDataManager`. Mientras el modo está activo, la pantalla no actualiza su contenido, aunque `DisplayQueue` continúa recibiendo el estado más reciente. Cuando se desactiva FREEZE, el panel puede mostrar los datos actuales en lugar de recuperar la información que tenía antes de congelarse.

- **Estabilidad de los valores mostrados.** En el Experimento A, aumentar la frecuencia de adquisición incrementa la cantidad de mensajes que circulan por las colas, pero no elimina las variaciones del ángulo calculado con el acelerómetro. Las vibraciones siguen afectando las lecturas, por lo que una mayor frecuencia no garantiza una indicación más estable. Una posible mejora sería calcular el promedio de varias muestras antes de actualizar los datos que se muestran en pantalla, como se plantea en el Experimento B.

- **Respuesta de los botones.** Las interrupciones se configuraron para detectar el flanco de subida.

## 3. Limitaciones y mejoras pendientes

Durante el desarrollo identificamos algunos aspectos que todavía necesitan corregirse o completarse:

- Debido a que todavía faltan estas mediciones y pruebas, los valores de tráfico de mensajes presentados en el reporte corresponden al análisis de las frecuencias configuradas y a pruebas parciales, no a una medición sistemática de todos los contadores.

A partir de lo observado, consideramos que las principales mejoras serían procesar varios mensajes pendientes de `SensorQueue` en cada iteración de `TaskDataManager`, agregar antirrebote a los botones y filtrar las lecturas antes de publicar los datos para la pantalla. También sería conveniente revisar los periodos de adquisición de los sensores para respetar sus tiempos de funcionamiento.

## 4. Conclusión individual – Luis Abraham Razo Quintero (TaskDataManager, botones y temperatura)

Durante la práctica trabajé en `TaskDataManager`, las interrupciones de los botones y `TaskTemp`. La tarea administradora se encarga de recibir los mensajes de los sensores y los comandos de los botones, actualizar los valores de pitch, roll, temperatura y distancia, y enviar el estado completo a la pantalla. Para esto utilizamos `xQueueOverwrite()`, ya que no es necesario conservar todos los estados anteriores del panel, sino mostrar el más reciente.

En los botones, las interrupciones solamente envían el comando correspondiente mediante `xQueueSendFromISR()`. El resto del procesamiento se realiza dentro de una tarea normal. Esto me permitió entender mejor la diferencia entre las funciones de FreeRTOS que se utilizan desde una tarea y las que deben utilizarse desde una ISR.

También observé que el sistema tiene dificultades para procesar todos los mensajes cuando se activa HIGH LOAD. La producción llega aproximadamente a 60 mensajes por segundo, mientras que el administrador procesa alrededor de 20. Como consecuencia, la cola puede llenarse y algunos mensajes se pierden. Una mejora sería revisar los mensajes pendientes en cada ciclo del administrador y agregar contadores para conocer con mayor precisión cuántos mensajes se producen, se procesan y se descartan.

## 5. Conclusión individual – Silvano Payan Covarrubias (MPU6050 y pantalla)

Mi parte del proyecto consistió en desarrollar `TaskIMU` y `TaskDisplay`. La tarea del IMU obtiene las lecturas del acelerómetro mediante I²C, las convierte a unidades de g para el rango de ±2 g y calcula los ángulos de roll y pitch utilizando `atan2f`. Después, envía los resultados a la cola de sensores. Para controlar el periodo de ejecución se utiliza `vTaskDelayUntil()`, mientras que `xQueueSend(..., 0)` permite enviar los datos sin bloquear la tarea si la cola está llena.

En la parte de visualización, `TaskDisplay` es la única encargada de comunicarse con los módulos MAX7219. La tarea espera a que haya información disponible en `DisplayQueue` mediante `portMAX_DELAY` y, cuando FREEZE está activo, no actualiza la pantalla. En el modo GRAPHIC se representa un horizonte artificial cuya inclinación depende del roll y cuyo centro cambia con el pitch. En LEVEL se muestra una barra de pitch con una referencia central. En ambos casos, los valores se limitan a un rango de ±45°. La temperatura se representa separando las decenas y unidades en las matrices M2 y M3, con valores limitados de 0 a 99.

Con esta parte de la práctica aprendí cómo obtener una estimación de la orientación a partir de las lecturas de un acelerómetro y cómo separar la adquisición de datos de su representación gráfica. También comprobé que aumentar la frecuencia de lectura no es suficiente para obtener una indicación más estable, ya que las vibraciones siguen afectando los ángulos calculados. Para mejorar este comportamiento sería necesario implementar algún método de filtrado.

## 6. Conclusión individual – Fátima Jiménez Jacobo (HC-SR04 y estructura del proyecto)

Mi parte del proyecto fue desarrollar `TaskDistance` en `task_distance.c` y el módulo de lectura del sensor en `distance.c` y `distance.h`. También apoyé en la organización de los archivos y en la configuración del proyecto para trabajar con el ESP32-S3.

El módulo de distancia mide el ancho del pulso Echo del HC-SR04 y utiliza ese tiempo para calcular la distancia en centímetros, considerando un rango de 2 a 400 cm. Además, contempla diferentes situaciones de error, como la falta de respuesta del sensor, una señal Echo que permanece activa o un pulso demasiado corto.

Una decisión importante fue distinguir entre la ausencia de un objeto dentro del rango de medición y una falla en la lectura. Cuando no se recibe un eco dentro del tiempo esperado, la tarea interpreta la situación como una distancia fuera del rango útil y envía un valor de 400 cm. En cambio, cuando se detecta una falla real, como un problema de conexión o ruido en la señal, no se envía una distancia inventada. En ese caso, se informa del problema por UART una vez y se vuelve a notificar cuando el sensor recupera su funcionamiento.

La tarea también consulta el modo de adquisición en cada ciclo para trabajar con periodos de 200 ms en NORMAL y 50 ms en HIGH LOAD. El uso de `vTaskDelayUntil()` permite mantener la temporización prevista, mientras que `xQueueSend(..., 0)` evita que la tarea quede bloqueada cuando `SensorQueue` está llena.


Esta parte del proyecto me ayudó a comprender cómo integrar un sensor de distancia con las tareas de FreeRTOS y cómo enviar información estructurada entre módulos mediante colas. También trabajé con `TaskHandle_t` y `QueueHandle_t` para relacionar los componentes del programa, además de utilizar PlatformIO para la configuración y el desarrollo. Finalmente, comprobé que definir desde el principio las estructuras de datos, los pines y la distribución de archivos facilita la integración de los módulos desarrollados por cada integrante del equipo.
