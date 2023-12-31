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
    int talon_base;
    int talon_length;
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

        if (index == state->talon_base + state->talon_length - 1 && state->talon_length > 0)
        {
            state->talon_length -= 1;
        }
        else
        {
            state->talon_base = index - 2;
            state->talon_length = 2;
        }

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
        if (from_stack->base < 0)
            from_stack->base = 0;
    }
    else if (move->type == RESET_DRAW)
    {
        state->talon_length = 0;
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

void print_legal_moves(State *state, int choice)
{
    for (int i = 0; i < n_legal_moves; i++)    
    {
        if (i == choice)
        {
            printf("! ");
        }

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

void find_legal_moves_for_card(State *state, Card card, int move_type, int from_position)
{
    for (int to_stack = 0; to_stack < N_STACKS; to_stack++)
    {
        if (to_stack == from_position)
        {
            continue;
        }

        int can = can_place_card(card, &state->stacks[to_stack]);
        if (can)
        {
            legal_moves[n_legal_moves].type = move_type;
            
            if (move_type == FROM_STACK)
            {
                legal_moves[n_legal_moves].from_stack = from_position;            
            }
            else
            {
                legal_moves[n_legal_moves].from_draw = from_position;
            }
            
            legal_moves[n_legal_moves].to_stack = to_stack;
            n_legal_moves += 1;
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
        find_legal_moves_for_card(state, moving, FROM_STACK, from_stack);
    }

    int have_talon = state->talon_length > 0;
    
    int before_talon = state->n_draw_cards;
    if (have_talon)
    {
        int before_talon = state->talon_base - 1;    
    }
    
    for (int i = 0; i < before_talon; i++)
    {
        if (i % 3 == 2)
        {
            // legal
            find_legal_moves_for_card(state, state->draw[i], FROM_DRAW, i);
        }
    }

    if (have_talon)
    {
        int talon_tip = state->talon_base + state->talon_length - 1;
        // legal
        find_legal_moves_for_card(state, state->draw[talon_tip], FROM_DRAW, talon_tip);

        for (int i = talon_tip + 1; i < state->n_draw_cards; i++)
        {
            if ((i - (talon_tip + 1)) % 3 == 2)
            {
                // legal
                find_legal_moves_for_card(state, state->draw[i], FROM_DRAW, i);
            }
        }
    }

    if (have_talon)
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
    
    state->talon_base = 0;
    state->talon_length = 0;

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
    int caret_position = 0;
    int caret = 0;
    
    for (int i = 0; i < state->n_draw_cards; i++)
    {
        Card card = state->draw[i];
        print_card(card);
        caret_position += 2;

        if (i == state->talon_base - 1 && state->talon_length > 0)
        {
            cycle_offset = state->talon_base % 3;
            for (int j = 0; j < cycle_offset; j++)
            {
                printf("  ");
                caret_position += 2;
            }
        }

        if (i == state->talon_base + state->talon_length - 1 && state->talon_length > 0)
        {
            caret = caret_position - 2;
            cycle_offset = (state->talon_base + state->talon_length - 1) % 3;
            for (int j = 0; j < cycle_offset; j++)
            {
                printf("  ");
            }
        }

        if ((i + cycle_offset) % 3 == 2 && i != state->n_draw_cards - 1)
        {
            printf("|");
            caret_position += 1;
        }
    }
    printf("\n");
    if (state->talon_length > 0)
    {
        printf("       ");
        for (int i = 0; i < caret; i++)
        {
            printf(" ");
        }
        printf("^");
    }
    printf("\n");

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

            if (stack.base != 0)
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

        int stuck = 0;
        if (current_state >= 30)
        {
            stuck = 1;
        }
        else
        {
            find_legal_moves(state);

            if (next_move >= n_legal_moves)
            {
                stuck = 1;
            }
        }

        if (!stuck)
        {
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

            // int i = 0;
            // find_legal_moves(&states[i]);
            // print_legal_moves(&states[i], move_sequence[i]);
            // for (int i = 1; i < n_moves + 1; i++)
            // {
            //     printf("--------------------------------\n");
            //     printf("\n");
            //     print_state(&states[i]);
            //     find_legal_moves(&states[i]);
            //     print_legal_moves(&states[i], move_sequence[i]);
            // }
            // exit(1);
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