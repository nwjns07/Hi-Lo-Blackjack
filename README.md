# Hi-Lo-Blackjack
Self-made Blackjack project, intended to be used as a "practice" for the Hi-Lo Counting system to better understand Blackjack hands and strategies.

# Blackjack with Hi-Lo Counting

A console Blackjack simulator written in C++, with a built-in Hi-Lo card-counting trainer.

## Features

- Standard Blackjack rules: hit, stand, double down, split (up to 4 hands), insurance, and blackjack payouts
- Soft hand and split-ace handling
- Input validation for invalid or out-of-range entries
- **Hi-Lo counting trainer:** tracks the running count and true count, shows remaining card composition, and includes quizzes for practicing counting

## Design

The code is split into independent classes, so each part of the game can be changed without affecting the others:

- **Shoe:** manages the decks, shuffling, and dealing
- **Hand:** hand values, soft totals, and split logic
- **Dealer:** dealer rules for when to hit or stand
- **Player statistics:** tracks results across rounds

## Build and Run

Requires a C++ compiler such as `g++`.

```bash
g++ -std=c++17 -Wall -o blackjack *.cpp
./blackjack
```

On Windows, run `blackjack.exe` instead of `./blackjack`.
