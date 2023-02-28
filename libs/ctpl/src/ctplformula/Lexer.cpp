//
// Created by mkk on 2/24/23.
//
#include "Tokens.cpp"
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
namespace CtlFormula {

    class CtlTokenizer {
    public:
        CtlTokenizer(std::string input) : input_(std::move(input)) {
            tokenize();
        }

        std::vector<Tokens::CtlTokens> getTokens() {
            return token_;
        }
    private:
        std::vector<Tokens::CtlTokens> token_;
        std::string input_;
        size_t pos_ = 0;
        const std::unordered_map<std::string, Tokens::CtlTokens> keywords_ = {
                {"true", Tokens::CtlTokens::True},
                {"false", Tokens::CtlTokens::False},
                {"and", Tokens::CtlTokens::And},
                {"not", Tokens::CtlTokens::Not},
                {"var", Tokens::CtlTokens::Var},
                {"X", Tokens::CtlTokens::X},
                {"U", Tokens::CtlTokens::U},
                {"F", Tokens::CtlTokens::F},
                {"G", Tokens::CtlTokens::G},
                {"E", Tokens::CtlTokens::E},
                {"A", Tokens::CtlTokens::A},
                {"(", Tokens::CtlTokens::leftParen},
                {")", Tokens::CtlTokens::rightParen}
        };

        void tokenize(){
            while(pos_ < input_.size()){
                char c = input_[pos_];
                if(isspace(c)) {
                    pos_++;
                }
                else if (c == '(') {
                    token_.push_back(Tokens::CtlTokens::leftParen);
                    std::vector<char> var;
                    while (c != ')') {
                        var.push_back(c);
                        pos_++;
                        c = input_[pos_];
                    }
                }
                else if (c == ')'){
                    token_.push_back(Tokens::CtlTokens::rightParen);
                    pos_++;
                }
                else if(c == 'true'){
                    token_.push_back(Tokens::CtlTokens::True);
                    pos_++;
                }
                else if(c == 'false'){
                    token_.push_back(Tokens::CtlTokens::False);
                    pos_++;
                }
                else if(c == 'not'){
                    token_.push_back(Tokens::CtlTokens::Not);
                    pos_++;
                }
                else if(c == 'and'){
                    token_.push_back(Tokens::CtlTokens::And);
                    pos_++;
                }
                else if(c == 'X'){
                    token_.push_back(Tokens::CtlTokens::X);
                    pos_++;
                }
                else if(c == 'U'){
                    token_.push_back(Tokens::CtlTokens::U);
                    pos_++;
                }
                else if(c == 'F'){
                    token_.push_back(Tokens::CtlTokens::F);
                    pos_++;
                }
                else if(c == 'G'){
                    token_.push_back(Tokens::CtlTokens::G);
                    pos_++;
                }
                else if(c == 'E'){
                    token_.push_back(Tokens::CtlTokens::E);
                    pos_++;
                }
                else if(c == 'A'){
                    token_.push_back(Tokens::CtlTokens::A);
                    pos_++;
                }
                else{
                    std::cout << "Error: Invalid token" << std::endl;
                    exit(1);
                }
            }
        }
    };


}