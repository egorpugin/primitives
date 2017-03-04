/*
name: main

*/

#include <primitives/hash.h>

int main(int argc, char **argv)
{
    auto s = generate_random_sequence(50);
    s = generate_random_sequence(50);

    s = generate_random_sequence(500);
    s = generate_strong_random_sequence(0);
    s = generate_strong_random_sequence(1);
    s = generate_strong_random_sequence(2);
    s = generate_strong_random_sequence(3);
    s = generate_strong_random_sequence(10);
    s = generate_strong_random_sequence(500);
    s = generate_strong_random_sequence(501);

    return 0;
}
