# Blackjack with Hi-Lo Counting

A console Blackjack simulator written in C++, with a built-in Hi-Lo card-counting trainer for practicing the counting system and understanding Blackjack hands and strategy.

## Features

- Blackjack with hit, stand, double down, split (up to 4 hands), insurance, and 3:2 blackjack payouts
- Soft hand handling, and split aces receive one card each
- Shoe of 4 to 7 decks, reshuffled when about 25% remains
- Input validation for bets, menu choices, and deck count
- **Hi-Lo counting trainer:**
  - Tracks the running count and true count (running count divided by decks remaining)
  - Shows remaining cards by rank, grouped into low, neutral, and high cards
  - Optional running-count quiz after each round
  - Optional on-screen count info and a bet-sizing guide based on the true count
- Session statistics: hands won, lost, and pushed, net gain, and quiz accuracy

## House Rules

- Dealer hits on soft 18 and stands on hard 18
- Blackjack pays 3:2, insurance pays 2:1
- The dealer's face-down card is not counted until it is revealed

## Design

The game is written in a single file (`Blackjack.cpp`), with data and logic organized as follows:

- **`Cards`, `Hand`, `Player`:** structs for card data, hands (bet and split state), and session statistics
- **`CardCounter`:** a struct with member functions that tracks the running count, cards seen per rank, decks remaining, and true count
- **Shoe and dealer logic:** shoe creation, shuffling (`std::mt19937`), dealing, and dealer play are standalone functions
- **Round flow:** `playRound`, `playerTurn`, and `resolveHand` handle betting, player decisions, and payouts

## Build and Run

Requires a C++17 compiler such as `g++`.

```bash
g++ -std=c++17 -Wall -Wextra -o blackjack Blackjack.cpp
./blackjack
```

On Windows, run `blackjack.exe` instead of `./blackjack`.

Eugene Magsino, Engineering Science student at Simon Fraser University
[Portfolio](https://nwjns07.github.io/nwjns07/)
