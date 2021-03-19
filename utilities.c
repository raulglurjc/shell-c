#include "utilities.h"


#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <fcntl.h>
#include <stdbool.h>
#include <glob.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

enum
{
    MAXBUFF = 1024,
    MAXARGS = 200,
    MAXCMDS = 200,
    MAXTOKENS = 200
};




struct Comandos{
  char* args[MAXARGS];
  int argumentos;
};

struct Info{
  int here;
  int pipehere[2];
  int background;
  int ncommands;
  char* entrada;
  char* salida;
  struct Comandos* cmd;
};







int tokenizar(char* string,int max, char* match, char *tokens[])
{
  int n=0;
  char* token_aux;
  char* resto;

  while(((token_aux=strtok_r(string,match, &resto))!=NULL))
  {
    if(n>=max)
      break;

    if(tokens!=NULL)
      tokens[n]=token_aux;

    n++;
    string=resto;
  }
  return n;
}

int here_exists(char* buffer)
{
  if(strstr(buffer, "HERE{")!=NULL)
  {
    return 1;
  }

  else
    return 0;
}

void leer_here(struct Info* cmds)
{
  char aux[1024];
  while(fgets(aux, 1024, stdin)!=NULL)
  {
    if(strstr(aux, "}")!=NULL)
      break;
    write(cmds->pipehere[1], aux, strlen(aux));
  }

  close(cmds->pipehere[1]);
}
///////////////////////


void here(struct Info* cmds)
{

    pipe(cmds->pipehere);
    leer_here(cmds);

}



///////////////////////
void background(char* buffer, struct Info* cmds)
{
  if(strchr(buffer,'&'))
  {
    cmds->background = 1;
    tokenizar(buffer, 1, "&", NULL);
  }
}


bool setvar(char* buffer)
{
  char* tokens[2];
  int n=tokenizar(buffer, 2, "=\n", tokens);
  if (n==2)
  {
    setenv(tokens[0], tokens[1], 1);
    return true;
  }
  else
    return false;
}
int chvar(struct Info* cmds)
{
  for(int i=0; i<cmds->ncommands; i++)
  {
    int j=0;
    while(cmds->cmd[i].args[j]!=NULL)
    {
      if(cmds->cmd[i].args[j][0]=='$')
      {
        char *aux;
        aux = cmds->cmd[i].args[j]+1;
        if((cmds->cmd[i].args[j] = getenv(aux))== NULL)
        {
          fprintf(stderr, "error: var %s does not exist.\n", aux);
          return 1;
        }


      }


      j++;
    }
  }
  return 0;
}
///////////////////////
void analizar_linea(struct Info cmds, char* tokens[])
{
    int n;

    for(int i=0; i<cmds.ncommands; i++)
    {
      n = tokenizar(tokens[i], MAXARGS, " \t", cmds.cmd[i].args);

      cmds.cmd[i].args[n]=NULL;
      cmds.cmd[i].argumentos = n;
    }
}
///////////////////////
char* operar_redir(char* file)
{
  char* tokens[2];
  tokenizar(file, 2, " \n\t", tokens);

  if(tokens[0][0]=='$')
  {
    char* aux = getenv(tokens[0]+1);
    if (aux==NULL)
      fprintf(stderr, "error: var %s does not exist.\n", tokens[0]);
    return aux;
  }
  else
    return tokens[0];
}
int redireccion(char* buffer, struct Info *cmds)
{
  char* tokens[MAXCMDS];
  char* tokens2[MAXCMDS];
  int n;
  n = tokenizar(buffer, MAXCMDS, ">", tokens);
  if(n==2)
  {
    n=tokenizar(tokens[1], MAXCMDS, "<", tokens2);
    if(n==2)
    {
      cmds->salida=tokens2[0];
      cmds->entrada=tokens2[1];
    }
    else
      cmds->salida=tokens2[0];
  }
  n=tokenizar(buffer, MAXCMDS, "<", tokens);
  if(n==2)
    cmds->entrada=tokens[1];

  if(cmds->entrada!=NULL)
    {
      if((cmds->entrada = operar_redir(cmds->entrada))==NULL)
        return 1;

    }

  if(cmds->salida!=NULL)
  {
    if((cmds->salida = operar_redir(cmds->salida))==NULL)
      return 1;
  }


  return 0;
}
///////////////////////
char* getpath(char* command)
{
  char* path;
  char* tokens[MAXTOKENS];
  char cmdaux[MAXBUFF];
  char* cmdpath;
  int ntokens;
  int i;

  path = getenv("PATH");
  if(path == NULL)
    return NULL;

  if(access(command,X_OK)==0)
  {
    cmdpath = strdup(command);
    return cmdpath;
  }
  ntokens = tokenizar(path, MAXTOKENS, ":", tokens);
  for(i=0; i<ntokens; i++)
  {
    snprintf(cmdaux,MAXBUFF, "%s/%s", tokens[i], command);
    if(access(cmdaux, X_OK)==0)
    {
      cmdpath = strdup(cmdaux);
      return cmdpath;
    }
  }
  return NULL;
}
///////////////////////
void cerrar_pipes(int **pipes, int ncommands)
{
  for(int i = 0; i<ncommands-1; i++)
  {
    close(pipes[i][0]);
    close(pipes[i][1]);
    free(pipes[i]);
  }
  free(pipes);
}

