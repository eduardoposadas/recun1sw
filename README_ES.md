# recun1sw

El 11/11/2021 cometí un error. Compré un reloj inteligente [Cubot N1](https://www.cubot.net/wearables/N1) para monitorizar mi frecuencia cardíaca mientras salía a correr. Tras unas semanas de uso me di cuenta de que las mediciones, por decirlo de una forma suave, son solamente aproximadas. Además, la aplicación para el teléfono móvil deja bastante que desear y no es de código abierto, por lo que no es posible saber dónde van a acabar mis datos personales. En definitiva, me di cuenta de que el reloj inteligente Cubot N1 es una basura y había tirado mi dinero.

Constado el error, decidí divertirme con el reloj de otra forma: hacer ingeniería inversa del protocolo de comunicación que el reloj usa para enviar los datos a la aplicación del teléfono móvil. Una vez que supiera como el reloj pasa las mediciones a la aplicación del teléfono móvil podría hacer una pequeña aplicación Linux que mostrase unas gráficas con las muestras tomadas por el reloj.

El motivo para hacer una aplicación para Linux y no hacer un aplicación para Android es, sinceramente, que no sé absolutamente nada sobre desarrollo en Android. Si hubiese sabido, seguramente hubiera hecho un PR a alguna aplicación ya existente como [Gadgetbridge](https://codeberg.org/Freeyourgadget/Gadgetbridge/).

Una vez que tenía el objetivo fijado, establecí los pasos para conseguirlo:

 1. Hacer ingeniería inversa del protocolo que el reloj y la aplicación del teléfono móvil usan para comunicarse. Esto puede lograrse de dos maneras:	 
	 - Descompilar la [aplicación](https://play.google.com/store/apps/details?id=com.yc.gloryfit) para el móvil que Cubot facilita para gestionar el reloj.
	 - Capturar el tráfico Bluetooth entre el reloj y la aplicación del móvil e inspeccionarlo con una analizador de tráfico como Wireshark.
 2. Confirmar y ampliar el conocimiento adquirido en el paso anterior usando pequeños scripts fáciles de desarrollar.
 3. Desarrollar una aplicación Linux que se comunicase con el reloj y mostrase gráficas de los datos obtenidos por el reloj.

## Ingeniería inversa del protocolo de comunicación.
### Descompilar la aplicación
 El punto de partida más obvio para obtener información acerca del protocolo de comunicación entre el reloj y la App es la App en sí misma. Desafortunadamente, en el momento en el que intenté descompilarla, la aplicación parecía que estaba protegida por [Legu Packer de Tencent](https://github.com/quarkslab/legu_unpacker_2019) y no fui capaz de *desofuscar* el código. Fue muy frustrante. Si al menos fuera una aplicación que funciona correctamente podría entender la precaución de que nadie más usara su magnífico código, pero es que el funcionamiento de la aplicación deja bastante que desear.

### Capturar el tráfico Bluetooth entre el reloj y la App.
En el anterior teléfono que tenía, un Android 7, la forma más sencilla de capturar el tráfico Bluetooth fue activar en el teléfono las opciones para [desarrolladores](https://developer.android.com/studio/debug/dev-options), habilitar el registro de búsqueda de la Interfaz de Controlador (HCI) de host [Bluetooth](https://developer.android.com/studio/debug/dev-options#general) y examinar el fichero creado por el teléfono con [Wireshark](https://www.wireshark.org/). Para copiar el fichero que contiene la captura de tráfico Bluetooth, llamado `btsnoop_hci.log`, del teléfono al ordenador utilicé [KDE Connect](https://kdeconnect.kde.org/) que viene instalado por defecto en [Kubuntu](https://kubuntu.org/), la distribución que utilizo. Un proceso directo que no fue complicado.

Con el nuevo teléfono que tengo ahora, un Android 10, el proceso no ha sido tan fácil. La única manera sencilla y rápida que he encontrado para conseguir una captura del tráfico Bluetooth ha sido:

 1. Instalar [Wireshark](https://www.wireshark.org/)  y [adb](https://developer.android.com/studio/command-line/adb) en el ordenador con `sudo apt install wireshark-qt adb`.
 2.  [Activar](https://developer.android.com/studio/debug/dev-options) en el teléfono las opciones para desarrolladores.
 3. [Habilitar](https://developer.android.com/studio/debug/dev-options#enable) la depuración por USB.
 4. [Habilitar](https://developer.android.com/studio/debug/dev-options#general) el registro de búsqueda de la Interfaz de Controlador (HCI) de host Bluetooth para iniciar la captura del tráfico Bluetooth.   
 5. Conectar el teléfono al ordenador con un cable USB y comprobar que es accesible con `adb devices`.
    ```console
    user@DESKTOP:~$ adb devices
    * daemon not running; starting now at tcp:5037  
    * daemon started successfully  
    List of devices attached  
    CHISME10Plus18445 device
    ```
 6. Generar tráfico Bluetooth entre el teléfono y el reloj usando la App en el teléfono. Se puede optar por dos estrategias:
      - Hacer una captura de todo el tráfico Bluetooth generado durante el uso normal de la App. Por ejemplo, capturar todo el tráfico generado por la App al lanzarla, ver alguna gráfica y cerrar la App. Esto ayuda a tener una visión general del tráfico generado por la App.
      - Hacer una sola operación con la App, parar la captura de tráfico y examinar el tráfico generado. Esto ayuda a saber qué tráfico genera una única operación en la App.
 7. Seguir las [instrucciones](https://source.android.com/devices/bluetooth/verifying_debugging#debugging-with-bug-reports) de Google para generar un informe de errores del servicio Bluetooth:
    ```
    adb shell dumpsys bluetooth_manager > dumpsys_out.txt
    ```
 8. Usar el [script](https://cs.android.com/android/platform/superproject/+/master:packages/modules/Bluetooth/system/tools/scripts/btsnooz.py) `btsnooz.py` para generar un fichero con formato BTSnoop legible por Wireshark.
    ```
    python3 btsnooz.py dumpsys_out.txt > bt_capture.cfa
    ```
 9. Abrir el fichero `bt_capture.cfa` con Wireshark.

Los pasos 6, 7, 8 y 9 del proceso se repiten todas las veces que sean necesarias para conseguir la mayor cantidad de conocimiento acerca del protocolo de comunicación. Los pasos 7 y 8 se pueden ejecutar en una sola línea:
```
adb shell dumpsys bluetooth_manager | awk '/--- BEGIN:BTSNOOP_LOG_SUMMARY/ {found=1; next} /--- END:BTSNOOP_LOG_SUMMARY/ {found=0} {if (found) {print}}' | python3 btsnooz.py > bt_capture.cfa
```
Para comprobar que el fichero `bt_capture.cfa` se ha generado correctamente se puede usar `file` o leer el contenido con `btmon`:
```console
user@DESKTOP:~$ file bt_capture.cfa  
bt_capture.cfa: BTSnoop version 1, HCI UART (H4)  

user@DESKTOP:~$ btmon -r bt_capture.cfa | head
Bluetooth monitor ver 5.64  
< HCI Command: Vendor (0x3f|0x011b) plen 0 #1 0.202176  
< HCI Command: Vendor (0x3f|0x011b) plen 0 #2 0.202176  
> HCI Event: Command Status (0x0f) plen 4 #3 0.202176  
Vendor (0x3f|0x011b) ncmd 1  
Status: Unknown HCI Command (0x01)
```

#### Errores comunes durante la captura de tráfico

Si el fichero `bt_capture.cfa` no se ha generado correctamente se pueden comprobar varias cosas:
 - El funcionamiento de adb.
    ```console
    user@DESKTOP:~$ adb devices
    List of devices attached  
    CHISME10Plus18445 device
    
    user@DESKTOP:~$ adb shell uptime
    10:59:54 up 2 days, 15:18, 0 users, load average: 12.80, 13.00, 13.21
    ```
    La orden `adb devices` lista los teléfonos conectados al ordenador. La orden `adb shell uptime` muestra la hora del teléfono: `10:59:54`, y cuanto tiempo que lleva encendido: `2 days, 15:18`.
    
 - Si aparece el error `unpack_from requires a buffer of at least 9 bytes for unpacking 9 bytes at offset 0 (actual buffer size is 0)` hay que revisar la salida de `adb shell dumpsys bluetooth_manager`.
    ```console
    user@DESKTOP:~$ adb shell dumpsys bluetooth_manager | awk '/--- BEGIN:BTSNOOP_LOG_SUMMARY/ {found=1; next} /--- END:BTSNOOP_LOG_SUMMARY/ {found=0} {if (found) {print}} ' | python3 btsnooz.py > bt_capture.cfa
	 Failed uudecoding...ensure input is a valid uuencoded stream.  
	 Traceback (most recent call last):  
	   File "/router/Conf/BT sniffing/adb/btsnooz.py", line 158, in main
	     decode_snooz(base64.standard_b64decode(base64_string))
	   File "/router/Conf/BT sniffing/adb/btsnooz.py", line 76, in decode_snooz
	     version, last_timestamp_ms = struct.unpack_from('=bQ', snooz)
	 struct.error: unpack_from requires a buffer of at least 9 bytes for unpacking 9 bytes at offset 0 (actual buffer size is 0)  
 	
	 During handling of the above exception, another exception occurred:  
 	
	 Traceback (most recent call last):  
	   File "/router/Conf/BT sniffing/adb/btsnooz.py", line 198, in <module>
	     main()  
	   File "/router/Conf/BT sniffing/adb/btsnooz.py", line 162, in main
	     sys.stderr.write(e)  
	 TypeError: write() argument must be str, not error
    ```

    La salida de `adb shell dumpsys bluetooth_manager` debe terminar con un bloque de líneas delimitado por `--- BEGIN:BTSNOOP_LOG_SUMMARY` y `--- END:BTSNOOP_LOG_SUMMARY` como aparece aquí:
    ```console
    user@DESKTOP:~$ adb shell dumpsys bluetooth_manager
     ... [Líneas cortadas] ...
     BT Quality Report Events:  
     Event queue is empty.  
     --- BEGIN:BTSNOOP_LOG_SUMMARY (69696 bytes in) ---
     AgdDfxcoWAAAQtNFkOzMOrBGWvnk+VF3nSqIx+csXY+WcAKfl0mfl6mR5iqIaHt4qE+2C96QHgbryEBcm4MnwXgK/tQ7zr3W4MkAl6F3CnJr8K/ea7pORZR/zyDG0sMJ
     pQtNFkOzMOrBGWvnk+VF3nSqIx+csXY+WV4UzUJ15IMz1s4ny4sJLzSl0Q7OWDvfPy+CJ9Vux7HYqe//RJXF6h25dUdo3XIsyNUisli95ahi+Hh1VnzWOqs8a52V283K
     6riOA0rTBD5fp32+Tvr8Iqn5xRiao5mEzYxYzFPYhjozAHRYHs3K/vuH6DiLV/qPc3Sk5onRmx8Yw0Fy5K1AEe5TLDqCC+S4LsQYVyVd56kc9mGcDIhGZYWI1MpAqZPL
     BVwFkWEeHuz1TKBjEipeus9BZl6LVSUtWncrjm7CvxbpdYvK7gVUQdMEI71Aj/QCOdL8Xo0YaQ8n8NklfXapkeYqiGh7eLDXkVuLRai1tVhV0qZ1t+PoJvxrk163qZFe
     7r6Z0eA41v0PEFPUlQ==
     --- END:BTSNOOP_LOG_SUMMARY ---
    ```
    Si no aparece este bloque de líneas hay que revisar que el Bluetooth del teléfono está encendido y que el registro de búsqueda de la Interfaz de Controlador (HCI) de host Bluetooth está habilitado, es decir el paso 4 de la lista anterior.

#### Análisis del tráfico Bluetooth
Para poder analizar del tráfico capturado es importante tener claro cómo funciona el protocolo Bluetooth Low Energy (BLE). Se puede leer esta excelente [guía](https://reverse-engineering-ble-devices.readthedocs.io/en/latest/index.html) que es la que yo mismo he utilizado.

Al abrir el fichero `bt_capture.cfa` con Wireshark aparece una ventana similar a esta:
![Ventana principal de Wireshark](https://user-images.githubusercontent.com/55441185/208170469-131df20f-288a-4f91-929a-4506989ad18f.png)
Puede parecer bastante intimidante, pero Wireshark permite usar filtros para mostrar únicamente los paquetes relevantes para nuestro objetivo.

En la primera toma de contacto es mejor tener una visión general del tráfico que genera la App, por lo que es conveniente hacer una captura de todo el proceso, es decir: iniciar la captura de tráfico (punto 4 de REF TODO CORREGIR), arrancar la App en el teléfono, dejar que al App haga la recogida de datos almacenados en el reloj, cerrar la App y detener la captura de tráfico. Una vez hecha esta captura de tráfico de principio a fin, se puede observar todo el proceso de recogida de datos por parte de la App.

En la primera parte de la captura se pueden ver gran cantidad de paquetes con el protocolo HCI. Estos paquetes no contienen información útil para nuestro propósito.

![Paquetes HCI](https://user-images.githubusercontent.com/55441185/208172292-9d9dc3b5-c9cd-4744-840f-ac26690cf51d.png)

A continuación vamos a usar `btatt` como filtro:

![Filtro btatt](https://user-images.githubusercontent.com/55441185/208172708-6dafe83b-decd-41ec-af48-39532cdd4bf8.png)

Con este filtro se muestran los paquetes con protocolo Bluetooth Attribute Protocol. Este protocolo es el usado para mandar datos a nivel de aplicación entre dispositivos Bluetooth Low Energy, es decir con el filtro `btatt` Wireshark va a mostrar únicamente los paquetes en los que la App pregunta al reloj información y los paquetes de las respuestas del reloj a la App.

Al principio de la conversación entre la App y el reloj aparecen paquetes con la palabra GATT en la descripción, como "GATT Primary Service Declaration". En esta parte de la conversación el teléfono está preguntando al reloj qué servicios y características tiene. En el protocolo Bluetooth Low Energy cada dato que puede dar un dispositivo, como el valor de la frecuencia cardíaca que da un reloj inteligente, se llama característica y a un grupo de características se le llama servicio.
Es útil conocer la lista de servicios y características para saber cómo hay que preguntar datos al dispositivo Bluetooth Low Energy. Más adelante se muestra cómo sacar la lista de servicios y características y cómo usarla.

Avanzando en la conversación entre el teléfono y el reloj se pueden ver paquetes con datos. Usando el filtro `btatt.value` se muestran únicamente estos paquetes:

![filtro btatt.value](https://user-images.githubusercontent.com/55441185/208172888-6555a5a1-29da-4969-983c-fb46cc55604f.png)


Seleccionando en el panel central Bluetooth Attribute Protocol y desplegando sus campos podemos ver el campo `Value`, que en este caso tiene un valor de `0x01f01708`. Este campo es el dato devuelto por el reloj a una consulta que ha realizado la App del teléfono. En la imagen anterior se puede ver que la petición se realizó en el paquete 332, pero para ver el paquete 332 es necesario volver a usar el filtro `btatt`.

![filtreo batt](https://user-images.githubusercontent.com/55441185/208172965-b45fb1ff-6906-4e41-a378-c1215d8cfb54.png)


En la descripción del paquete 332 y en su contendido se puede ver que es una petición de lectura usando el handle `0x002a`. Un handle es un número asociado a una característica en concreto, o lo que es lo mismo, para leer un dato de una característica hay que hacer una petición de lectura a su handle asociado.
Con el número de handle y el valor devuelto se puede intentar replicar la petición para confirmar que se está interpretando correctamente la salida de Wireshark y que es posible la comunicación con el reloj de manera programática.
Se hace un escaneo Bluetooth Low Energy con `hcitool`:
```console
user@DESKTOP:~$ sudo timeout 10s hcitool lescan  
LE Scan ...  
66:GF:13:XX:XX:XX (unknown)
78:02:B7:XX:XX:XX N1(ID-XXXX)
F1:CG:BF:XX:XX:XX (unknown)
```
Los relojes Cubot N1 aparecen en la lista con un nombre como `N1(ID-XXXX)`, donde XXXX son los últimos cuatro dígitos de la dirección MAC. Una vez localizada la dirección MAC, se lanza la petición de lectura del handle `0x002a` con `gatttool`:
```console
user@DESKTOP:~$ gatttool -b 78:02:B7:XX:XX:XX --char-read -a 0x002a  
Characteristic value/descriptor: 01 f0 17 08
```
El valor devuelto es el valor que aparece en la captura de tráfico que se ha visualizado con Wireshark, por lo que se confirma que la interpretación que se está haciendo de la captura de tráfico BLE es correcta y que se puede interactuar con el reloj.

Una vez que se conoce la forma de hacer consultas simples al reloj, hay que averiguar cómo consigue la App los datos con los que hace las gráficas, como por ejemplo la gráfica de frecuencia cardíaca:

![Captura datos HR](https://user-images.githubusercontent.com/55441185/208173267-83b14858-8e9f-409b-97a4-9f9be4d1eb89.png)

A simple vista, ordenando por la longitud de los paquetes en Wireshark, no hay paquetes de más de 255 bytes.

![Wireshark ordenado por longitud](https://user-images.githubusercontent.com/55441185/208173337-8a2b4896-7744-4dbc-aa84-33e37b449fe0.png)

Si no hay paquetes grandes tiene que haber muchos paquetes pequeños, ya que las gráficas de la App muestran varios días. La forma más sencilla de contar el número de paquetes que hay de cada tipo es utilizar la orden `tshark` en la consola. `tshark` Se instala con `sudo apt install tshark` y para contar el número de paquetes de cada tipo se ejecuta la orden:
```console  
user@DESKTOP:~$ tshark -r bt_capture.cfa -T fields -e _ws.col.Info | sort | uniq -c | sort -n
1 Rcvd Command Status (Create Connection)  
1 Sent Create Connection  
2 Rcvd Command Status (LE Connection Update)  
2 Rcvd Connect Complete  
2 Rcvd Connection Parameter Update Request  
2 Rcvd LE Meta (LE Connection Update Complete)  
2 Sent Connection Parameter Update Response (Accepted)  
2 Sent LE Connection Update  
14 Rcvd Write Response, Handle: 0x0010 (Unknown)  
14 Sent Write Request, Handle: 0x0010 (Unknown)  
16 Rcvd Number of Completed Packets  
232 Rcvd Handle Value Notification, Handle: 0x0015 (Unknown)
```

Nota: Para obtener en la consola un fichero CSV con unos datos similares a los que Wireshark muestra en la interfaz gráfica se puede usar la orden:
```console  
user@DESKTOP:~$ tshark -r bt_capture.cfa -T fields -E separator=, -E header=y -E quote=d -e frame.number -e frame.time_relative -e _ws.col.Source -e _ws.col.Destination -e _ws.col.Protocol -e _ws.col.Info
frame.number,frame.time_relative,_ws.col.Source,_ws.col.Destination,_ws.col.Protocol,_ws.col.Info  
"1","0.000000000","localhost ()","remote ()","ATT","Sent Write Request, Handle: 0x0010 (Unknown)"  
"2","0.006782000","controller","host","HCI_EVT","Rcvd Number of Completed Packets"  
"3","0.247289000","remote ()","localhost ()","ATT","Rcvd Handle Value Notification, Handle: 0x0015 (Unknown)"  
"4","0.252755000","remote ()","localhost ()","ATT","Rcvd Write Response, Handle: 0x0010 (Unknown)"  
"5","0.287181000","remote ()","localhost ()","ATT","Rcvd Handle Value Notification, Handle: 0x0015 (Unknown)"  
"6","0.366944000","remote ()","localhost ()","ATT","Rcvd Handle Value Notification, Handle: 0x0015 (Unknown)"  
"7","0.406672000","remote ()","localhost ()","ATT","Rcvd Handle Value Notification, Handle: 0x0015 (Unknown)"  
"8","1.379788000","localhost ()","remote ()","ATT","Sent Write Request, Handle: 0x0010 (Unknown)"  
"9","1.406752000","controller","host","HCI_EVT","Rcvd Number of Completed Packets"
```

El que haya 232 paquetes del tipo `Rcvd Handle Value Notification, Handle: 0x0015 (Unknown)` es indicativo de que estos son los paquetes que se utilizan para pasar los datos biométricos del reloj a la App. Usando la interfaz gráfica de Wireshark se pueden buscar estos paquetes. Son tantos que no se tarda en encontrarlos moviendo la barra de desplazamiento. Para mostrar únicamente esos paquetes se puede usar el filtro `btatt.opcode == 0x1b`:

![Filtro btatt.opcode == 0x1b](https://user-images.githubusercontent.com/55441185/208173418-a063612b-73ad-4892-be51-470db34ba29c.png)

Las notificaciones del protocolo Bluetooth Low Energy son paquetes que el servidor GATT (el reloj en este caso) manda al cliente (la App del teléfono en este caso) con datos, por ejemplo el nivel de carga de la batería cuando ha llegado a un nivel bajo o el número de pasos dados por el usuario en la última hora . Las notificaciones pueden ser enviadas en cualquier momento y su recepción no necesitan ser confirmada. El cliente tiene que habilitar el envío de notificaciones de los tipos que quiera y, una vez habilitado, el servidor las enviará sin que el cliente tenga que solicitarlas.
Como las notificaciones BLE son enviadas por el reloj sin que la App las solicite, a primera vista no tiene sentido que el reloj use notificaciones BLE para enviar a la App datos que se cargan cada vez que se lanza la App en el teléfono. Las notificaciones parecen estar pensadas para avisos asíncronos entre el servidor GATT (el reloj en este caso) y el cliente (la App del teléfono en este caso), no para enviar volcados de datos bajo demanda.

Hay que seguir indagando.

En la interfaz gráfica de Wireshark, usando el filtro `btatt.opcode == 0x1b` para mostrar únicamente las notificaciones, se pueden observar que hay tres tandas de notificaciones en los que los números de paquetes (la primera columna) son correlativos:

![Tanda notificaciones 1](https://user-images.githubusercontent.com/55441185/208173528-63db7d05-d7ce-4467-8590-51c12b7226d1.png)

![Tanda notificaciones 2](https://user-images.githubusercontent.com/55441185/208173570-a76b719e-3988-4f5a-a5e6-e1d95b89d55a.png)

![Tanda notificaciones 3](https://user-images.githubusercontent.com/55441185/208173584-e4817633-b033-4604-aeaf-eaf13b7668d3.png)

Los paquetes 386, 622 y 662 parecen ser los primeros de tres series de notificaciones. Si se cambia el filtro por `btatt`, para ver los paquetes con protocolo Bluetooth Attribute Protocol, aparece un patrón:

![Tanda notificaciones 4](https://user-images.githubusercontent.com/55441185/208173995-76f73fba-d0cf-4923-8005-3c5bfcc53ba2.png)

![Tanda notificaciones 5](https://user-images.githubusercontent.com/55441185/208174002-f81900b5-d304-4513-8d70-1536f5d4babf.png)

![Tanda notificaciones 6](https://user-images.githubusercontent.com/55441185/208174014-64c566c7-2e0c-43e8-a1e9-2185a4d73f6f.png)

Justo antes de cada tanda de notificaciones hay una petición de escritura al handle `0x0010` con su correspondiente respuesta. Al mostrar el contenido de la petición de escritura:

![Contenido petición escritura 1](https://user-images.githubusercontent.com/55441185/208174121-6863e86b-eb54-46be-874c-9f52057b9a24.png)

![Contenido petición escritura 2](https://user-images.githubusercontent.com/55441185/208174125-09ffdbae-2349-4d98-8d4a-83356ed36187.png)

![Contenido petición escritura 3](https://user-images.githubusercontent.com/55441185/208174133-c96049c7-6158-4bd7-9b9c-6e1452554359.png)

Parece que los volcados de datos son solicitados por la App del teléfono escribiendo un valor en el handle `0x0010` y que los datos son enviados del reloj al teléfono mediante notificaciones.
Sólo queda intentar replicar los volcados desde la linea de comandos para confirmar que la forma de realizar los volcados de datos es escribir en el handle `0x0010` los valores `0xb2fa`, `0xf7fa07e50c110e0c` y `0x34fa`, y esperar las notificaciones.

Como se ha mencionado arriba, para que le reloj envíe notificaciones primero hay que habilitarlas. Para buscar en la captura de tráfico cómo la App ha habilitado las notificaciones se usa el filtro `btatt.characteristic_configuration_client.notification == 1`:

![Filtro btatt.characteristic_configuration_client.notification == 1](https://user-images.githubusercontent.com/55441185/208174421-4d5dec41-4665-46db-999d-c6c8cd32d2a8.png)

La App habilita las notificaciones escribiendo el valor `0x0100` en los handles `0x0016` y `0x001f`. Para replicarlo en la consola:
```console  
user@DESKTOP:~$ gatttool -b 78:02:B7:XX:XX:XX --char-write-req -a 0x0016 -n 0100  
Characteristic value was written successfully
user@DESKTOP:~$ gatttool -b 78:02:B7:XX:XX:XX --char-write-req -a 0x001f -n 0100
Characteristic value was written successfully
```
Y ahora las peticiones de volcados de datos:
```console  
user@DESKTOP:~$ timeout --foreground 15s gatttool -b 78:02:B7:XX:XX:XX --char-write-req -a 0x0010 -n b2fa --listen
Characteristic value was written successfully
... [Líneas cortadas] ...
Notification handle = 0x0015 value: b2 07 e6 08 0d 0c 09 73 21 2d 0b 06 93 02 3a 0c 02 e0  
Notification handle = 0x0015 value: b2 07 e6 08 0d 0d 04 00 2b 32 01 00 1e 00 3b 0c 03 e2  
Notification handle = 0x0015 value: b2 07 e6 08 0d 0e 01 cc 00 00 00 00 00 00 35 06 01 cc  
Notification handle = 0x0015 value: b2 07 e6 08 0d 0f 00 43 00 00 00 00 00 05 27 02 00 43  
Notification handle = 0x0015 value: b2 07 e6 08 0d 10 00 49 00 00 00 00 00 29 2f 02 00 49  
Notification handle = 0x0015 value: b2 07 e6 08 0d 11 00 2d 00 00 00 00 00 19 1d 01 00 2d  
Notification handle = 0x0015 value: b2 fd 26

user@DESKTOP:~$ timeout --foreground 15s gatttool -b 78:02:B7:XX:XX:XX --char-write-req -a 0x0010 -n f7fa07e50c110e0c --listen
Characteristic value was written successfully
... [Líneas cortadas] ...
Notification handle = 0x0015 value: f7 07 e6 08 0d 0a 35 34 34 39 36 40 50 33 32 34 39 3a  
Notification handle = 0x0015 value: f7 07 e6 08 0d 0c 5b 57 39 3d 33 37 3e 38 34 3b 41 30  
Notification handle = 0x0015 value: f7 07 e6 08 0d 0e 3e 41 3f 6d 38 55 4c 39 53 43 73 56  
Notification handle = 0x0015 value: f7 07 e6 08 0d 10 4b 47 58 4a 4e 55 4a 3c 3c 45 45 38  
Notification handle = 0x0015 value: f7 07 e6 08 0d 12 38 39 3c 3f 36 3c 37 3b 34 34 33 37  
Notification handle = 0x0015 value: f7 07 e6 08 0d 14 3d 35 37 35 36 33 30 ff ff ff ff ff  
Notification handle = 0x0015 value: f7 fd cf

user@DESKTOP:~$ timeout --foreground 15s gatttool -b 78:02:B7:XX:XX:XX --char-write-req -a 0x0010 -n 34fa --listen  
Characteristic value was written successfully  
Notification handle = 0x0015 value: 34 fa 07 e6 07 07 10 00 ff ff ff 62 62 ff ff ff ff ff ff ff  
Notification handle = 0x0015 value: 34 fa 07 e6 07 0a 0e 00 ff ff ff 62 62 ff ff ff ff ff ff ff  
Notification handle = 0x0015 value: 34 fa 07 e6 07 0f 14 00 ff ff ff 62 62 ff ff ff ff ff ff ff  
Notification handle = 0x0015 value: 34 fa 07 e6 07 10 0a 00 ff ff ff ff ff ff ff ff ff ff ff ff  
Notification handle = 0x0015 value: 34 fa 07 e6 08 0d 0c 00 62 ff 62 ff ff ff ff ff ff ff ff ff  
Notification handle = 0x0015 value: 34 fa 07 e6 08 0d 0e 00 62 ff 62 ff ff ff ff ff ff ff ff ff  
Notification handle = 0x0015 value: 34 fa fd 10
```

Ahora que la forma de hacer los volcados es conocida, hay que intentar averiguar cual es el significado de los datos contenidos en los volcados. 

## Confirmar y ampliar el conocimiento adquirido en el paso anterior usando pequeños scripts fáciles de desarrollar
Para poder trabajar de una forma más ágil se guardan los volcados en archivos de texto:
```console    
user@DESKTOP:~$ timeout --foreground 15s gatttool -b 78:02:B7:XX:XX:XX --char-write-req -a 0x0010 -n b2fa --listen > b2fa.txt
user@DESKTOP:~$ timeout --foreground 15s gatttool -b 78:02:B7:XX:XX:XX --char-write-req -a 0x0010 -n f7fa07e50c110e0c --listen > f7fa.txt
user@DESKTOP:~$ timeout --foreground 15s gatttool -b 78:02:B7:XX:XX:XX --char-write-req -a 0x0010 -n 34fa --listen > 34fa.txt
```
### Volcado de muestras de frecuencia cardíaca
Sin más información que una lista de dígitos hexadecimales es complicado discernir el significado de un volcado de datos. Sin embargo a simple vista se puede intuir algo. Comparando dos lineas de distintos volcados:
```
   vv vv vv vv
b2 07 e6 08 0d 08 00 35 00 00 00 00 00 38 3b 02 00 35
f7 07 e6 08 0d 04 38 30 30 32 3b 3d 3a 34 34 33 31 31
   ^^ ^^ ^^ ^^
```
Los dígitos `07 e6 08 0d` se repiten en la misma posición. Pasando estos números a decimal:
 - `07e6` = 2022
 - `08` = 8
 - `0d` = 13

Estos dígitos son la fecha ,13 de Agosto de 2022, en la que se ha hecho el volcado. Si esos tres dígitos son la fecha es posible que el siguiente sea la hora. Por ejemplo, en el volcado hecho con `f7fa07e50c110e0c` la siguiente columna:
```
               vv
f7 07 e6 08 0d 0a 35 34 34 39 36 40 50 33 32 34 39 3a  
f7 07 e6 08 0d 0c 5b 57 39 3d 33 37 3e 38 34 3b 41 30  
f7 07 e6 08 0d 0e 3e 41 3f 6d 38 55 4c 39 53 43 73 56  
f7 07 e6 08 0d 10 4b 47 58 4a 4e 55 4a 3c 3c 45 45 38  
f7 07 e6 08 0d 12 38 39 3c 3f 36 3c 37 3b 34 34 33 37  
f7 07 e6 08 0d 14 3d 35 37 35 36 33 30 ff ff ff ff ff  
               ^^
```
Son números pares: `0a` = 10, `0c` = 12, `0e` = 14, `10` = 16, `12` = 18 y `14` = 20. Cada uno de estos números pares está seguido por 12 números. Estos 12 números podrían indicar el valor medio de alguna medida en los 12 tramos de diez minutos que hay en dos horas. Si, además de lo anterior, se supone que `ff` indica que el valor no existe o es incorrecto, al pasar estos números a decimal y mostrar las fechas se obtiene :
```console
user@DESKTOP:~$ cat f7fa.txt | awk '{$1=$2=$3=$4=$5=""; printf "%03d %d-%02d-%02d %02d:00 ", "0x"$6, "0x"$7$8, "0x"$9, "0x"$10, "0x"$11; for (i = 12; i <= NF; i++) {if ($i == "ff") {printf  
"--- "} else {printf "%03d ", "0x"$i}}; print "" }'  
247 2022-08-13 10:00 053 052 052 057 054 064 080 051 050 052 057 058  
247 2022-08-13 12:00 091 087 057 061 051 055 062 056 052 059 065 048  
247 2022-08-13 14:00 062 065 063 109 056 085 076 057 083 067 115 086  
247 2022-08-13 16:00 075 071 088 074 078 085 074 060 060 069 069 056  
247 2022-08-13 18:00 056 057 060 063 054 060 055 059 052 052 051 055  
247 2022-08-13 20:00 061 053 055 053 054 051 048 --- --- --- --- ---  
```
Contrastando estos valores con los valores de las diferentes gráficas que tiene la App se comprueba que el volcado realizado con `f7fa07e50c110e0c` corresponde al volcado de los datos de frecuencia cardíaca. Sólo queda un pequeño ajuste. Los valores de frecuencia cardíaca mostrados en cada linea no son los de los tramos de diez minutos de las dos horas posteriores a la hora indicada en la linea, sino de las dos horas anteriores. Es decir:
```
 247 2022-08-13 18:00 056 057 060 063 054 060 055 059 052 052 051 055
```
debería ser:
```
 247 2022-08-13 16:00 056 057 060 063 054 060 055 059 052 052 051 055
```
por lo que el paso a decimal del volcado es:
```console  
user@DESKTOP:~$ cat f7fa.txt | awk '{$1=$2=$3=$4=$5=""; printf "%03d %d-%02d-%02d %02d:00 ", "0x"$6, "0x"$7$8, "0x"$9, "0x"$10, (("0x"$11) + 22) % 24; for (i = 12; i <= NF; i++) {if ($i == "ff") {printf "--- "} else {printf "%03d ", "0x"$i}}; print "" }'
247 2022-08-13 08:00 053 052 052 057 054 064 080 051 050 052 057 058  
247 2022-08-13 10:00 091 087 057 061 051 055 062 056 052 059 065 048  
247 2022-08-13 12:00 062 065 063 109 056 085 076 057 083 067 115 086  
247 2022-08-13 14:00 075 071 088 074 078 085 074 060 060 069 069 056  
247 2022-08-13 16:00 056 057 060 063 054 060 055 059 052 052 051 055  
247 2022-08-13 18:00 061 053 055 053 054 051 048 --- --- --- --- ---
```
Con estos datos ya es posible hacer una gráfica de las medidas de la frecuencia cardíaca tomadas por el reloj. Sólo queda un pequeño detalle. Mientras que las otras dos peticiones de volcado se hacen mandando una petición con los valores `0xb2fa` y `0x34fa`, la petición de volcado de datos de frecuencia cardíaca se hace con `0xf7fa07e50c110e0c`. Si se observa con detenimiento este valor se puede ver que:
 - `07e5` = 2021
 - `0c` = 12
 - `11` = 17
 - `0e` = 14
 - `0c` = 12

Estos números corresponden a unos días antes de la fecha en la que se hizo la captura de tráfico Bluetooth: 17 de diciembre de 2021 a las 14:12. Haciendo nuevas peticiones de volcado de datos de frecuencia cardíaca se comprueba que la fecha de la petición es la fecha desde la que se quiere que empiece el volcado y que el reloj guarda siete días de muestras de frecuencia cardíaca.

El resumen del conocimiento obtenido sobre cómo conseguir los datos de frecuencia cardíaca:

- El volcado de datos se solicita al reloj escribiendo en el handle `0x0010` una fecha de inicio de datos del volcado con siguiente forma:
	|  - | Cabecera  | Año | Mes | Día | Hora | Minutos|
	| :-- | :--: | :--: | :--: | :--: | :--: | :--: |
	| Hex | f7fa | 07e6 | 01 | 01 | 0c | 00|
	| Dec | 63482 | 2202 | 01 | 01 | 12 | 00|
- El volcado está compuesto por notificaciones con el formato:
	| - | Cabecera  | Año | Mes | Día | Hora | H-2:00 | H-2:10 | H-2:20 | H-2:30 | H-2:40 | H-2:50 | H-1:00 | H-1:10 | H-1:20 | H-1:30 | H-1:40 | H-1:50 |
	| :-- | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: |
	| Hex | f7 | 07e6 | 01 | 01 | 0c | 3c | 3c | 3c | 3c | 3c | 3c | 3c | 3c | 3c | 3c | 3c | 3c |
	| Dec | 247 | 07e6 | 01 | 01 | 12 | 60 | 60 | 60 | 60 | 60 | 60 | 60 | 60 | 60 | 60 | 60 | 60 |
	Donde:
	- Cabecera: valor que indica que la notificación es parte de un volcado de datos de frecuencia cardíaca.
	-  Año, Mes y Día: fecha de la toma de los datos incluidos en la notificación.
	- Hora: hora de la toma de los datos incluidos en la notificación. El resto de la notificación es una lista de datos en la que cada dato es la media de la frecuencia cardíaca en una franja de diez minutos. El primer valor de la lista es la frecuencia cardíaca media en los diez minutos que empiezan dos horas antes de la hora indicada en el campo Hora, el segundo valor es la media de los diez minutos que empiezan 110 minutos antes de la hora indicada en el campo Hora... Por ejemplo, si el campo Hora es 14, la lista de datos será la frecuencia cardíaca media en las franjas de tiempo:
		- 12:00 - 12:09
		- 12:10 - 12:19
		- 12:20 - 12:29
		- 12:30 - 12:39
		- 12:40 - 12:49
		- 12:50 - 12:59
		- 13:00 - 13:09
		- 13:10 - 13:19
		- 13:20 - 13:29
		- 13:30 - 13:39
		- 13:40 - 13:49
		- 13:50 - 13:59
	- H-2:00: frecuencia cardíaca media en la franja de tiempo que empieza en el valor del campo Hora menos 120 minutos.
	- H-2:10: frecuencia cardíaca media en la franja de tiempo que empieza en el valor del campo Hora menos 110 minutos.
	- H-2:20: frecuencia cardíaca media en la franja de tiempo que empieza en el valor del campo Hora menos 100 minutos.
	- H-1:00: frecuencia cardíaca media en la franja de tiempo que empieza en el valor del campo Hora menos 60 minutos.
	- H-1:10: frecuencia cardíaca media en la franja de tiempo que empieza en el valor del campo Hora menos 50 minutos.
	- H-1:50: frecuencia cardíaca media en la franja de tiempo que empieza en el valor del campo Hora menos 10 minutos.
	- Si el valor de la frecuencia cardíaca es `ff` indica que no hay dato.


Una vez que se ha comprendido el significado de los diferentes valores del volcado de frecuencia cardíaca seguramente sea mucho más sencillo hacer lo mismo con los otros dos volcados, puesto que probablemente sean similares.

### Volcado de muestras de saturación de O<sub>2</sub> en sangre
El volcado realizado mandando una petición con el valor `0x34fa` se puede pasar a decimal con:
```console    
user@DESKTOP:~$ cat 34fa.txt | awk '{$1=$2=$3=$4=$5=""; printf "%05d %d-%02d-%02d %02d:00 ", "0x"$6$7, "0x"$8$9, "0x"$10, "0x"$11, (("0x"$12) + 22) % 24; for (i = 13; i <= NF; i++)  
{if ($i == "ff") {printf "--- "} else {printf "%03d ", "0x"$i}}; print "" }'
13562 2022-07-07 14:00 000 --- --- --- 098 098 --- --- --- --- --- --- ---  
13562 2022-07-10 12:00 000 --- --- --- 098 098 --- --- --- --- --- --- ---  
13562 2022-07-15 18:00 000 --- --- --- 098 098 --- --- --- --- --- --- ---  
13562 2022-07-16 08:00 000 --- --- --- --- --- --- --- --- --- --- --- ---  
13562 2022-08-13 10:00 000 098 --- 098 --- --- --- --- --- --- --- --- ---  
13562 2022-08-13 12:00 000 098 --- 098 --- --- --- --- --- --- --- --- ---
```
De nuevo, contrastando estos valores con los valores de las diferentes gráficas que tiene la App se comprueba que el volcado realizado con el valor `0x34fa` corresponde al volcado de los datos de saturación de O<sub>2</sub> en sangre. Averiguar qué representan los datos de este volcado ha sido particularmente sencillo porque el reloj Cubot N1 siempre da un valor de 98% para la saturación de O<sub>2</sub> en sangre y, aunque se configure el muestreo automático, el reloj sólo toma datos cuando la pantalla de saturación de O<sub>2</sub> se está mostrando en la esfera del reloj. Como ya he dicho, el Cubot N1 es una basura.

El resumen del conocimiento obtenido sobre cómo conseguir los datos de saturación de O<sub>2</sub> en sangre:
 - El volcado de datos se solicita al reloj escribiendo en el handle `0x0010` el valor `0x34fa`.
 - El volcado está compuesto por notificaciones con el formato:
	| - | Cabecera  | Año | Mes | Día | Hora | NUL | H-2:00 | H-2:10 | H-2:20 | H-2:30 | H-2:40 | H-2:50 | H-1:00 | H-1:10 | H-1:20 | H-1:30 | H-1:40 | H-1:50 |
	| :-- | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: |:--: |
	| Hex | 34fa | 07e6 | 01 | 01 | 0c | 00 | ff | ff | ff | 62 | ff | ff | ff | ff | ff | ff | ff | ff |
	| Dec | 13562 | 2022 | 01 | 01 | 12 | 00 | 255 | 255 | 255 | 98 | 255 | 255 | 255 | 255 | 255 | 255 | 255 | 255 |
	
	Donde:
	
	 - List item
	 - Cabecera: valor que indica que la notificación es parte de un volcado de datos de saturación de O<sub>2</sub> en sangre.
	 - Año, Mes y Día: fecha de la toma de los datos incluidos en la notificación.
	- Hora: hora de la toma de los datos incluidos en la notificación. Cada dato es la media de la saturación de O<sub>2</sub> en una franja de diez minutos que empieza restando dos horas al valor de este campo.
	  El resto de los campos, excepto NUL, son la lista de datos tal y como se explica en el volcado de frecuencia cardíaca.
	- NUL: siempre es cero.
	- H-2:00: saturación de O<sub>2</sub> media en la franja de tiempo que empieza en el valor del campo Hora menos 120 minutos.
	- H-2:10: saturación de O<sub>2</sub> media en la franja de tiempo que empieza en el valor del campo Hora menos 110 minutos.
	- H-2:20: saturación de O<sub>2</sub> media en la franja de tiempo que empieza en el valor del campo Hora menos 100 minutos.
	- H-1:00: saturación de O<sub>2</sub> media en la franja de tiempo que empieza en el valor del campo Hora menos 60 minutos.
	- H-1:10: saturación de O<sub>2</sub> media en la franja de tiempo que empieza en el valor del campo Hora menos 50 minutos.
	- H-1:50: saturación de O<sub>2</sub> media en la franja de tiempo que empieza en el valor del campo Hora menos 10 minutos.
	- Si el valor de la saturación de O<sub>2</sub> es `ff` indica que no hay dato.

### Volcado de muestras de pasos
El volcado realizado mandando una petición con el valor `0xb2fa` es algo diferente a los dos anteriores:
```
b2 07 e6 08 0c 14 00 29 00 00 00 00 00 34 3a 01 00 29
b2 07 e6 08 0c 15 00 41 00 00 00 00 00 06 34 01 00 41
b2 07 e6 08 0c 16 00 13 00 00 00 00 00 0d 0e 01 00 13
b2 07 e6 08 0c 17 00 14 00 00 00 00 00 1a 1b 01 00 14
b2 07 e6 08 0d 07 00 a8 00 00 00 00 00 08 32 02 00 a8
b2 07 e6 08 0d 08 00 35 00 00 00 00 00 38 3b 02 00 35
b2 07 e6 08 0d 0a 00 39 00 00 00 00 00 14 16 02 00 39
b2 07 e6 08 0d 0b 00 b8 00 00 00 00 00 2d 3b 02 00 b8
b2 07 e6 08 0d 0c 09 73 21 2d 0b 06 93 02 3a 0c 02 e0
b2 07 e6 08 0d 0d 04 00 2b 32 01 00 1e 00 3b 0c 03 e2
b2 07 e6 08 0d 0e 01 cc 00 00 00 00 00 00 35 06 01 cc
b2 07 e6 08 0d 0f 00 43 00 00 00 00 00 05 27 02 00 43
b2 07 e6 08 0d 10 00 49 00 00 00 00 00 29 2f 02 00 49
b2 07 e6 08 0d 11 00 2d 00 00 00 00 00 19 1d 01 00 2d
```
Se puede observar que el primer campo se repite en todas las notificaciones recibidas, por lo que es una cabecera identificadora del tipo de notificación. Los siguientes cuatro campos son una fecha, como en los dos anteriores volcados. El siguiente campo casi siempre es correlativo y siempre  es menor que 24 (0x18 en hexadecimal) por lo que, al igual que en los anteriores volcados, es el valor de la hora de la toma de los datos.

También es fácil comprobar que el penúltimo y el último campo y coinciden con el séptimo y el octavo campo si los valores de los campos noveno, décimo y undécimo son cero. Para interpretar el volcado se tomarán estas dos parejas de valores como uno solo.

A partir del séptimo campo no es fácil intuir cual es el significado. Quizás mostrando los valores en decimal se pueda averiguar cual es la gráfica en la App que muestra los datos del volcado:
```console      
user@DESKTOP:~$ cat b2fa.txt | awk '{$1=$2=$3=$4=$5=""; printf "%03d %d-%02d-%02d %02d:00 %04d ", "0x"$6, "0x"$7$8, "0x"$9, "0x"$10, "0x"$11, "0x"$12$13; for (i = 14; i <= NF - 2; i++) {if  
($i == "ff") {printf "--- "} else {printf "%03d ", "0x"$i}};printf "%04d\n", "0x"$22$23}'  
178 2022-08-12 20:00 0041 000 000 000 000 000 052 058 001 0041  
178 2022-08-12 21:00 0065 000 000 000 000 000 006 052 001 0065  
178 2022-08-12 22:00 0019 000 000 000 000 000 013 014 001 0019  
178 2022-08-12 23:00 0020 000 000 000 000 000 026 027 001 0020  
178 2022-08-13 07:00 0168 000 000 000 000 000 008 050 002 0168  
178 2022-08-13 08:00 0053 000 000 000 000 000 056 059 002 0053  
178 2022-08-13 10:00 0057 000 000 000 000 000 020 022 002 0057  
178 2022-08-13 11:00 0184 000 000 000 000 000 045 059 002 0184  
178 2022-08-13 12:00 2419 033 045 011 006 147 002 058 012 0736      <<<< Esta línea
178 2022-08-13 13:00 1024 043 050 001 000 030 000 059 012 0994      <<<< Esta línea
178 2022-08-13 14:00 0460 000 000 000 000 000 000 053 006 0460  
178 2022-08-13 15:00 0067 000 000 000 000 000 005 039 002 0067  
178 2022-08-13 16:00 0073 000 000 000 000 000 041 047 002 0073  
178 2022-08-13 17:00 0045 000 000 000 000 000 025 029 001 0045
```
Mirando la gráfica de pasos en la App del teléfono se observa que el último campo del volcado coincide con los datos de pasos menos en las líneas marcadas.

Ahora que se sabe que el volcado son datos que están en la gráfica de pasos, comparando datos del volcado con los mostrados en la gráfica de la App se llega a averiguar el significado de todos los campos del volcado.
```console        
user@DESKTOP:~$ cat b2fa.txt | awk '  
{  
$1=$2=$3=$4=$5=""  
printf "%03d %d-%02d-%02d %02d:00 Total: %04d ", "0x"$6, "0x"$7$8, "0x"$9, "0x"$10, "0x"$11, "0x"$12$13  
if ($14 != "00" || $15 != "00") {printf "MinRun: %02d:%02d %02d:%02d ", "0x"$11, "0x"$14, "0x"$11, "0x"$15} else {printf "MinRun: --:-- --:-- "}  
if ($16 != "00") {printf "NTimesRun: %03d ", "0x"$16} else {printf "NTimesRun: --- "}  
if ($17 != "00" || $18 != "00") {printf "TotalRun: %04d ", "0x"$17$18} else {printf "TotalRun: ---- "}  
if ($19 != "00" || $20 != "00") {printf "MinWalk: %02d:%02d %02d:%02d ", "0x"$11, "0x"$19, "0x"$11, "0x"$20} else {printf "MinWalk: --:-- --:-- "}  
printf "NTimesWalk: %03d TotalWalk: %04d ", "0x"$21, "0x"$22$23  
print ""  
}'  
178 2022-08-12 20:00 Total: 0041 MinRun: --:-- --:-- NTimesRun: --- TotalRun: ---- MinWalk: 20:52 20:58 NTimesWalk: 001 TotalWalk: 0041  
178 2022-08-12 21:00 Total: 0065 MinRun: --:-- --:-- NTimesRun: --- TotalRun: ---- MinWalk: 21:06 21:52 NTimesWalk: 001 TotalWalk: 0065  
178 2022-08-12 22:00 Total: 0019 MinRun: --:-- --:-- NTimesRun: --- TotalRun: ---- MinWalk: 22:13 22:14 NTimesWalk: 001 TotalWalk: 0019  
178 2022-08-12 23:00 Total: 0020 MinRun: --:-- --:-- NTimesRun: --- TotalRun: ---- MinWalk: 23:26 23:27 NTimesWalk: 001 TotalWalk: 0020  
178 2022-08-13 07:00 Total: 0168 MinRun: --:-- --:-- NTimesRun: --- TotalRun: ---- MinWalk: 07:08 07:50 NTimesWalk: 002 TotalWalk: 0168  
178 2022-08-13 08:00 Total: 0053 MinRun: --:-- --:-- NTimesRun: --- TotalRun: ---- MinWalk: 08:56 08:59 NTimesWalk: 002 TotalWalk: 0053  
178 2022-08-13 10:00 Total: 0057 MinRun: --:-- --:-- NTimesRun: --- TotalRun: ---- MinWalk: 10:20 10:22 NTimesWalk: 002 TotalWalk: 0057  
178 2022-08-13 11:00 Total: 0184 MinRun: --:-- --:-- NTimesRun: --- TotalRun: ---- MinWalk: 11:45 11:59 NTimesWalk: 002 TotalWalk: 0184  
178 2022-08-13 12:00 Total: 2419 MinRun: 12:33 12:45 NTimesRun: 011 TotalRun: 1683 MinWalk: 12:02 12:58 NTimesWalk: 012 TotalWalk: 0736  
178 2022-08-13 13:00 Total: 1024 MinRun: 13:43 13:50 NTimesRun: 001 TotalRun: 0030 MinWalk: 13:00 13:59 NTimesWalk: 012 TotalWalk: 0994  
178 2022-08-13 14:00 Total: 0460 MinRun: --:-- --:-- NTimesRun: --- TotalRun: ---- MinWalk: 14:00 14:53 NTimesWalk: 006 TotalWalk: 0460  
178 2022-08-13 15:00 Total: 0067 MinRun: --:-- --:-- NTimesRun: --- TotalRun: ---- MinWalk: 15:05 15:39 NTimesWalk: 002 TotalWalk: 0067  
178 2022-08-13 16:00 Total: 0073 MinRun: --:-- --:-- NTimesRun: --- TotalRun: ---- MinWalk: 16:41 16:47 NTimesWalk: 002 TotalWalk: 0073  
178 2022-08-13 17:00 Total: 0045 MinRun: --:-- --:-- NTimesRun: --- TotalRun: ---- MinWalk: 17:25 17:29 NTimesWalk: 001 TotalWalk: 0045
```

El resumen del conocimiento obtenido sobre cómo conseguir información de los pasos dados:
 - El volcado de datos se solicita al reloj escribiendo en el handle `0x0010` el valor `0xb2fa`.
 - El volcado está compuesto por notificaciones con el formato:
	| - | Cabecera  | Año | Mes | Día | Hora | Total | MinRun1 | MinRun2 | NTimesRun | TotalRun | MinWalk1 | MinWalk2 | NTimesWalk| TotalWalk |
	| :-- | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | 
	| Hex |  b2 | 07e6 | 08 | 0d | 0c | 0973 | 21 | 2d | 0b | 0693 | 02 | 3a | 0c | 02e0 |
	| Dec | 178 | 2022 | 08 | 13 | 12 | 2419 | 33 | 45 | 11 | 1683 | 02 | 58 | 12 | 736 |

	Donde:
	- Cabecera: valor que indica que la notificación es parte de un volcado de datos de saturación de pasos.
	- Año, Mes y Día: fecha de la toma de los datos incluidos en la notificación.
	- Hora: hora de la toma de los datos incluidos en la notificación. P.ej: el valor 12 indica desde 12:00 hasta 12:59.
	- Total: total del paso dados en la hora. Es la suma de TotalRun y TotalWalk.
	- MinRun1: minuto en el que el usuario empezó a correr. P.ej: 33 significa que el usuario empezó a correr a las 12:33.
	- MinRun2: minuto en el que el usuario paró de correr. P.ej: 45 significa que el usuario paró de correr a las 12:45.
	- NTimesRun: número de veces que el usuario ha corrido.
	- TotalRun: total del paso dados corriendo en la hora.
	- MinWalk1: minuto en el que el usuario empezó a andar. P.ej: 02 significa que el usuario empezó a andar a las 12:02.
	- MinWalk2: minuto en el que el usuario paró de andar. P.ej: 58 significa que el usuario paró de andar a las 12:58.
	- NTimesWalk: número de veces que el usuario ha andado.
	- TotalWalk: total del paso dados andando en la hora.


## Desarrollar una aplicación Linux
Ahora que se conoce la forma de obtener los volcados de datos del reloj y que se conoce el significado de los diferentes campos de los volcados, es hora de ponerse mano a la obra.

El lenguaje de programación elegido en un primer momento fue [Python](https://www.python.org/) con [Qt](https://doc.qt.io/qtforpython/) para la creación de la interfaz gráfica. Elegí Python porque es fácil de escribir y es un lenguaje que se adapta bien a la creación de programas pequeños como este. Elegí Qt porque en una sola biblioteca aúna todas las funcionalidades que necesito: creación de interfaz gráfica, acceso a la pila Bluetooth y creación de gráficas. Lamentablemente PySide6, el módulo de Qt para Python, tenía un error en el acceso a las características BLE en el momento en el que empecé el desarrollo.
Esto me llevó a usar C++ y Qt.

La aplicación es muy sencilla y no la hice con el propósito de que fuera útil para otro usuario aparte de mí mismo. Quizás pueda ser útil para otros como ejemplo de uso de Qt para acceder a dispositivos Bluetooth Low Energy y para la creación de gráficas.

La aplicación tiene un panel lateral en el lado izquierdo donde muestra los dispositivos BLE encontrados. En el lado derecho tiene cuatro pestañas donde muestra los servicios y características del dispositivo BLE conectado. Si el dispositivo BLE conectado es un reloj Cubot N1 muestra las gráficas de frecuencia cardíaca, saturación de O<sub>2</sub> en sangre y pasos realizados.

Esto es una referencia a una parte del [código](https://github.com/eduardoposadas/recun1sw/blob/5f9f33e94ec222f80293979ff9997255e2eb8f12/recun1sw/mainwindow.cpp#L626) de la aplicación.

### Compilación de la aplicación
Instalación de dependencias para compilar:
```console
user@DESKTOP:~$ sudo apt install g++ libgl-dev qt6-base-dev qt6-connectivity-dev libqt6charts6-dev qt6-l10n-tools
```
Compilación:
```console
user@DESKTOP:~$ git clone https://github.com/eduardoposadas/recun1sw.git
user@DESKTOP:~$ cd recun1sw/recun1sw/
user@DESKTOP:~/recun1sw/recun1sw$ qmake6 && make -j $( grep -c processor /proc/cpuinfo )
```
## Referencias:

 - https://reverse-engineering-ble-devices.readthedocs.io/en/latest/
 - https://github.com/Freeyourgadget/Gadgetbridge/wiki/BT-Protocol-Reverse-Engineering

