#pragma once

// Shared DLL contract. Only Success makes a calculation's result data valid.
enum class Status : int {
    Success = 0,
    InvalidParameter = 1,
    InvalidInput = 2,
    NotInitialized = 3,
    Unreachable = 4,
    NonUniqueSolution = 5,
    NumericalError = 6,
    NotImplemented = 7
};
