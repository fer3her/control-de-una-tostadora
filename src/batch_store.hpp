#pragma once

#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include "batch.hpp"

// Almacen de baches compartido por los dos hilos.
// Todos los metodos publicos toman el mutex, asi un solo hilo a la vez
// puede leer o modificar los datos.
class BatchStore {
public:
    // Crea un bache nuevo y devuelve su id.
    int create(double initialWeight) {
        // Toma el candado; se libera solo al salir de la funcion.
        std::lock_guard<std::mutex> lock(m_);

        Batch b;
        b.id = nextId_++;                // usa el id actual y luego suma 1
        b.initialWeight = initialWeight;
        b.finalWeight = std::nullopt;    // aun no se conoce

        batches_[b.id] = b;
        return b.id;
    }

    // Busca un bache por id. Devuelve nullopt si no existe.
    // Devuelve una COPIA: asi el otro hilo no puede modificarla mientras la usamos.
    std::optional<Batch> get(int id) const {
        std::lock_guard<std::mutex> lock(m_);

        auto it = batches_.find(id);
        if (it == batches_.end()) {
            return std::nullopt;
        }

        return it->second;
    }

    // Registra el peso final (cafe ya tostado).
    // Devuelve "" si todo salio bien, o un mensaje de error.
    std::string setFinalWeight(int id, double w) {
        std::lock_guard<std::mutex> lock(m_);

        auto it = batches_.find(id);
        if (it == batches_.end()) {
            return "error: lote no existe";
        }

        Batch& b = it->second;   // referencia (&): modifica el bache real, no una copia

        // Un bache cerrado ya no se puede modificar.
        if (b.state == BatchState::Finished ||
            b.state == BatchState::Cancelled) {
            return "error: lote cerrado";
        }

        // El cafe tostado pesa menos que el verde, nunca mas.
        if (w <= 0 || w > b.initialWeight) {
            return "error: peso final invalido";
        }

        b.finalWeight = w;
        return "";
    }

    // Cambia el estado de un bache si la transicion es valida.
    // Devuelve "" si todo salio bien, o un mensaje de error.
    std::string changeState(int id, BatchState next) {
        std::lock_guard<std::mutex> lock(m_);

        auto it = batches_.find(id);
        if (it == batches_.end()) {
            return "error: lote no existe";
        }

        Batch& b = it->second;

        // Solo se permiten los saltos de estado definidos en validTransition().
        if (!validTransition(b.state, next)) {
            return "transicion invalida: " +
                   std::string(toString(b.state)) + " -> " +
                   toString(next);
        }

        // No se puede terminar un bache sin saber cuanto peso quedo.
        if (next == BatchState::Finished &&
            !b.finalWeight.has_value()) {
            return "error: falta el peso final";
        }

        b.state = next;
        return "";
    }

    // Devuelve una copia de todos los baches en estado Finished.
    std::vector<Batch> finished() const {
        std::lock_guard<std::mutex> lock(m_);

        std::vector<Batch> result;
        for (const auto& par : batches_) {     // par.first = id, par.second = bache
            if (par.second.state == BatchState::Finished) {
                result.push_back(par.second);
            }
        }

        return result;
    }

    // Merma = porcentaje de peso perdido durante el tueste.
    // Devuelve nullopt si aun no hay peso final.
    // Es static y sin candado: solo usa la copia que recibe, no toca datos compartidos.
    static std::optional<double> shrinkPercent(const Batch& b) {
        if (!b.finalWeight.has_value() ||
            b.initialWeight <= 0) {            // evita dividir entre cero
            return std::nullopt;
        }

        return (b.initialWeight - b.finalWeight.value())
               / b.initialWeight * 100.0;
    }

private:
    // Define que cambios de estado estan permitidos.
    // No toma el mutex: solo la llaman metodos que ya lo tienen
    // (si lo tomara otra vez, el programa se congelaria).
    static bool validTransition(BatchState from, BatchState to) {
        switch (from) {
            case BatchState::Created:
                return to == BatchState::Roasting ||
                       to == BatchState::Cancelled;

            case BatchState::Roasting:
                return to == BatchState::Cooling ||
                       to == BatchState::Cancelled;

            case BatchState::Cooling:
                return to == BatchState::Finished ||
                       to == BatchState::Cancelled;

            // Estados finales: no se puede salir de ellos.
            case BatchState::Finished:
            case BatchState::Cancelled:
                return false;
        }

        return false;
    }

    std::map<int, Batch> batches_;   // id -> bache
    int nextId_ = 1;                 // siguiente id a asignar

    // "mutable" permite bloquearlo dentro de metodos const (get, finished).
    mutable std::mutex m_;
};