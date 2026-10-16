// Blackjack with a Hi-Lo Card Counting Trainer, made in C++
// Original game by Eugene Magsino
//
// Build:  g++ -std=c++17 -Wall -Wextra -o blackjack blackjack.cpp
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif
using namespace std;

const double RESHUFFLE_THRESHOLD = 0.25;  // reshuffle when 25% of the shoe is left
const size_t MAX_HANDS = 4;               // max hands after re-splitting
const int CARDS_PER_DECK = 52;

// When true, printCard also shows each card's Hi-Lo tag (set by the "show count" option)
static bool showTags = false;

// -------- SMALL HELPERS --------

string fmt(double v, int prec = 2) {
    ostringstream o;
    o << fixed << setprecision(prec) << v;
    return o.str();
}

string signedInt(int v) { return (v > 0 ? "+" : "") + to_string(v); }

string signedFmt(double v, int prec = 1) {
    if (fabs(v) < 0.5 * pow(10.0, -prec)) v = 0;  // avoid printing "-0.0"
    return (v > 0 ? "+" : "") + fmt(v, prec);
}

string pad(const string& s, size_t w) {
    return s.size() >= w ? s : s + string(w - s.size(), ' ');
}

string readToken() {
    string s;
    if (!(cin >> s)) {
        cout << "\nInput closed. Goodbye!\n";
        exit(0);
    }
    return s;
}

char readChoice() {
    return static_cast<char>(toupper(static_cast<unsigned char>(readToken()[0])));
}

bool askYesNo(const string& prompt) {
    while (true) {
        cout << prompt << " (Y/N): ";
        char c = readChoice();
        if (c == 'Y') return true;
        if (c == 'N') return false;
        cout << "Invalid input. Please enter Y or N.\n";
    }
}

bool parseInt(const string& s, int& out) {
    try {
        size_t pos;
        int v = stoi(s, &pos);
        if (pos != s.size()) return false;
        out = v;
        return true;
    } catch (...) {
        return false;
    }
}

// Display Instructions
void displayRules() {

    // Welcome Message
    cout << "====================================" << endl;
    cout << "        WELCOME TO BLACKJACK" << endl;
    cout << "====================================" << endl << endl;

    // Game Objective
    cout << "Blackjack is a popular card game where the goal is to have a hand value as close to 21 as possible without exceeding it." << endl;
    cout << "You will be playing against the dealer." << endl << endl;

    // House Rules
    cout << "====================================" << endl;
    cout << "           House Rules" << endl;
    cout << "====================================" << endl << endl;

    cout << "1. Number of Decks: The game uses 4 to 7 standard 52-card decks." << endl;
    cout << "2. Cards are shuffled together in a shoe. The shoe is reshuffled when about 25% of it remains." << endl;
    cout << "3. Dealer hits on a soft 18, and stands on a hard 18." << endl;
    cout << "4. Blackjack pays 3:2. Insurance pays 2:1." << endl;
    cout << "5. Pushes result in no win or loss." << endl;
    cout << "6. You may split any two cards of equal value (up to 4 hands). Split Aces receive one card each." << endl << endl;

    // How to Play
    cout << "====================================" << endl;
    cout << "           How to Play" << endl;
    cout << "====================================" << endl << endl;

    cout << "1. Each player is dealt two cards, and the dealer receives one card face up and one card face down." << endl;
    cout << "2. Number Cards (2-10) are worth their face value." << endl;
    cout << "3. Face Cards (Jack, Queen, King) are worth 10." << endl;
    cout << "4. Aces can be worth 1 or 11, depending on which value benefits the hand more." << endl;
    cout << "5. Players can choose to 'Hit' (take another card) or 'Stand' (keep their current hand)." << endl;
    cout << "6. Players can also choose to 'Double Down' (double their bet and receive one final card) \n"
         << "or 'Split' (if they have two cards of the same value, they can split them into two separate hands)." << endl;
    cout << "7. After all players have completed their hands, the dealer reveals their face-down card and plays their hand according to house rules." << endl;
    cout << "8. The player with a hand value closest to 21 without exceeding it wins the round." << endl << endl;

    // Hi-Lo Trainer
    cout << "====================================" << endl;
    cout << "      Hi-Lo Counting (Educational)" << endl;
    cout << "====================================" << endl << endl;

    cout << "Hi-Lo assigns every card a tag:  2-6 = +1   |   7-9 = 0   |   10, J, Q, K, A = -1" << endl;
    cout << "Running Count = sum of the tags of all cards you have SEEN since the last shuffle." << endl;
    cout << "True Count    = Running Count / decks remaining (this is what actually measures your edge)." << endl;
    cout << "A high positive count means the shoe is rich in tens and aces, which favors the player." << endl;
    cout << "The dealer's face-down card is not counted until it is revealed." << endl;
    cout << "Type C at any prompt (bet or action) to view the full count breakdown." << endl << endl;

    cout << "Good luck and have fun playing Blackjack!" << endl << endl;
}

