#pragma once

int main(int argc, char *argv[]);
int PRIMITIVES_MAIN(int argc, char *argv[]);

#ifndef PRIMITIVES_MAIN_IMPL
#define main PRIMITIVES_MAIN
#endif
