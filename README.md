# Blackjack with Hi-Lo Counting

A console Blackjack simulator written in C++, with a built-in Hi-Lo card-counting trainer.

## Features

- Standard Blackjack rules: hit, stand, double down, split (up to 4 hands), insurance, and blackjack payouts
- Soft hand and split-ace handling
- Input validation for invalid or out-of-range entries
- **Hi-Lo counting trainer:** tracks the running count and true count, shows remaining card composition, and includes quizzes for practicing counting

## Design

- **Cards, Hand, Player:** structs for card data, hands (with bets and split state), and session statistics
- **CardCounter:** tracks the running count, true count, and cards seen per rank
- **Shoe and dealer logic:** shoe creation, shuffling (`std::mt19937`), dealing, and dealer play (hits soft 18) as standalone functions

## Build and Run

Requires a C++ compiler such as `g++`.

```bash
g++ -std=c++17 -Wall -o Blackjack *.cpp
./blackjack
```

On Windows, run `Blackjack.exe` instead of `./Blackjack`.
