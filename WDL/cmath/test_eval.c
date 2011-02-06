/*
  test_eval.h
  Copyright (C) 2011 and later Lubomir I. Ivanov (neolit123 [at] gmail)
  
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.
  
  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:
  
  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/*
  test horner.h for complex numbers and other related headers 

  gcc -W -Wall -Wextra -ansi pedantic
  cl /W4 /Za

  reduced precisions for ansi c
*/

#include "stdio.h"
#include "complex_number.h"
#include "horner.h"
#include "durand_kerner.h"

int main(void)
{
  cmath_uint16_t i = 0;

  cnum_t y[] = {2, -6, 2, -1};
  cnum_s cy[] = {{2, 0}, {-6, 0}, {2, 0}, {-1, 0}};

  cnum_t fx = horner_eval(y, 3, poly_order(y));
  cnum_s fcx = horner_eval_c(cy, _CNUM(3, 0), poly_order(y));

  cnum_t dk_coeff[] = {1, -3, 3, -5};
  cnum_s dk_roots[3];

  cnum_s dk_coeff_c[] = {{1, 0}, {-3, 0}, {3, 0}, {-5, 0}};
  cnum_s dk_roots_c[3];

  durand_kerner(dk_coeff, dk_roots, poly_order(dk_coeff));
  durand_kerner_c(dk_coeff_c, dk_roots_c, poly_order(dk_coeff_c));

  /* */
  #ifdef _CMATH_ANSI
    puts("\n\nansi c is: on");
  #else
    puts("\n\nansi c is: off");
  #endif

  /* */
  puts("\n\nevaluate polynomials:\n");
  puts("y(x) = x^3 - 6*x^2 + 2*x - 1");
  printf("y(3) = %.15f\n\n", (double)fx);
  puts("y(x) = x^(3 + i*0) - 6*x^(2 + i*0) + 2*x^(1 + i*0) - 1");
  printf("y(3 + i*0) = %.15f \t %.15f*i\n", (double)fcx.r, (double)fcx.i);

  /* */
  puts("\n\nfind roots of:");
  puts("\nx^3 - 3*x^2 + 3*x - 5 = 0");
  i = 0;
  while (i < poly_order(dk_coeff))
  {
    printf("root[%2d]: %.15f \t % .15f*i\n",
      i+1, (double)dk_roots[i].r, (double)dk_roots[i].i);
    i++;
  }
  puts("\n(1 + i*0)*x^3 - (3 + i*0)*x^2 + (3 + i*0)*x - 5 = 0");
  i = 0;
  while (i < poly_order(dk_coeff_c))
  {
    printf("root[%2d]: %.15f \t % .15f*i\n",
      i+1, (double)dk_roots_c[i].r, (double)dk_roots_c[i].i);
    i++;
  }

  return 0;
}
