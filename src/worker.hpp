#pragma once

#include <iomanip>      
#include <iostream>
#include <mutex>
#include <sstream>      // std::istringstream / ostringstream (leer y armar texto)
#include <string>

#include "batch_store.hpp"
#include "command_queue.hpp"

// Mutex solo para la consola: evita que las lineas de los dos hilos se mezclen.
inline std::mutex& coutMutex() {
    static std::mutex m;   // se crea una sola vez y lo comparten todos
    return m;
}

// Imprime un mensaje con el nombre del hilo, de forma segura entre hilos.
inline void say(const std::string& who, const std::string& msg) {
    std::lock_guard<std::mutex> lock(coutMutex());
    std::cout << "[" << who << "] " << msg << std::endl;
}

// Convierte un bache en una linea de texto legible.
inline std::string describe(const Batch& b) {
    std::ostringstream o;                    // como un cout, pero escribe en un string
    o << std::fixed << std::setprecision(2); // siempre 2 decimales ---std::setprecision (decimales al imprimir)
    o << "Bache #" << b.id << " | estado=" << toString(b.state)
      << " | inicial=" << b.initialWeight << " kg | final=";

    // Si hay peso final lo muestra; si no, un guion.
    if (b.finalWeight) o << *b.finalWeight << " kg";
    else               o << "-";

    // La merma solo existe si ya hay peso final.
    if (auto s = BatchStore::shrinkPercent(b)) o << " | merma=" << *s << " %";
    return o.str();
}

// Lo que ejecuta cada hilo (A o B):

inline void workerLoop(const std::string& name, BatchStore& store, CommandQueue& queue) {
    // pop() duerme hasta que llegue un comando. Devuelve nullopt cuando la cola
    // se cierra (quit),  bucle termina y el hilo acaba.
    while (auto line = queue.pop()) {
        std::istringstream in(*line);   // permite leer la linea palabra por palabra
        std::string cmd;
        in >> cmd;                      

        if (cmd == "create") {
            // Lee el peso; si no se pudo leer o no es positivo, muestra el uso.
            double w;
            if (!(in >> w) || w <= 0) { say(name, "uso: create <peso_kg>"); continue; }
            int id = store.create(w);
            say(name, "creado bache #" + std::to_string(id));

        } else if (cmd == "get") {
            int id;
            if (!(in >> id)) { say(name, "uso: get <id>"); continue; }
            auto b = store.get(id);
            say(name, b ? describe(*b) : "bache no existe");

        } else if (cmd == "state") {
            int id; std::string st;
            if (!(in >> id >> st)) { say(name, "uso: state <id> <estado>"); continue; }

            // Convierte el texto a estado; si no existe, avisa y sigue.
            auto ns = parseState(st);
            if (!ns) { say(name, "estado desconocido: " + st); continue; }

            // El almacen valida la transicion. "" = todo bien, otro texto = error.
            std::string err = store.changeState(id, *ns);
            say(name, err.empty() ? "bache #" + std::to_string(id) + " -> " + toString(*ns) : err);

        } else if (cmd == "final") {
            int id; double w;
            if (!(in >> id >> w)) { say(name, "uso: final <id> <peso_kg>"); continue; }
            std::string err = store.setFinalWeight(id, w);
            say(name, err.empty() ? "peso final registrado" : err);

        } else if (cmd == "finished") {
            auto v = store.finished();
            if (v.empty()) say(name, "no hay baches finalizados");
            for (const auto& b : v) say(name, describe(b));

        } else if (cmd == "shrink") {
            int id;
            if (!(in >> id)) { say(name, "uso: shrink <id>"); continue; }
            auto b = store.get(id);
            if (!b) { say(name, "bache no existe"); continue; }

            auto s = BatchStore::shrinkPercent(*b);   // vacio si aun no hay peso final
            std::ostringstream o;
            o << std::fixed << std::setprecision(2);
            if (s) o << "merma = " << *s << " %";
            else   o << "aun sin peso final";
            say(name, o.str());

        } else if (!cmd.empty()) {
            //cualquier otra cosa es un comando desconocido.
            say(name, "comando desconocido: " + cmd);
        }
    }
}