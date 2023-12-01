#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "deck.c"

#define N_STACKS 7

struct Stack
{
    Card cards[N_STACKS - 1 + K];
    int base;
    int length;
};
typedef struct Stack Stack;

struct State
{
    Stack stacks[N_STACKS];
    Card draw[52 - (N_STACKS * (N_STACKS + 1) / 2)];
    int n_draw_cards;
    int draw_position;
};
typedef struct State State;

struct Move
{
    enum {FROM_DRAW, FROM_STACK, RESET_DRAW} type;
    union
    {
        int from_draw;
        int from_stack;
    };
    int to_stack;
};
typedef struct Move Move;

void play_move(State *state, Move *move)
{
    if (move->type == FROM_DRAW)
    {
        int index = move->from_draw;

        Card moving = state->draw[index];
        for (int i = index + 1; i < state->n_draw_cards; i++)
        {
            state->draw[i - 1] = state->draw[i];
        }

        int to_index = state->stacks[move->to_stack].length;
        state->stacks[move->to_stack].cards[to_index] = moving;
        state->stacks[move->to_stack].length += 1;

        int new_draw_position = index - 1;
        if (new_draw_position % 3 == 2)
        {
            new_draw_position = -1;
        }
        state->draw_position = new_draw_position;

        state->n_draw_cards -= 1;
    }
    else if (move->type == FROM_STACK)
    {
        Stack *from_stack = &state->stacks[move->from_stack];
        Stack *to_stack = &state->stacks[move->to_stack];
        
        for (int i = from_stack->base; i < from_stack->length; i++)
        {
            to_stack->cards[to_stack->length++] = from_stack->cards[i];
        }
        from_stack->length = from_stack->base;
        from_stack->base = from_stack->length - 1;
    }
    else if (move->type == RESET_DRAW)
    {
        state->draw_position = -1;
    }
}

int can_place_card(Card card, Stack *stack)
{
    if (card.rank == K)
    {
        if (stack->length == 0)
        {
            return 1;
        }
    }
    else
    {
        if (stack->length == 0)
        {
            return 0;
        }

        Card target = stack->cards[stack->length - 1];
        if (target.rank == card.rank + 1)
        {
            if (COLOR(target) != COLOR(card))
            {
                return 1;
            }
        }
    }

    return 0;
}

Move legal_moves[256];
int n_legal_moves = 0;

int move_sequence[256];
int n_moves = 0;

void print_legal_moves(State *state)
{
    for (int i = 0; i < n_legal_moves; i++)    
    {
        Move move = legal_moves[i];
        if (move.type == RESET_DRAW)
        {
            printf("reset stack\n");
        }
        else
        {
            if (move.type == FROM_DRAW)
            {
                printf("draw %d (", move.from_draw);
                print_card(state->draw[move.from_draw]);
            }
            else
            {
                printf("stack %d (", move.from_stack + 1);
                print_card(state->stacks[move.from_stack].cards[state->stacks[move.from_stack].base]);
            }

            printf(") -> stack %d", move.to_stack + 1);
            if (state->stacks[move.to_stack].length > 0)
            {
                printf("(");
                print_card(state->stacks[move.to_stack].cards[state->stacks[move.to_stack].length - 1]);
                printf(")");
            }
            printf("\n");
        }
    }
}

void find_legal_moves(State *state)
{
    n_legal_moves = 0;
    for (int from_stack = 0; from_stack < N_STACKS; from_stack++)
    {
        if (state->stacks[from_stack].length == 0)
        {
            continue;
        }

        Card moving = state->stacks[from_stack].cards[state->stacks[from_stack].base];
        for (int to_stack = 0; to_stack < N_STACKS; to_stack++)
        {
            if (to_stack == from_stack)
            {
                continue;
            }

            int can = can_place_card(moving, &state->stacks[to_stack]);
            if (can)
            {
                legal_moves[n_legal_moves].type = FROM_STACK;
                legal_moves[n_legal_moves].from_stack = from_stack;
                legal_moves[n_legal_moves].to_stack = to_stack;
                n_legal_moves += 1;
            }
        }
    }

    int cycle_offset = 0;
    int draw_position = state->draw_position;
    if (draw_position == -1)
    {
        draw_position = 0;
    }

    for (int i = draw_position; i < state->n_draw_cards; i++)
    {
        Card card = state->draw[i];

        if (i == draw_position)
        {
            cycle_offset = draw_position % 3;
        }
        
        if ((i + cycle_offset) % 3 == 2 || i == state->n_draw_cards - 1)
        {
            Card moving = state->draw[i];
            for (int to_stack = 0; to_stack < N_STACKS; to_stack++)
            {
                int can = can_place_card(moving, &state->stacks[to_stack]);
                if (can)
                {
                    legal_moves[n_legal_moves].type = FROM_DRAW;
                    legal_moves[n_legal_moves].from_draw = i;
                    legal_moves[n_legal_moves].to_stack = to_stack;
                    n_legal_moves += 1;
                }
            }
        }
    }

    if (state->draw_position >= 0)
    {
        legal_moves[n_legal_moves].type = RESET_DRAW;
        n_legal_moves += 1;
    }
}

