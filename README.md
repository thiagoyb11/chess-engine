# Chess Engine

Motor de ajedrez experimental escrito en C++. El estado del tablero se representa con bitboards de 64 bits.

## Funcionalidades

- Generación de movimientos para peones, caballos, alfiles, torres, damas y reyes.
- Capturas, jaque, enroque y captura al paso.
- Carga de posiciones mediante FEN.
- Historial y deshacer movimientos.
- Conteo `perft` para probar la generación de movimientos.

## Requisitos

- Compilador compatible con C++17, por ejemplo `g++`.

## Compilación

Desde la raíz del proyecto:

```bash
mkdir -p build
g++ -std=c++17 -O2 -Wall -Wextra src/main.cpp src/board.cpp src/magic_bitboards.cpp -o build/chess-engine
```

En Windows con MinGW, el ejecutable puede generarse como `build/chess-engine.exe`.

## Ejecución

```bash
./build/chess-engine
```

El programa actual ejecuta `perft` sobre la posición inicial con profundidad 6 e imprime el número de nodos calculados.

## Estructura

```text
src/
├── board.h     # Interfaz y estado del tablero
├── board.cpp   # Reglas y generación de movimientos
└── main.cpp    # Punto de entrada y prueba perft
```

## Licencia

Este proyecto se distribuye bajo la licencia indicada en [LICENSE](LICENSE).