int** crear_pipes(int ncommands)
{
  int** pipes = malloc(sizeof(int*)*ncommands-1);
  if(ncommands>1)
  {
    for(int i=0; i<ncommands-1; i++)
    {
      pipes[i] = malloc(sizeof(int)*2);
      pipe(pipes[i]);
    }
  }
  return pipes;
}



void globbing(char* argv[])
{
  glob_t buffer;

  char *argvf[MAXARGS];
  int i, n, j;

  i=0;
  n=0;
  while(argv[i]!=NULL)
  {
    if(glob(argv[i], GLOB_DOOFFS, NULL, &buffer)!=0)
    {
      argvf[n] = argv[i];
      n++;
    }
    else
    {
      for(j=0; j<buffer.gl_pathc; j++)
      {
        argvf[n] = strdup(buffer.gl_pathv[j]);
        n++;
      }
    }
    i++;
  }
  i=0;
  while(argvf[i]!=NULL)
  {
    argv[i]=argvf[i];
    i++;
  }

}
///////////////////////
void ejecutar(struct Info cmds)
{
  int status;
  int i;
  pid_t* pid;
  char* path;
  char aux[1024];
  int fd;
  int **pipes = crear_pipes(cmds.ncommands);

  pid = malloc(cmds.ncommands*sizeof(pid_t));

  for(i=0; i<cmds.ncommands; i++)
  {
    pid[i] = fork();
    if(pid[i]<0)
        err(EXIT_FAILURE,"ERROR fork");
    else if(pid[i]==0)
    {
        if(cmds.ncommands>1)
        {
          if(i==0)
            dup2(pipes[0][1],1);
          else if(i==cmds.ncommands-1)
            dup2(pipes[i-1][0],0);
          else
          {
            dup2(pipes[i-1][0],0);
            dup2(pipes[i][1],1);
          }
          cerrar_pipes(pipes,cmds.ncommands);
        }

        if(i==0)
        {
          if(cmds.entrada!=NULL)
          {
              fd=open(cmds.entrada, O_RDONLY);
              if(fd<0)
                err(EXIT_FAILURE, "ERROR fd");
              dup2(fd, 0);
              close(fd);
          }
          else
          {
            if(cmds.background)
            {
              fd=open("/dev/null",O_RDONLY);
              if(fd<0)
                err(EXIT_FAILURE, "ERROR fd");
              dup2(fd,0);
              close(fd);
            }
          }

          if(cmds.here==1)
            dup2(cmds.pipehere[0], 0);
        }

        if((i==cmds.ncommands-1)&&(cmds.salida!=NULL))
        {
          fd = creat(cmds.salida, S_IRUSR|S_IWUSR|S_IRGRP);
          if(fd<0)
            err(EXIT_FAILURE, "ERROR fd");
          dup2(fd, 1);
          close(fd);
        }


        if(cmds.here==1)
        {
          close(cmds.pipehere[0]);
          close(cmds.pipehere[1]);
        }


        path = getpath(cmds.cmd[i].args[0]);
        if(path == NULL)
          errx(EXIT_FAILURE, "%s: command not found", cmds.cmd[i].args[0]);

        globbing(cmds.cmd[i].args);
        execv(path, cmds.cmd[i].args);
        err(EXIT_FAILURE, "ERROR execv");
    }
  }
  if(cmds.ncommands>1)
    cerrar_pipes(pipes,cmds.ncommands);


  if(!cmds.background)
  {
    for(i=0; i<cmds.ncommands; i++)
    {
      if(waitpid(pid[i], &status, 0) < 0)
        err(EXIT_FAILURE, "ERROR wait");

      if(i==cmds.ncommands-1)
      {
        if(WIFEXITED(status))
        {
          snprintf(aux, 1024, "%d", WEXITSTATUS(status));
          setenv("result", aux, 1);
        }
      }

    }
  }
  free(pid);


}
///////////////////////
void cd(struct Comandos* comandos)
{
  char* home;
  if(!comandos[0].args[1])
  {
    home = getenv("HOME");
    if(!home)
      fprintf(stderr, "ERROR en home");
    chdir(home);
  }
  else
    chdir(comandos[0].args[1]);
}



int ifok(struct Comandos* comando)
{
  if(!strcmp(getenv("result"),"0"))
  {
    int i = 0;
    while(comando->args[i]!=NULL)
    {
      comando->args[i] = comando->args[i+1];
      i++;
    }
    return 0;

  }
  return 1;
}


int ifnot(struct Comandos* comando)
{
  if(strcmp(getenv("result"),"0"))
  {
    int i = 0;
    while(comando->args[i]!=NULL)
    {
      comando->args[i] = comando->args[i+1];
      i++;
    }
    return 0;

  }
  return 1;
}
