#pragma once

#include <cctype>      //  (pasar texto a minusculas)
#include <optional>    //  (valores que pueden no existir)
#include <string>      

// Los estados posibles de un bache.
// "enum class" evita escribir un estado que no existe.
enum class BatchState {
    Created,
    Roasting,
    Cooling,
    Finished,
    Cancelled
};

// Un bache de tueste: cuanto cafe entro, cuanto salio y en que estado va.
struct Batch {
    int id = 0;                          // identificador unico valores unicales 
    double initialWeight = 0.0;          // kg de cafe verde
    std::optional<double> finalWeight;   // kg de cafe tostado; vacio hasta que termina
    BatchState state = BatchState::Created;
};

// Convierte un estado a texto, para poder imprimirlo.
// "inline" permite definirla en un .hpp que se incluye desde varios archivos.
inline const char* toString(BatchState s) {
    switch (s) {
        case BatchState::Created:   return "Created";
        case BatchState::Roasting:  return "Roasting";
        case BatchState::Cooling:   return "Cooling";
        case BatchState::Finished:  return "Finished";
        case BatchState::Cancelled: return "Cancelled";
    }

    return "Unknown";   // por seguridad, si llegara un valor inesperado
}

// convierte texto ("roasting", "Roasting", "ROASTING") a un estado.
// Devuelve nullopt si el texto no corresponde a ningun estado.
inline std::optional<BatchState> parseState(std::string s) {
    // Pasa todo a minusculas para que no importe como lo escriba el usuario.
    for (auto& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    if (s == "created")   return BatchState::Created;
    if (s == "roasting")  return BatchState::Roasting;
    if (s == "cooling")   return BatchState::Cooling;
    if (s == "finished")  return BatchState::Finished;
    if (s == "cancelled") return BatchState::Cancelled;

    return std::nullopt;   // texto desconocido
}