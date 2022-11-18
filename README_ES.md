

# recun1sw

El 11/11/2021 cometí un error. Compré un reloj inteligente [Cubot N1](https://www.cubot.net/wearables/N1) para monitorizar mi frecuencia cardíaca mientras salía a correr. Tras unas semanas de uso me di cuenta de que las mediciones, por decirlo de una forma suave, son solamente aproximadas. Además, la aplicación para el teléfono móvil deja bastante que desear y no es de código abierto, por lo que no es posible saber dónde van a acabar mis datos personales.

Constado el error, decidí divertirme con el reloj de otra forma: hacer ingeniería inversa del protocolo de comunicación del reloj con la aplicación para el teléfono móvil. Una vez que supiera como el reloj pasa las mediciones a la aplicación para el teléfono móvil podría hacer una pequeña aplicación Linux que mostrase unas gráficas con las muestras tomadas por el reloj.

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
En el anterior teléfono que tenía, un Android 7, la forma más sencilla de capturar el tráfico Bluetooth fue activar en el teléfono las opciones para [desarrolladores](https://developer.android.com/studio/debug/dev-options), habilitar el registro de búsqueda de la Interfaz de Controlador (HCI) de host [Bluetooth](https://developer.android.com/studio/debug/dev-options#general) y examinar el fichero creado por el teléfono con [Wireshark](https://www.wireshark.org/). Para copiar el fichero `btsnoop_hci.log` del teléfono al ordenador utilicé [KDE Connect](https://kdeconnect.kde.org/) que viene instalado por defecto en [Kubuntu](https://kubuntu.org/), la distribución que utilizo. Un proceso directo que no fue complicado.

Con el nuevo teléfono que tengo ahora, un Android 10, el proceso no ha sido tan fácil. La única manera sencilla y rápida que he encontrado para conseguir una captura del tráfico Bluetooth ha sido:

 1. Instalar [Wireshark](https://www.wireshark.org/)  y [adb](https://developer.android.com/studio/command-line/adb) en el ordenador con `sudo apt install wireshark-qt adb`.
 2.  [Activar](https://developer.android.com/studio/debug/dev-options) en el teléfono las opciones para desarrolladores.
 3. [Habilitar](https://developer.android.com/studio/debug/dev-options#enable) la depuración por USB.
 4. [Habilitar](https://developer.android.com/studio/debug/dev-options#general) el registro de búsqueda de la Interfaz de Controlador (HCI) de host Bluetooth.   
 5. Conectar el teléfono al ordenador con un cable USB y comprobar que es accesible con `adb devices`.
    ```console
    user@DESKTOP:~$ adb devices
    * daemon not running; starting now at tcp:5037  
    * daemon started successfully  
    List of devices attached  
    CHISME10Plus18445 device
    ```
 6. Generar tráfico Bluetooth entre el teléfono y el reloj usando la App en el teléfono.
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
    Si no aparece este bloque de líneas hay que revisar que el Bluetooth del téléfono está encendido y que el registro de búsqueda de la Interfaz de Controlador (HCI) de host Bluetooth está habilitado, es decir el paso 4 de la lista anterior.

#### Análisis del tráfico Bluetooth
Al abrir el fichero `bt_capture.cfa` con Wireshark aparece algo como esto:

## Desarrollar una aplicación Linux 
## Referencias:

 - https://reverse-engineering-ble-devices.readthedocs.io/en/latest/
 - https://github.com/Freeyourgadget/Gadgetbridge/wiki/BT-Protocol-Reverse-Engineering

