#pragma once

#include <condition_variable>   
#include <mutex>
#include <optional>
#include <queue>
#include <string>


// El hilo principal (consola) pone comandos; el hilo trabajador los saca.
class CommandQueue {
public:
    // Agrega un comando a la cola (lo llama el hilo principal).
    void push(std::string cmd) {
        {   // estas llaves limitan el candado: se suelta antes de avisar
            std::lock_guard<std::mutex> lock(m_);
            q_.push(std::move(cmd));    // move: pasa el texto sin copiarlo
        }
        cv_.notify_one();               // "suena la campanita": despierta al hilo
    }

    // Saca el siguiente comando (lo llama el hilo trabajador).
    // Si no hay ninguno, el hilo DUERME hasta que llegue uno.
    // Devuelve nullopt si la cola se cerro es la senal para terminar
    std::optional<std::string> pop() {
        std::unique_lock<std::mutex> lock(m_);

        // Duerme ) hasta que haya un comando o se cierre la cola.
        cv_.wait(lock, [&] { return !q_.empty() || closed_; });

        // Si despertamos y sigue vacia, es porque la cerraron: no hay mas trabajo.
        if (q_.empty()) return std::nullopt;

        std::string c = std::move(q_.front());   // toma el primer comando
        q_.pop();                                // lo quita de la cola
        return c;
    }

    // Cierra la cola: avisa a los hilos que ya no vendran mas comandos.
    void close() {
        {
            std::lock_guard<std::mutex> lock(m_);
            closed_ = true;
        }
        cv_.notify_all();              // despierta al hilo para que termine
    }

private:
    std::mutex m_;                     // protege la cola
    std::condition_variable cv_;       // la "campanita" para despertar al hilo
    std::queue<std::string> q_;        // los comandos pendientes (primero en llegar, primero en salir)
    bool closed_ = false;              // true cuando ya no se aceptan mas comandos
};