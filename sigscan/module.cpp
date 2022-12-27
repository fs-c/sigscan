#include "module.h"

#include "process.h"

#include <format>
#include <exception>

uintptr_t Module::find_signature(const Signature &&signature) const {
    auto buffer = std::vector<uint8_t>(size);

    if (!process->read_memory<uint8_t>(base, buffer.data(), buffer.size())) {
        throw std::runtime_error(std::format("couldn't read module memory at {} ({})", base, size));
    }

    const auto offset = signature.scan(buffer);

    if (!offset) {
        throw std::runtime_error("couldn't find signature");
    }

    return base + offset;
}
