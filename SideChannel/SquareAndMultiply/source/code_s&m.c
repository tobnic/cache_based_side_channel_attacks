/*
Square-and-Multiply Algorithm (SMA)

@authors:
	Jonathan André Gangi 345334
	Philippe Cesar Ramos 380415

as seen on section 7.4 of Understanding Criptography:

	Square-and-Multiply for Modular Exponentiation

	Input:  base element x, Exponent H, Modulus n
	Output: Y = xˆH mod n
	Initialization: r = x

	Algorithm:
		FORi=t−1DOWNTO0

			r = rˆ2 mod n

			IF hi = 1
				r = r * x mod n

		RETURN (r)

and specified by:
<http://www.moodle.ufscar.br/file.php/4783/Aulas/Aula06/Atividades_sobre_a_teoria_dos_numeros.pdf>
	Input: three integers X, k, N, being 2 <= X, k, N < 2ˆ32.
	Output: one integers Y, as result from its exponentiation

*/

#include <stdio.h>
#include <stdlib.h>

long fnc_sqr_n(long r){
  return r*r;
}
long fnc_mul_n(long r, long n){
  return r * n;
}
long fnc_mod_n(long r, long n){
  return r % n;
}

 long sma(){

  long x = 2;
  long n = 600;
  int bin[32]; // = 5;
  bin[0]=1;
  bin[1]=0;
  bin[2]=1;
  bin[3]=0;
  bin[4]=1;
  bin[5]=1;
  bin[6]=1;
  int i = 7;

	long h;
	unsigned long long r;

	r = x;
	i--; //t-1

	while(i>0){

		//r = (r * r) % n;
    r = fnc_sqr_n(r);
    r = fnc_mod_n(r,n);


		if( bin[--i] == 1 ){
      r = fnc_mul_n(r,x);
      r = fnc_mod_n(r,n);
		}
printf("%d %d\n",i,bin[i]);
	}

	return r;

}


int main(){


	printf("%ld\n", sma());

	return 0;
}
