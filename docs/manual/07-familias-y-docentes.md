# Para familias y docentes

Este capítulo es para las personas adultas que acompañan a los peques. Resume qué es el aparato, cómo juega cada nivel y qué hace con los datos.

## Qué es

Un ajedrez con pantalla táctil que funciona **por sí solo**: se enciende y se juega, sin móvil, sin cuenta y sin conexión a internet. El programa no usa wifi ni envía nada a ningún sitio. Es software libre (GPL v3).

## Para qué sirve en casa o en el aula

- **Aprender las reglas** sin necesitar a alguien que sepa jugar: al tocar una pieza se muestran sus movimientos legales, y el juego **no permite jugadas ilegales**.
- **Jugar solo** a un ritmo adecuado: seis niveles de dificultad que van de «casi aleatorio» a «exigente».
- **Jugar en pareja** con un solo aparato, en rincón de juego, biblioteca o recreo: modo *Cara a cara* o *Pasar el aparato*, con reloj opcional para trabajar el turno y la espera.
- **Sin frustración:** *Deshacer* ilimitado, *Pista* y el aviso de pieza en peligro permiten equivocarse y corregir.

## Cómo juega cada nivel

Cada nivel combina tres ajustes: cuántas jugadas mira por delante el motor, cuánto tiempo puede pensar y con qué frecuencia hace una jugada **al azar** (un «despiste»).

| Nivel | Jugadas que mira | Tiempo máximo | Despistes | Tiempo mínimo de respuesta |
|---|:---:|:---:|:---:|:---:|
| Pollito | 1 | 0,3 s | 55 % | 0,8 s |
| Tortuga | 1 | 0,4 s | 25 % | 0,8 s |
| Conejo | 2 | 0,5 s | 10 % | 0,8 s |
| Zorro | 3 | 0,8 s | 3 % | — |
| Búho | 5 | 3 s | 0 % | — |
| Dragón | hasta 20 | 5 s | 0 % | — |

Notas:

- En la placa real, Zorro, Búho y Dragón **suelen agotar su tiempo máximo**, así que lo que determina su fuerza es el tiempo, más que la profundidad. Los tres niveles bajos contestan en milisegundos; por eso se les añade una pausa mínima de 0,8 s, para que se note que «piensan».
- Los porcentajes de despistes y los tiempos son valores ajustables en `src/game/difficulty.h`.

### Ayudas y su intención educativa

- **Puntos en las casillas legales:** el niño no tiene que memorizar los movimientos para empezar.
- **Pista:** marca una jugada recomendada; no la hace por el niño.
- **Aviso de pieza en peligro:** solo salta si el rival puede ganar material neto (al menos una pieza menor). No avisa por un peón suelto ni por cambios igualados. Se puede desactivar para quien ya no lo necesite.
- **Modo adaptable:** sube o baja el nivel hasta dos puntos según el material, de modo que la partida se mantenga **ni demasiado fácil ni demasiado difícil**. Puede desactivarse si prefieres que el nivel sea fijo.

## Datos personales

- El aparato **no recoge datos personales** ni se conecta a ninguna red.
- Los **nombres** son opcionales (hasta 10 caracteres). Se guardan **solo** en la memoria interna del aparato (NVS) y dentro de las partidas guardadas. Para borrarlos: dejar el nombre vacío y empezar una partida nueva, y borrar las partidas guardadas desde *Partidas › Borrar*.
- Conviene recordar a los niños que **no hace falta usar el nombre real**: un apodo vale igual.
- No hay publicidad, compras ni enlaces externos.

## Consejos de uso

- **Sesiones cortas:** una partida contra Pollito o Tortuga dura pocos minutos.
- **Primero sin reloj:** el reloj es para cuando ya dominan el movimiento de las piezas.
- **Hablar de las jugadas:** pregunta «¿qué quería hacer el rival?» cuando salte el aviso de peligro; es una buena ocasión para razonar.
- **Varios peques:** usa los tres huecos de *Partidas* para que cada uno guarde la suya.
- **En el aula:** en modo *Cara a cara* dos niños pueden jugar a la vez, y con el reloj de 3 o 5 minutos se organizan turnos rápidos.

## Accesibilidad y límites

- La interacción es **solo táctil** y con textos grandes; no hay sonido en esta versión.
- Las piezas se distinguen por su forma y por el color (blancas y negras), no solo por el color del fondo.
- No hay guardado automático: al apagar se pierde la partida no guardada.

## ¿Algo no va bien?

Mira el [anexo técnico](08-montaje.md), o abre una incidencia en el [repositorio](https://github.com/reasol-edu/esp32-chess/issues).