// -------- CARD LOGIC --------

// Card Data Structure
struct Cards {
    string rank;
    string suit;
    int value;
};

const vector<string> RANKS = {"A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"};

// Card Deck
vector<Cards> createDeck() {
    vector<Cards> deck;
    vector<string> suits = {"♥", "♦", "♣", "♠"};

    for (const string& suit : suits) {
        for (const string& rank : RANKS) {
            Cards card;
            card.rank = rank;
            card.suit = suit;

            if (rank == "A") card.value = 11;
            else if (rank == "J" || rank == "Q" || rank == "K") card.value = 10;
            else card.value = stoi(rank);

            deck.push_back(card);
        }
    }
    return deck;
}

// Shoe Creation
vector<Cards> createShoe(int numDecks) {
    vector<Cards> shoe;
    for (int i = 0; i < numDecks; i++) {
        vector<Cards> deck = createDeck();
        shoe.insert(shoe.end(), deck.begin(), deck.end());
    }
    return shoe;
}

// Shoe Shuffle
void shuffleShoe(vector<Cards>& shoe) {
    static mt19937 rng(random_device{}());
    shuffle(shoe.begin(), shoe.end(), rng);
}

// -------- HI-LO CARD COUNTING --------

// Hi-Lo tag for a rank: 2-6 = +1, 7-9 = 0, 10/J/Q/K/A = -1
int tagForRank(const string& rank) {
    if (rank == "2" || rank == "3" || rank == "4" || rank == "5" || rank == "6") return 1;
    if (rank == "7" || rank == "8" || rank == "9") return 0;
    return -1;
}

int hiLoValue(const Cards& card) { return tagForRank(card.rank); }

struct CardCounter {
    int numDecks = 0;
    int runningCount = 0;
    int cardsSeen = 0;
    map<string, int> seenByRank;

    void reset(int decks) {
        numDecks = decks;
        runningCount = 0;
        cardsSeen = 0;
        seenByRank.clear();
        for (const string& r : RANKS) seenByRank[r] = 0;
    }

    // Call this for every card the player can actually see
    void observe(const Cards& card) {
        runningCount += hiLoValue(card);
        cardsSeen++;
        seenByRank[card.rank]++;
    }

    int totalCards() const { return numDecks * CARDS_PER_DECK; }
    int cardsRemaining() const { return totalCards() - cardsSeen; }
    double decksRemaining() const { return cardsRemaining() / static_cast<double>(CARDS_PER_DECK); }

    double trueCount() const {
        double d = max(decksRemaining(), 0.25);  // guard against dividing by ~0
        return runningCount / d;
    }

    int remainingOfRank(const string& rank) const { return numDecks * 4 - seenByRank.at(rank); }
};

// Educational bet-ramp suggestion based on the true count
string betAdvice(double trueCount) {
    int tc = static_cast<int>(floor(trueCount));
    if (tc >= 5) return "True count +5 or higher: strongly player-favorable (typical ramp ~8x base bet).";
    if (tc == 4) return "True count +4: favorable (typical ramp ~6x base bet).";
    if (tc == 3) return "True count +3: favorable (typical ramp ~4x base bet).";
    if (tc == 2) return "True count +2: slightly favorable (typical ramp ~2x base bet).";
    return "True count +1 or lower: no player edge. Stick to your minimum/base bet.";
}

