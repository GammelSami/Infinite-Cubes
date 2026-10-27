/* rand and srand as the Windows C runtime implements them, in place of glibc's: the game seeds them to generate
   worlds and missions, expects RAND_MAX to be 0x7fff (platform/linux/include/windows.h) and must produce the same
   sequences as on Windows. As there, every thread has its own state, starting at 1. */
static _Thread_local unsigned int s_state = 1;

int rand(void)
{
    s_state = s_state * 214013u + 2531011u;
    return (int)((s_state >> 16) & 0x7fff);
}

void srand(unsigned int seed)
{
    s_state = seed;
}
