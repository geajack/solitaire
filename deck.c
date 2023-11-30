enum Suit { SPADES, HEARTS, CLUBS, DIAMONDS };
typedef enum Suit Suit;

enum Rank { A = 1, J = 11, Q = 12, K = 13, NO_CARD = 0 };
typedef enum Rank Rank;

struct Card
{
    Suit suit;
    Rank rank;
};
typedef struct Card Card;

const char *SUITS[4] = {
    [SPADES] = "♠",
    [CLUBS] = "♣",
    [HEARTS] = "♥",
    [DIAMONDS] = "♦"
};

const char *RANKS[K + 1] = {
    [A] = "1",
    [2] = "2",
    [3] = "3",
    [4] = "4",
    [5] = "5",
    [6] = "6",
    [7] = "7",
    [8] = "8",
    [9] = "9",
    [10] = "T",
    [J] = "J",
    [Q] = "Q",
    [K] = "K"
};

int COLOR(Card card)
{
    return card.suit % 2;
}


int print_card(Card card)
{
    if (card.suit == HEARTS || card.suit == DIAMONDS)
    {
        printf("\033[1;31m%s\033[1;39m%s", SUITS[card.suit], RANKS[card.rank]);
    }
    else
    {
        printf("%s%s", SUITS[card.suit], RANKS[card.rank]);
    }
}

void shuffle_init(Card *deck)
{
    for (int i = 0; i < 52; i++)
    {
        deck[i].rank = NO_CARD;
    }
}

int shuffle_insert(Card *deck, Card card)
{
    int target_index = -1;
    int n_possibilities = 0;
    for (int i = 0; i < 52; i++)
    {
        if (deck[i].rank == NO_CARD)
        {
            n_possibilities += 1;
            if ((rand() % n_possibilities) == 0)
            {
                target_index = i;
            }
        }
    }

    if (target_index >= 0)
    {
        deck[target_index] = card;
    }

    return target_index;
}