void printCountSummary(const CardCounter& c) {
    cout << "[Hi-Lo] Running: " << signedInt(c.runningCount)
         << " | True: " << signedFmt(c.trueCount())
         << " | Decks left: " << fmt(c.decksRemaining(), 1)
         << " | Cards seen: " << c.cardsSeen << "/" << c.totalCards() << "\n";
}

void printCountDetails(const CardCounter& c) {
    cout << "\n=========== HI-LO COUNT ===========\n";
    printCountSummary(c);

    cout << "\nRank  Tag  Left  (each rank starts at " << c.numDecks * 4 << ")\n";
    int low = 0, neutral = 0, high = 0;
    for (const string& r : RANKS) {
        int left = max(0, c.remainingOfRank(r));
        int tag = tagForRank(r);
        if (tag > 0) low += left;
        else if (tag < 0) high += left;
        else neutral += left;
        cout << pad(r, 6) << pad(signedInt(tag), 5) << pad(to_string(left), 6)
             << string(static_cast<size_t>(left), '#') << "\n";
    }

    int total = c.cardsRemaining();
    auto pct = [&](int n) { return total > 0 ? 100.0 * n / total : 0.0; };
    cout << "\nRemaining cards by group (fresh shoe: 38.5% / 23.1% / 38.5%):\n";
    cout << "  Low     (2-6,  +1): " << pad(to_string(low), 4) << fmt(pct(low), 1) << "%\n";
    cout << "  Neutral (7-9,   0): " << pad(to_string(neutral), 4) << fmt(pct(neutral), 1) << "%\n";
    cout << "  High (10-A,    -1): " << pad(to_string(high), 4) << fmt(pct(high), 1) << "%\n";

    if (high > low) cout << "\nThe remaining shoe is rich in high cards (favors the player).\n";
    else if (low > high) cout << "\nThe remaining shoe is rich in low cards (favors the dealer).\n";
    else cout << "\nThe remaining shoe is balanced.\n";

    cout << "Bet guide: " << betAdvice(c.trueCount()) << "\n";
    cout << "(The dealer's face-down card counts as unseen until it is revealed.)\n";
    cout << "===================================\n";
}

// -------- DEALING --------

// Card Dealing (no counting, used for the dealer's hidden card)
Cards dealCard(vector<Cards>& shoe) {
    Cards card = shoe.back();
    shoe.pop_back();
    return card;
}

// Deal a face-up card: the player sees it, so the counter observes it
Cards dealVisible(vector<Cards>& shoe, CardCounter& counter) {
    Cards card = dealCard(shoe);
    counter.observe(card);
    return card;
}

// Card Printing
void printCard(const Cards& card) {
    cout << card.rank << " of " << card.suit << " (Value: " << card.value;
    if (showTags) cout << ", Hi-Lo: " << signedInt(hiLoValue(card));
    cout << ")" << endl;
}

// -------- HAND LOGIC --------

struct Hand {
    vector<Cards> cards;
    double bet = 0.0;
    bool fromSplit = false;
};

int calculateHandValue(const Hand& hand) {
    int totalValue = 0;
    int numOfAces = 0;

    for (const Cards& card : hand.cards) {
        totalValue += card.value;
        if (card.rank == "A") numOfAces++;
    }

    while (totalValue > 21 && numOfAces > 0) {
        totalValue -= 10;
        numOfAces--;
    }
    return totalValue;
}

// A hand is soft if an Ace is still being counted as 11
bool isSoftHand(const Hand& hand) {
    int total = 0;
    int aceCount = 0;
    for (const Cards& card : hand.cards) {
        total += card.value;
        if (card.rank == "A") aceCount++;
    }
    while (total > 21 && aceCount > 0) {
        total -= 10;
        aceCount--;
    }
    return aceCount > 0;
}

bool isBust(const Hand& hand) { return calculateHandValue(hand) > 21; }

// A natural blackjack only counts on an original two-card hand (not after a split)
bool isBlackjack(const Hand& hand) {
    return !hand.fromSplit && hand.cards.size() == 2 && calculateHandValue(hand) == 21;
}

void printHand(const Hand& hand) {
    for (const Cards& card : hand.cards) {
        printCard(card);
    }
    cout << "Hand Value: " << calculateHandValue(hand) << (isSoftHand(hand) ? " (soft)" : "") << endl;
}

