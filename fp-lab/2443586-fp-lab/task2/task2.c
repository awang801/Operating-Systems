#include <stdio.h>
#include <stdlib.h>

/* IMPLEMENT ME: Declare your functions here */
int add (int a, int b);
int subtract (int a, int b);
int multiply(int a, int b);
int divide(int a, int b);

int main ()
{
	/* IMPLEMENT ME: Insert your algorithm here */
	int a = 5;
	int b = 5;
	int choice;
	int x;
	printf("Specify the operation to perform(0 : add | 1 :subtract | 2 : Multiply | 3: divide\n");
	scanf("%d", &choice);
	if (choice == 0)
	{
		x = add(a, b);
		printf("x = %d\n", x);
	}
	else if (choice == 1)
	{
		x = subtract(a,b);
		printf("x = %d\n", x);
	}
	else if (choice == 2)
	{
		x = multiply(a,b);
		printf("x = %d\n", x);
	}
	else if (choice == 3)
	{
		x = divide(a,b);
		printf("x = %d\n", x);
	}
	else
	{

	}

	return 0;
}

/* IMPLEMENT ME: Define your functions here */
int add (int a, int b) { printf ("Adding 'a' and 'b'\n"); return a + b; }
int subtract(int a, int b) { printf("Subtracting 'a' and 'b'\n"); return a - b;}
int multiply(int a, int b) { printf("Multiplying 'a' and 'b' \n"); return a * b;}
int divide(int a, int b) { printf("Diving 'a' by 'b' \n"); return a/b;}
