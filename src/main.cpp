#include <functional>   
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include "batch_store.hpp"
#include "command_queue.hpp"
#include "worker.hpp"

// Muestra los comandos disponibles.

static void printHelp() {
    std::cout <<
       "Formato: <A|B> <comando> [args]   (A o B elige el hilo que ejecuta)\n"
        "  create <peso_inicial_kg>        crea un bache\n"
        "  get <id>                        consulta un bache\n"
        "  state <id> <estado>             Created|Roasting|Cooling|Finished|Cancelled\n"
        "  final <id> <peso_final_kg>      registra el peso final\n"
        "  finished                        lista baches finalizados\n"
        "  shrink <id>                     merma porcentual de un bache\n"
        "  help | quit\n";
}

int main() {
    BatchStore store;            // los baches: el unico dato compartido
    CommandQueue qA, qB;         // una cola de comandos por hilo

    // Crea los dos hilos trabajadores. Arrancan de inmediato y quedan
    // esperando comandos en su cola.
  
    std::thread tA(workerLoop, "Hilo A", std::ref(store), std::ref(qA));
    std::thread tB(workerLoop, "Hilo B", std::ref(store), std::ref(qB));

    printHelp();

    // El hilo principal solo lee la consola y reparte comandos.
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string target;
        in >> target;                    // primera palabra: A o B (o help / quit)

        if (target.empty()) continue;    // linea vacia:
        if (target == "quit") break;     // sale del bucle y empieza el cierre
        if (target == "help") { printHelp(); continue; }

        std::string rest;
        std::getline(in, rest);       

        // Segun el prefijo, el comando va a la cola de A o a la de B.
        if (target == "A" || target == "a") qA.push(rest);
        else if (target == "B" || target == "b") qB.push(rest);
        else std::cout << "Empiece con A o B (ej: A create 100)\n";
    }

    // Cierre ordenado:
    qA.close();     // avisa a los hilos que no vendran mas comandos
    qB.close();
    tA.join();      // espera a que cada hilo termine lo que estaba haciendo
    tB.join();
    return 0;
}