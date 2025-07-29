#include "DataloggerSis.h"

int main()
{
  InitSistema();

  while (true)
  {
    if(flag_grav_dados)
    {
      gravar_dados();
      printf("Gravação bem sucedida\n");
      flag_grav_dados = 0;
    }
    else
    {
      sistema_livre();
      sleep_ms(1000);
    }
  }
  
}