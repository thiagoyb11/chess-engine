# Chess Engine

Motor de ajedrez experimental escrito en C++. El estado del tablero se representa con bitboards de 64 bits.

## Funcionalidades

- Generacion de movimientos para peones, caballos, alfiles, torres, damas y reyes.
- Promociones de peones, incluyendo subpromociones.
- Capturas, jaque, enroque y captura al paso.
- Carga de posiciones mediante FEN.
- Historial y deshacer movimientos.
- Conteo `perft` para probar la generacion de movimientos.

## Requisitos

- Compilador compatible con C++17, por ejemplo `g++`.

## Compilacion

Desde la raiz del proyecto:

```bash
mkdir -p build
g++ -std=c++17 -O2 -Wall -Wextra src/main.cpp src/move_generation.cpp src/board.cpp src/magic_bitboards.cpp -o build/chess-engine
```

En Windows con MinGW, el ejecutable puede generarse como `build/chess-engine.exe`.

## Ejecucion

```bash
./build/chess-engine
```

El programa actual ejecuta `perft` sobre la posicion inicial e imprime el numero de nodos calculados.

## Estructura

```text
src/
├── board.h             # Interfaz y estado del tablero
├── board.cpp           # Estado y reglas de movimiento del tablero
├── move_generation.h   # API de movimientos y perft
├── move_generation.cpp # Generacion de movimientos y perft
└── main.cpp            # Punto de entrada
```

## Licencia

Este proyecto se distribuye bajo la licencia indicada en [LICENSE](LICENSE).