void init_state(State *state)
{
    Card deck[52];
    shuffle_init(deck);
    for (Suit s = 0; s < 4; s++)
    {
        for (Rank r = A; r <= K; r++)
        {
            Card card = {s, r};
            shuffle_insert(deck, card);
        }
    }

    int deck_index = 0;
    state->n_draw_cards = 52 - (N_STACKS * (N_STACKS + 1) / 2);
    state->draw_position = -1;
    for (int stack_id = 0; stack_id < N_STACKS; stack_id++)
    {
        state->stacks[stack_id].base = stack_id;
        state->stacks[stack_id].length = stack_id + 1;
        for (int i  = 0; i < stack_id + 1; i++)
        {
            state->stacks[stack_id].cards[i] = deck[deck_index++];
        }
    }
    for (int i = 0; i < state->n_draw_cards; i++)
    {
        state->draw[i] = deck[deck_index++];
    }
}

void print_state(State *state)
{
    printf("draw: ");

    int cycle_offset = 0;
    int draw_position = state->draw_position;
    for (int i = 0; i < state->n_draw_cards; i++)
    {
        Card card = state->draw[i];
        print_card(card);

        if (i == draw_position)
        {
            cycle_offset = draw_position % 3;
            for (int j = 0; j < cycle_offset; j++)
            {
                printf("  ");
            }
        }

        if ((i + cycle_offset) % 3 == 2 && i != state->n_draw_cards - 1)
        {
            printf("|");
        }
    }
    printf("\n");
    if (state->draw_position >= 0)
    {
        printf("draw position: %d\n", state->draw_position);
    }

    int row = 0;
    int are_cards = 1;
    while (are_cards)
    {
        are_cards = 0;
        for (int stack_id = 0; stack_id < N_STACKS; stack_id++)
        {
            int base = state->stacks[stack_id].base;
            int length = state->stacks[stack_id].length;
            
            if (row < base)
            {
                printf("▨▨ ");
                are_cards = 1;
            }
            else if (row >= length)
            {
                printf("   ");
            }
            else
            {
                Card card = state->stacks[stack_id].cards[row];
                print_card(card);
                printf(" ");
                are_cards = 1;
            }
        }
        printf("\n");
        row += 1;
    }
}

int is_solved(State *state)
{
    for (int i = 0; i < N_STACKS; i++)
    {
        Stack stack = state->stacks[i];
        if (stack.length > 0)
        {
            if (stack.length < 13)
            {
                return 0;
            }

            if (stack.base != 12)
            {
                return 0;
            }
        }
    }
    return 1;
}

State states[256];
int current_state = 0;

int main()
{
    int seed = time(0);
    seed = 1701386713;
    srand(seed);
    printf("random seed: %d\n", seed);

    State *state = &states[0];
    init_state(state);

    print_state(state);

    int quit = 0;
    int next_move = 0;
    while (!quit)
    {
        State *state = &states[current_state];

        if (is_solved(state))
        {
            printf("solved\n");
            exit(0);
        }

        if (current_state >= 30)
        {
            n_moves -= 1;
            current_state -= 1;
            next_move = move_sequence[n_moves] + 1;
        }
        else
        {
            find_legal_moves(state);

            if (next_move < n_legal_moves)
            {
                // make move
                move_sequence[n_moves] = next_move;
                n_moves += 1;
                current_state += 1;
                states[current_state] = states[current_state - 1];
                play_move(&states[current_state], &legal_moves[next_move]);
                next_move = 0;
            }
            else
            {
                n_moves -= 1;
                current_state -= 1;
                next_move = move_sequence[n_moves] + 1;
            }
        }

        if (n_moves < 0)
        {
            printf("unsolvable\n");
            exit(0);
        }
    }

    // for (int i = 0; i < 10; i++)
    // {
    //     print_state(state);
    //     find_legal_moves(state);
    //     print_legal_moves(state);
        
    //     if (n_legal_moves > 0)
    //     {
    //         printf("---------------------------------------------\n");
    //         printf("\n");
    //         play_move(state, &legal_moves[0]);
    //     }
    //     else
    //     {
    //         printf("no legal moves\n");
    //         break;
    //     }
    // }
}