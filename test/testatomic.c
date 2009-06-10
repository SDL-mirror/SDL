#include "SDL.h"

int
main(int argc, char **argv)
{
    int rv = 10;
    volatile int atomic;

    SDL_atomic_int_set(&atomic, 10);
    if (SDL_atomic_int_get(&atomic) != 10)
        printf("Error: ");
    printf("SDL_atomic_int_set(atomic, 10): atomic-> %d\n",
           SDL_atomic_int_get(&atomic));

    SDL_atomic_int_add(&atomic, 10);
    if (SDL_atomic_int_get(&atomic) != 20)
        printf("Error: ");
    printf("SDL_atomic_int_add(atomic, 10): atomic-> %d\n",
           SDL_atomic_int_get(&atomic));

    rv = SDL_atomic_int_cmp_xchg(&atomic, 20, 30);
    if (rv != SDL_TRUE || SDL_atomic_int_get(&atomic) != 30)
        printf("Error: ");
    printf("SDL_atomic_int_cmp_xchg(atomic, 20, 30): rv-> %d, atomic-> %d\n",
           rv, SDL_atomic_int_get(&atomic));

    rv = SDL_atomic_int_cmp_xchg(&atomic, 20, 30);
    if (rv != SDL_FALSE || SDL_atomic_int_get(&atomic) != 30)
        printf("Error: ");
    printf("SDL_atomic_int_cmp_xchg(atomic, 20, 40): rv-> %d, atomic-> %d\n",
           rv, SDL_atomic_int_get(&atomic));

    rv = SDL_atomic_int_xchg_add(&atomic, 10);
    if (rv != 30 || SDL_atomic_int_get(&atomic) != 40)
        printf("Error: ");
    printf("SDL_atomic_int_xchg_add(atomic, 10): rv-> %d, atomic-> %d\n",
           rv, SDL_atomic_int_get(&atomic));

    SDL_atomic_int_inc(&atomic);
    if (SDL_atomic_int_get(&atomic) != 41)
        printf("Error: ");
    printf("SDL_atomic_int_inc(atomic): atomic-> %d\n",
           SDL_atomic_int_get(&atomic));

    rv = SDL_atomic_int_dec_test(&atomic);
    if (rv != SDL_FALSE || SDL_atomic_int_get(&atomic) != 40)
        printf("Error: ");
    printf("SDL_atomic_int_dec_test(atomic): rv-> %d, atomic-> %d\n",
           rv, SDL_atomic_int_get(&atomic));

    SDL_atomic_int_set(&atomic, 1);
    if (SDL_atomic_int_get(&atomic) != 1)
        printf("Error: ");
    printf("SDL_atomic_int_set(atomic, 1): atomic-> %d\n",
           SDL_atomic_int_get(&atomic));

    rv = SDL_atomic_int_dec_test(&atomic);
    if (rv != SDL_TRUE || SDL_atomic_int_get(&atomic) != 0)
        printf("Error: ");
    printf("SDL_atomic_int_dec_test(atomic): rv-> %d, atomic-> %d\n",
           rv, SDL_atomic_int_get(&atomic));

    return 0;
}