void printDealerUpCard(const Hand& dealerHand) {
    cout << "Dealer's Hand:\n";
    printCard(dealerHand.cards[0]);
    cout << "[Hidden Card]\n";
}

// -------- PLAYER LOGIC AND STATS --------
struct Player {
    double balance = 1000.0;
    double netGain = 0.0;
    int handsPlayed = 0;
    int handsWon = 0;
    int handsLost = 0;
    int handsPushed = 0;
    int quizAsked = 0;
    int quizCorrect = 0;
};

// -------- DEALER LOGIC --------
void dealerPlay(Hand& dealerHand, vector<Cards>& shoe, CardCounter& counter) {
    while (true) {
        int value = calculateHandValue(dealerHand);
        bool soft = isSoftHand(dealerHand);

        if (value < 18 || (value == 18 && soft))
            dealerHand.cards.push_back(dealVisible(shoe, counter));
        else break;
    }
}

// -------- GAME LOGIC --------
// The bet is deducted from the balance when placed, so winnings return stake + profit.
void resolveHand(Player& player, const Hand& hand, const Hand& dealerHand, const string& label) {
    int playerValue = calculateHandValue(hand);
    int dealerValue = calculateHandValue(dealerHand);

    player.handsPlayed++;

    if (playerValue > 21) {
        player.netGain -= hand.bet;
        player.handsLost++;
        cout << label << "Player busts. Lose $" << fmt(hand.bet) << ".\n";
    }
    else if (dealerValue > 21 || playerValue > dealerValue) {
        player.balance += hand.bet * 2;
        player.netGain += hand.bet;
        player.handsWon++;
        cout << label << "Player wins $" << fmt(hand.bet) << ".\n";
    }
    else if (playerValue < dealerValue) {
        player.netGain -= hand.bet;
        player.handsLost++;
        cout << label << "Dealer wins. Lose $" << fmt(hand.bet) << ".\n";
    }
    else {
        player.balance += hand.bet;
        player.handsPushed++;
        cout << label << "Push.\n";
    }
}

// -------- INSURANCE --------
// Returns the insurance amount placed (0 if declined / unavailable)
double offerInsurance(Player& player, double bet) {
    double insuranceBet = bet / 2;
    if (player.balance < insuranceBet) return 0.0;

    cout << "\nDealer shows an Ace.\n";
    if (askYesNo("Take insurance?")) {
        player.balance -= insuranceBet;
        cout << "Insurance bet placed: $" << fmt(insuranceBet) << endl;
        return insuranceBet;
    }
    return 0.0;
}

// -------- PLAYER OPTIONS --------
void playerTurn(vector<Hand>& hands, const Hand& dealerHand, vector<Cards>& shoe,
                CardCounter& counter, Player& player, bool showCount) {

    for (size_t i = 0; i < hands.size(); i++) {
        while (true) {
            cout << "\nDealer shows:\n";
            printDealerUpCard(dealerHand);

            if (hands.size() > 1)
                cout << "\n--- Hand " << i + 1 << " of " << hands.size() << " (bet $" << fmt(hands[i].bet) << ") ---\n";
            else
                cout << "\nYour hand (bet $" << fmt(hands[i].bet) << "):\n";
            printHand(hands[i]);

            if (showCount) printCountSummary(counter);

            if (isBust(hands[i])) {
                cout << "Bust!\n";
                break;
            }
            if (calculateHandValue(hands[i]) == 21) {
                cout << "21 - standing.\n";
                break;
            }
            if (hands[i].fromSplit && hands[i].cards[0].rank == "A") {
                cout << "Split Aces receive one card only.\n";
                break;
            }

            bool twoCards = hands[i].cards.size() == 2;
            bool canDouble = twoCards && player.balance >= hands[i].bet;
            bool canSplit = twoCards &&
                            hands[i].cards[0].value == hands[i].cards[1].value &&
                            player.balance >= hands[i].bet &&
                            hands.size() < MAX_HANDS;

            cout << "\nChoose action: (H)it, (S)tand";
            if (canDouble) cout << ", (D)ouble Down";
            if (canSplit) cout << ", S(P)lit";
            cout << ", (C)ount";
            cout << "\n> ";

            char choice = readChoice();

            if (choice == 'H') {
                hands[i].cards.push_back(dealVisible(shoe, counter));
            }
            else if (choice == 'S') {
                break;
            }
            else if (choice == 'D' && canDouble) {
                player.balance -= hands[i].bet;
                hands[i].bet *= 2;
                hands[i].cards.push_back(dealVisible(shoe, counter));
                cout << "\nDoubled down. You drew: ";
                printCard(hands[i].cards.back());
                cout << "Final hand value: " << calculateHandValue(hands[i]) << endl;
                break;
            }
            else if (choice == 'P' && canSplit) {
                player.balance -= hands[i].bet;

                Hand newHand;
                newHand.bet = hands[i].bet;
                newHand.fromSplit = true;
                hands[i].fromSplit = true;

                newHand.cards.push_back(hands[i].cards.back());
                hands[i].cards.pop_back();

                hands[i].cards.push_back(dealVisible(shoe, counter));
                newHand.cards.push_back(dealVisible(shoe, counter));

                hands.insert(hands.begin() + static_cast<long>(i) + 1, newHand);
                // loop again to play the (now updated) hand i
            }
            else if (choice == 'C') {
                printCountDetails(counter);
            }
            else {
                cout << "Invalid choice.\n";
            }
        }
    }
}

