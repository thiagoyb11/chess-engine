# Chess Engine

Motor de ajedrez experimental escrito en C++. El tablero se representa con
bitboards de 64 bits.

## Funcionalidades implementadas

- Representacion separada por color y tipo de pieza.
- Generacion de movimientos para peones, caballos, alfiles, torres, damas y
  reyes.
- Validacion de movimientos legales mediante deteccion de ataques al rey.
- Capturas normales y captura al paso.
- Enroque con seguimiento de derechos de enroque.
- Promociones de peon a dama, torre, alfil o caballo, incluyendo capturas y
  subpromociones.
- Carga de posiciones mediante FEN.
- Historial de posiciones y deshacer movimientos, incluidas promociones.
- Evaluacion material incremental.
- Generacion de movimientos con `Board::Move`, que conserva la pieza de
  promocion.
- Conteo `perft` secuencial y paralelo.
- Busqueda alpha-beta con:
  - evaluacion desde la perspectiva del jugador raiz;
  - poda alpha-beta;
  - seleccion del mejor movimiento en la raiz;
  - deteccion de jaque mate y ahogado cuando no quedan movimientos legales.

## Requisitos

- Compilador compatible con C++17, por ejemplo `g++`.

## Compilacion del motor

Desde la raiz del proyecto:

```bash
mkdir -p build
g++ -std=c++17 -O2 -Wall -Wextra \
  src/main.cpp src/search.cpp src/move_generation.cpp \
  src/board.cpp src/magic_bitboards.cpp src/zobrist_hashing.cpp \
  -o build/chess-engine
```

En Windows con MinGW, el ejecutable puede generarse como
`build/chess-engine.exe`.

## Ejecucion

```bash
./build/chess-engine
```

El programa principal ejecuta una busqueda alpha-beta, muestra su puntuacion y
el mejor movimiento encontrado, y despues ejecuta `parallelPerft` sobre la
posicion inicial.

## Pruebas perft

`src/perft-testing.cpp` contiene posiciones de prueba de perft conocidas.
Puede compilarse como un ejecutable independiente:

```bash
g++ -std=c++17 -O2 -Wall -Wextra \
  src/perft-testing.cpp src/move_generation.cpp \
  src/board.cpp src/magic_bitboards.cpp src/zobrist_hashing.cpp \
  -o build/perft-testing
```

## Estructura

```text
src/
├── board.h             # Interfaz y estado del tablero
├── board.cpp           # Estado, reglas y undo del tablero
├── magic_bitboards.h   # Interfaz de tablas de ataques magicos
├── magic_bitboards.cpp # Tablas y ataques de piezas deslizantes
├── move_generation.h   # API de movimientos y perft
├── move_generation.cpp # Movimientos legales, perft y parallelPerft
├── search.h            # API de busqueda alpha-beta
├── search.cpp          # Busqueda, evaluacion de hojas y mejor movimiento
├── perft-testing.cpp   # Suite de posiciones de prueba perft
├── generate_magics.cpp # Utilidad para generar datos de magic bitboards
└── main.cpp            # Punto de entrada del ejecutable principal
```

## Licencia

Este proyecto se distribuye bajo la licencia indicada en [LICENSE](LICENSE).
