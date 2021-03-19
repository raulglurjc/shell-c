
//RAÚL GIL LÓPEZ

#include  "utilities.h"


///////////////////////   MAIN   ///////////////////////
int main(int argc, char* argv[])
{
  char buffer[MAXBUFF];
  char* tokens[MAXCMDS];
  setenv("result", "0", 1);


  while(1)
  {
    printf("$ Shell: ");
    if(fgets(buffer,MAXBUFF,stdin)==NULL)
      break;
    if((buffer[0]=='\n') || (buffer[0]=='\t'))
      continue;
    if(setvar(buffer))
      continue;

    struct Info cmds;
    cmds.here = 0;
    cmds.entrada = NULL;
    cmds.salida = NULL;
    cmds.background = 0;
    if(here_exists(buffer))
    {
      cmds.here = 1;
      tokenizar(buffer, 2, "HERE{", NULL);
    }

    background(buffer, &cmds);
    if(redireccion(buffer, &cmds)==1)
      continue;

    if(cmds.here && (cmds.entrada || cmds.background))
    {
      fprintf(stderr, "No se puede utilizar redirecciones ni background para usar HERE{\n");
      continue;
    }

    if(cmds.here)
      here(&cmds);




    cmds.ncommands = tokenizar(buffer, MAXCMDS, "|\n", tokens);
    cmds.cmd = malloc(sizeof(struct Comandos)*cmds.ncommands);

    for(int i=0; i<cmds.ncommands;i++)//INICIALIZO A 0 EL NUMERO DE ARGUMENTOS DE CADA COMANDO
    {
      cmds.cmd[i].argumentos=0;
    }

    analizar_linea(cmds, tokens);

    if(chvar(&cmds))
      continue;

    if(!strcmp(cmds.cmd[0].args[0],"cd"))
    {
      cd(cmds.cmd);
      continue;
    }
    if(!strcmp(cmds.cmd[0].args[0],"ifok"))
    {
      if(ifok(&cmds.cmd[0])==1)
        continue;
    }
    if(!strcmp(cmds.cmd[0].args[0],"ifnot"))
    {
      if(ifnot(&cmds.cmd[0])==1)
        continue;
    }


    ejecutar(cmds);

    free(cmds.cmd);
  }
  exit(EXIT_SUCCESS);
}
////////////////////////////////////////////////////////
