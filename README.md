#Ping-Pong
=============

##Proyecto comunicación de datos

Antes de compilar es necesario tener la libreria ncurses


para instalar en linux, ejecuta 

```
sudo apt-get install libncurses5-dev libncursesw5-dev
```



Para compilar, ve al directorio del archivo y ejecuta:

      make all

Para ejecutar el servidor       

      ```
      ./server 7777
      ```

Para ejecutar los clientes 
      
      ```
      ./pong 127.0.0.1 7777
      ```
