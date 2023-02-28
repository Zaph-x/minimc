//
// Created by christofferaaen on 28/02/2023.
//
#include <string>
namespace Tokens {
    enum class CtlTokens {
        True, False, And, Not, Var, X, U, F, G, E, A, leftParen, rightParen
    };
    struct CtlToken {
        CtlTokens token;
        std::string value;
    };
}