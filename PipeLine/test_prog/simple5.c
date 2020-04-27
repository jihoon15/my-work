int chk_prime(int n)
{
	int i;
	int co_prime = 0; // co-prime chk with i
	int res = 1; // 0: not prime, 1: prime

	n = 0x127;

	co_prime = gcd(2, n);
	if (co_prime != 1) {
		res = 0;
		return res;
	}

	for (i = 3 ; i < (n>>1) ; i = i+2 ) {
		co_prime = gcd(i, n);
		if (co_prime != 1) {
			res = 0;
			break;
		}
	}
	return res;
}

int gcd(int a, int b)
{
	if (a==b) return a;
	else if (a>b) return gcd(a-b, b);
	else return gcd(b-a, a);
}