// -------- BETTING --------
double readBet(const Player& player, const CardCounter& counter, bool showCount) {
    while (true) {
        cout << "\nBalance: $" << fmt(player.balance) << endl;
        if (showCount) {
            printCountSummary(counter);
            cout << "Bet guide: " << betAdvice(counter.trueCount()) << "\n";
        }
        cout << "Enter bet (or C to view count): ";
        string s = readToken();

        if (s == "c" || s == "C") {
            printCountDetails(counter);
            continue;
        }

        try {
            size_t pos;
            double bet = stod(s, &pos);
            if (pos == s.size() && bet >= 0.01 && bet <= player.balance) return bet;
        } catch (...) {
        }
        cout << "Invalid bet.\n";
    }
}

// -------- COUNT QUIZ --------
void runningCountQuiz(Player& player, const CardCounter& counter) {
    cout << "\nCount check! What is the running count? (number, or S to skip): ";
    string s = readToken();
    int guess;
    if (!parseInt(s, guess)) {
        cout << "Skipped. The running count is " << signedInt(counter.runningCount) << ".\n";
        return;
    }
    player.quizAsked++;
    if (guess == counter.runningCount) {
        player.quizCorrect++;
        cout << "Correct! Running count is " << signedInt(counter.runningCount) << ".\n";
    } else {
        cout << "Not quite. You said " << signedInt(guess) << ", the actual running count is "
             << signedInt(counter.runningCount) << ".\n";
    }
    cout << "True count is " << signedFmt(counter.trueCount()) << " ("
         << fmt(counter.decksRemaining(), 1) << " decks left).\n";
}

// -------- SHOE MANAGEMENT --------
void newShoe(vector<Cards>& shoe, CardCounter& counter, int numDecks) {
    shoe = createShoe(numDecks);
    shuffleShoe(shoe);
    counter.reset(numDecks);
    cout << "\n*** New shoe shuffled - running count reset to 0 ***\n";
}

