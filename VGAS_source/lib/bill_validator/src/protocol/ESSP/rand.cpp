#include "rand.h"
unsigned long long GeneratePrime(void)
{
    unsigned long long tmp = random(100000);

    tmp %= MAX_PRIME_NUMBER;

    /*  ensure it is an odd number	*/
    if ((tmp & 1) == 0)
        tmp += 1;

    /* test for prime  */
    if (MillerRabin(tmp, 5) == 1)
        return tmp;
    /*  increment until prime  */
    do
    {
        tmp += 2;
    } while (MillerRabin(tmp, 5) == 0);

    return tmp;
}

/*	Performs the miller-rabin primality test on a guessed prime n.
|	trials is the number of attempts to verify this, because the function
|	is not 100% accurate it may be a composite.  However setting the trial
|	value to around 5 should guarantee success even with very large primes		*/

unsigned char MillerRabin(long long n, long long trials)
{
    long long a = 0;
    long long i;
    for (i = 0; i < trials; i++)
    {
        a = (random(100000) % (n - 3)) + 2; /* gets random value in [2..n-1] */

        if (IsItPrime(n, a) == 0)
        {
            return 0;
            /*n composite, return false */
        }
    }
    return 1; /* n probably prime */
}

/* Checks the integer n for primality		*/

unsigned char IsItPrime(long long n, long long a)
{
    long long d = XpowYmodN(a, n - 1, n);
    if (d == 1)
        return 1;
    else
        return 0;
}

/*		Raises X to the power Y in modulus N
		the values of X, Y, and N can be massive, and this can be
		acheived by first calculating X to the power of 2 then
		using power chaining over modulus N				*/

long long XpowYmodN(long long x, long long y, long long N)
{
    int i;
    long long result = 1;

    const long long oneShift63 = ((long long)1) << 63;

    if (y == 1)
        return (x % N);

    for (i = 0; i < 64; y <<= 1, i++)
    {
        result = result * result % N;
        if (y & oneShift63)
            result = result * x % N;
    }

    return result;
}