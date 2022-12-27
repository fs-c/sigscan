#pragma once

#include "signature.h"

#include <cstdint>

class Process;

class Module {
    const Process *process;
    uintptr_t base;
    unsigned int size;

public:
    Module(const Process *process, uintptr_t base, unsigned int size) : process(process), base(base),
        size(size) {};


    uintptr_t find_signature(const Signature&& signature) const;
};