// -------- ONE ROUND --------
void playRound(Player& player, vector<Cards>& shoe, CardCounter& counter, bool showCount, bool quiz) {

    double bet = readBet(player, counter, showCount);
    player.balance -= bet;

    vector<Hand> hands(1);
    hands[0].bet = bet;
    Hand dealerHand;

    // Deal order: player, dealer (up), player, dealer (hole - NOT counted yet)
    hands[0].cards.push_back(dealVisible(shoe, counter));
    dealerHand.cards.push_back(dealVisible(shoe, counter));
    hands[0].cards.push_back(dealVisible(shoe, counter));
    dealerHand.cards.push_back(dealCard(shoe));

    cout << "\nDealer shows:\n";
    printDealerUpCard(dealerHand);
    cout << "\nYour hand:\n";
    printHand(hands[0]);

    double insuranceBet = 0.0;
    if (dealerHand.cards[0].rank == "A")
        insuranceBet = offerInsurance(player, bet);

    bool playerBJ = isBlackjack(hands[0]);
    bool dealerBJ = isBlackjack(dealerHand);

    if (insuranceBet > 0) {
        if (dealerBJ) {
            player.balance += insuranceBet * 3;  // stake back + 2:1
            player.netGain += insuranceBet * 2;
            cout << "Insurance pays 2:1 (+$" << fmt(insuranceBet * 2) << ").\n";
        } else {
            player.netGain -= insuranceBet;
            cout << "Dealer does not have Blackjack. Insurance lost ($" << fmt(insuranceBet) << ").\n";
        }
    }

    if (playerBJ || dealerBJ) {
        counter.observe(dealerHand.cards[1]);  // hole card revealed
        cout << "\nDealer's Hand:\n";
        printHand(dealerHand);

        player.handsPlayed++;
        if (playerBJ && dealerBJ) {
            player.balance += bet;
            player.handsPushed++;
            cout << "Push.\n";
        }
        else if (playerBJ) {
            player.balance += bet * 2.5;
            player.netGain += bet * 1.5;
            player.handsWon++;
            cout << "Blackjack! Pays 3:2 (+$" << fmt(bet * 1.5) << ").\n";
        }
        else {
            player.netGain -= bet;
            player.handsLost++;
            cout << "Dealer Blackjack. Lose $" << fmt(bet) << ".\n";
        }
    }
    else {
        playerTurn(hands, dealerHand, shoe, counter, player, showCount);

        bool anyLive = false;
        for (const Hand& h : hands)
            if (!isBust(h)) anyLive = true;

        if (anyLive) dealerPlay(dealerHand, shoe, counter);

        counter.observe(dealerHand.cards[1]);  // hole card revealed at end of round
        cout << "\nDealer's Hand:\n";
        printHand(dealerHand);
        cout << endl;

        for (size_t i = 0; i < hands.size(); i++) {
            string label = hands.size() > 1 ? "Hand " + to_string(i + 1) + ": " : "";
            resolveHand(player, hands[i], dealerHand, label);
        }
    }

    if (quiz) runningCountQuiz(player, counter);
    if (showCount) {
        cout << "\n";
        printCountSummary(counter);
    }

    cout << "\nStats:";
    cout << "\nHands Played: " << player.handsPlayed;
    cout << "\nWon: " << player.handsWon;
    cout << "\nLost: " << player.handsLost;
    cout << "\nPush: " << player.handsPushed;
    cout << "\nNet Gain: " << (player.netGain < 0 ? "-$" : "$") << fmt(fabs(player.netGain));
    if (player.quizAsked > 0)
        cout << "\nCount Quiz: " << player.quizCorrect << "/" << player.quizAsked << " correct";
    cout << endl;
}

// -------- MAIN FUNCTION --------
int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);  // so the suit symbols display correctly
#endif

    displayRules();
    bool playAgain = true;

    while (playAgain) {

        Player player;

        int numDecks;
        while (true) {
            cout << "Enter number of decks to use (4-7): ";
            if (parseInt(readToken(), numDecks) && numDecks >= 4 && numDecks <= 7) break;
            cout << "Please enter a number between 4 and 7.\n";
        }

        bool showCount = askYesNo("Show Hi-Lo count info automatically (training wheels)?");
        bool quiz = askYesNo("Quiz me on the running count after each round?");
        showTags = showCount;

        vector<Cards> shoe;
        CardCounter counter;
        newShoe(shoe, counter, numDecks);

        while (player.balance >= 0.01) {

            if (static_cast<double>(shoe.size()) < counter.totalCards() * RESHUFFLE_THRESHOLD)
                newShoe(shoe, counter, numDecks);

            playRound(player, shoe, counter, showCount, quiz);

            if (player.balance < 0.01) {
                cout << "\nYou're out of money!\n";
                break;
            }
            if (!askYesNo("\nPlay another round?")) break;
        }

        playAgain = askYesNo("\nWould you like to start a new game with $1000?");
    }

    cout << "\nThanks for playing Blackjack!\n";
    return 0;
}