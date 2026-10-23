# luisa fernanda hernandez baena Prisma  (C++)

Simulación del control de una tostadora de café que trabaja por **baches**. El programa está compuesto por **dos hilos independientes** que funcionan al mismo tiempo y comparten los mismos datos sobre cada bache, sin interferir uno con el otro. Los eventos de creación, edición y consulta se inyectan por consola mediante comandos.

## Contexto del problema

Una tostadora transforma, en cada bache, una cantidad X de café verde en una cantidad Y de café tostado. Como el programa de control tiene dos hilos que trabajan simultáneamente, ambos necesitan leer y modificar la misma información de los baches. Si no se coordinan, pueden pisarse los datos (por ejemplo, asignar el mismo identificador a dos baches distintos o corromper la estructura donde se guardan). Este proyecto resuelve ese problema.

## Qué es un bache

Cada bache tiene:

| Campo | Descripción |

| Identificador | Número único, asignado automáticamente al crear el bache |
| Peso inicial | Kilos de café verde |
| Peso final | Kilos de café tostado. Es opcional porque no se conoce hasta que termina el tueste |
| Estado | `Created`, `Roasting`, `Cooling`, `Finished` o `Cancelled` |

### Reglas de los estados

Created --> Roasting --> Cooling --> Finished

Created, Roasting o Cooling --> Cancelled


- El flujo normal es `Created` -> `Roasting` -> `Cooling` -> `Finished`.
- `Cancelled` se puede aplicar desde `Created`, `Roasting` o `Cooling`.
- `Finished` y `Cancelled` son estados finales: no admiten más cambios.
- No se puede pasar a `Finished` sin haber registrado antes el peso final.
- El peso final debe ser mayor que 0 y no puede superar al peso inicial (el café tostado pesa menos que el verde porque pierde humedad).

### Merma porcentual

'''
merma % = (peso inicial - peso final) / peso inicial * 100
'''

Solo se puede calcular si el bache ya tiene peso final. Ejemplo: 150 kg de café verde que dan 85 kg de café tostado tienen una merma de 43.33 %.

## Requisitos

- Compilador con soporte de C++17 (desarrollado con g++ de MSYS2/MinGW en Windows)
- CMake 3.16 o superior

## Compilar y ejecutar

Desde la carpeta raíz del proyecto:
'''
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
.\build\roaster.exe
'''

## Uso

Cada comando sigue este formato:

'''
hilo  comando datos

'''

- `<hilo>`: `A` o `B`, el hilo que ejecuta el comando (también se acepta en minúscula).
- `<comando>`: la operación a realizar (ver la tabla de abajo).
- `<datos>`: los argumentos del comando, separados por espacios.

Ejemplo: `A create 150` hace que el hilo A cree un bache de 150 kg.

### Comandos disponibles

| Comando | Descripción |

| `create <peso_kg>` | Crea un bache nuevo con ese peso inicial |
| `get <id>` | Consulta un bache por su identificador |
| `state <id> <estado>` | Cambia el estado (`Created`, `Roasting`, `Cooling`, `Finished`, `Cancelled`) |
| `final <id> <peso_kg>` | Registra el peso final del bache |
| `finished` | Lista todos los baches finalizados |
| `shrink <id>` | Calcula la merma porcentual de un bache |
| `help` | Muestra la ayuda (sin prefijo de hilo) |
| `quit` | Termina el programa (sin prefijo de hilo) |

### Ejemplo de sesión

Las líneas que empiezan con `[Hilo ...]` son la respuesta del programa.


A create 150
[Hilo A] creado bache #1
B state 1 Roasting
[Hilo B] bache #1 -> Roasting
A final 1 85
[Hilo A] peso final registrado
B state 1 Cooling
[Hilo B] bache #1 -> Cooling
A state 1 Finished
[Hilo A] bache #1 -> Finished
B finished
[Hilo B] Bache #1 | estado=Finished | inicial=150.00 kg | final=85.00 kg | merma=43.33 %
A shrink 1
[Hilo A] merma = 43.33 %


El bache lo crea el hilo A, lo cambia de estado el hilo B y lo finaliza el hilo A. Los dos hilos trabajan sobre el mismo almacén de datos.

### Ejemplos de errores controlados

'''
A create 50
[Hilo A] creado bache #2
B state 2 Finished
[Hilo B] transicion invalida: Created -> Finished
A state 2 hola
[Hilo A] estado desconocido: hola
A final 2 80
[Hilo A] error: peso final invalido
B get 99
[Hilo B] bache no existe
'''


### Decisiones de diseño

- **Un único dato compartido, `BatchStore`.** Toda la información de los baches vive en un solo lugar. Los hilos nunca acceden directamente al contenedor interno, solo a través de los métodos públicos de la clase.
- **Exclusión mutua con `std::mutex`.** Cada método público del `BatchStore` toma un `std::lock_guard` al comenzar, así que solo un hilo a la vez puede ejecutar una operación. El `lock_guard` libera el candado automáticamente al terminar la función, incluso si hay un `return` temprano.
- **Las lecturas devuelven copias, no referencias.** `get` y `finished` devuelven los baches por valor. Si devolvieran una referencia, el candado se liberaría al salir del método y el otro hilo podría modificar ese bache mientras el primero aún lo lee.

- **Valores opcionales** El peso final y la merma pueden no existir todavía; `std::optional` lo expresa de forma explícita, en vez de usar un valor falso como 0.
- **Salida de consola serializada.** Un mutex adicional protege `std::cout` para que las líneas de los dos hilos no se mezclen.
- **Cierre ordenado.** Al escribir `quit`, el hilo principal cierra las colas, lo que despierta a los hilos para que terminen su bucle.

## Estructura del proyecto

'''
src/
  batch.hpp          Modelo de datos: Batch, BatchState y conversiones de estado
  batch_store.hpp    Almacén de baches con reglas de negocio y mutex
  command_queue.hpp  Cola bloqueante de comandos (mutex + condition_variable)
  worker.hpp         Bucle de cada hilo e interpretación de los comandos
  main.cpp           Lectura de consola y arranque de los hilos
CMakeLists.txt       Configuración de la compilación
'''

## Nota sobre concurrencia

'''
Como los hilos A y B corren en paralelo, si se escriben varios comandos dependientes muy seguidos (por ejemplo, cambiar el estado de un bache que acaba de crearse), el orden en que los ejecutan los dos hilos no está garantizado. 

Es el comportamiento esperado de la concurrencia real. Para comprobar el flujo paso a paso, conviene escribir los comandos de uno en uno
