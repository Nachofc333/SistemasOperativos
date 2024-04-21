
// P2-SSOO-22/23

// MSH main file
// Write your msh source code here

// #include "parser.h"
#include <stddef.h> /* NULL */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>

#define MAX_COMMANDS 8

// ficheros por si hay redirección
char filev[3][64];

// to store the execvp second parameter
char *argv_execvp[8];

void siginthandler(int param)
{
	printf("**** Saliendo del MSH **** \n");
	// signal(SIGINT, siginthandler);
	exit(0);
}

/* Timer */
pthread_t timer_thread;
unsigned long mytime = 0;

void *timer_run()
{
	while (1)
	{
		usleep(1000);
		mytime++;
	}
}

/**
 * Get the command with its parameters for execvp
 * Execute this instruction before running an execvp to obtain the complete command
 * @param argvv
 * @param num_command
 * @return
 */
void getCompleteCommand(char ***argvv, int num_command)
{
	// reset first
	for (int j = 0; j < 8; j++)
	{
		argv_execvp[j] = NULL;
	}

	int i = 0;
	for (i = 0; argvv[num_command][i] != NULL; i++)
	{
		argv_execvp[i] = argvv[num_command][i];
	}
}

/**
 * Main shell Loop
 */
int main(int argc, char *argv[])
{
	/**** Do not delete this code.****/
	int end = 0;
	int executed_cmd_lines = -1;
	char *cmd_line = NULL;
	char *cmd_lines[10];

	if (!isatty(STDIN_FILENO))
	{
		cmd_line = (char *)malloc(100);
		while (scanf(" %[^\n]", cmd_line) != EOF)
		{
			if (strlen(cmd_line) <= 0)
				return 0;
			cmd_lines[end] = (char *)malloc(strlen(cmd_line) + 1);
			strcpy(cmd_lines[end], cmd_line);
			end++;
			fflush(stdin);
			fflush(stdout);
		}
	}

	pthread_create(&timer_thread, NULL, timer_run, NULL);

	/*********************************/

	char ***argvv = NULL;
	int num_commands;
	int a = 0;

	while (1)
	{
		int status = 0;
		int command_counter = 0;
		int in_background = 0;
		signal(SIGINT, siginthandler);

		// Prompt
		write(STDERR_FILENO, "MSH>>", strlen("MSH>>"));

		// Get command
		// ********** DO NOT MODIFY THIS PART. IT DISTINGUISH BETWEEN NORMAL/CORRECTION MODE***************
		executed_cmd_lines++;
		if (end != 0 && executed_cmd_lines < end)
		{
			command_counter = read_command_correction(&argvv, filev, &in_background, cmd_lines[executed_cmd_lines]);
		}
		else if (end != 0 && executed_cmd_lines == end)
		{
			return 0;
		}
		else
		{
			command_counter = read_command(&argvv, filev, &in_background); // NORMAL MODE
		}
		/************************ STUDENTS CODE ********************************/
		if (command_counter > 0)
		{
			if (command_counter > MAX_COMMANDS)
			{
				printf("Error: Numero máximo de comandos es %d \n", MAX_COMMANDS);
			}
			for (int i = 0; i < command_counter; i++)
			{
				getCompleteCommand(argvv, i);
			}
			getCompleteCommand(argvv, 0);
			if (strcmp(argv_execvp[0], "mycalc") == 0)
			{
				char *pto;
				// Comprobar que el formato esté correctamente
				if (argv_execvp[1] != NULL && strtol(argv_execvp[1], &pto, 10) != 0 && argv_execvp[2] != NULL && argv_execvp[3] != NULL && strtol(argv_execvp[3], &pto, 10))
				{

					if (strcmp(argv_execvp[2], "add") == 0)
					{
						int n1 = atoi(argv_execvp[1]);
						int n2 = atoi(argv_execvp[3]);
						int r = n1 + n2;
						a = a + r;
						// Actualizamos la variable de entorno
						char buffer[20];
						sprintf(buffer, "%d", a);
						const char *p = buffer;
						setenv("Acc", p, 1);
						fprintf(stderr, "[OK] %d + %d = %d; Acc %s\n", n1, n2, r, getenv("Acc"));
					}
					else if (strcmp(argv_execvp[2], "mul") == 0)
					{
						int n1 = atoi(argv_execvp[1]);
						int n2 = atoi(argv_execvp[3]);
						int r = n1 * n2;
						fprintf(stderr, "[OK] %d * %d = %d\n", n1, n2, r);
					}
					else if (strcmp(argv_execvp[2], "div") == 0)
					{
						// No se puede dividir por 0
						if (strcmp(argv_execvp[3], "0") != 0)
						{
							int n1 = atoi(argv_execvp[1]);
							int n2 = atoi(argv_execvp[3]);
							int d = n1 / n2;
							int r = n1 % n2;
							fprintf(stderr, "[OK] %d/%d = %d; Resto %d\n", n1, n2, d, r);
						}
						else
						{
							fprintf(stdout, "[ERROR]Error dividir entre 0\n");
							fflush(stdout);
						}
					}
					else
					{
						fprintf(stdout, "[ERROR] La estructura del comando es mycalc <operando_1><add/mul/div><operando_2>\n");
						fflush(stdout);
					}
				}
				else
				{
					fprintf(stdout, "[ERROR] La estructura del comando es mycalc <operando_1><add/mul/div><operando_2>\n");
					fflush(stdout);
				}
			}
			else if (strcmp(argv_execvp[0], "mytime") == 0)
			{
				// A partir de mytime se obtiene el timepo que lleva ejecutandose
				unsigned long total_time = mytime / 1000;
				unsigned long hours = total_time / 3600;
				unsigned long minutes = (total_time % 3600) / 60;
				unsigned long seconds = total_time % 60;
				fprintf(stderr, "%02lu:%02lu:%02lu\n", hours, minutes, seconds);
			}

			// Para los procesos externos
			else if (command_counter == 1)
			{
				int pid = fork();
				if (pid < 0)
				{
					perror("Error: No se pudo crear un nuevo proceso.\n");
					exit(1);
				}

				else if (pid == 0)
				{
					// Proceso Hijo
					getCompleteCommand(argvv, 0);

					// Redireccciona la entrada
					if (strcmp(filev[0], "0") != 0)
					{
						int input_fd = open(filev[0], O_RDONLY);
						if (input_fd < 0)
						{
							perror("Error: No se pudo abrir el archivo de entrada.\n");
							exit(1);
						}
						dup2(input_fd, STDIN_FILENO);
						close(input_fd);
					}

					// Redirecciona la salida
					if (strcmp(filev[1], "0") != 0)
					{
						int output_fd = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
						if (output_fd < 0)
						{
							perror("Error: No se pudo abrir el archivo de salida.\n");
							exit(1);
						}
						dup2(output_fd, STDOUT_FILENO);
						close(output_fd);
					}

					// Redirecciona el error
					if (strcmp(filev[2], "0") != 0)
					{
						int error_fd = open(filev[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
						if (error_fd < 0)
						{
							perror("Error: No se pudo abrir el archivo de error.\n");
							exit(1);
						}
						dup2(error_fd, STDERR_FILENO);
						close(error_fd);
					}

					execvp(argv_execvp[0], argv_execvp);
					perror("Error: No se pudo ejecutar el mandato.\n");
					exit(1);
				}
				else
				{
					// Caso en el que se ejecute en background
					if (in_background)
					{

						printf("[%d]\n", getpid());
					}

					else
					{
						waitpid(pid, &status, 0);
					}
				}
			}

			else if (command_counter > 1)
			{
				// Para tres comados
				if (command_counter == 3)
				{
					// Crear las pipes
					int p1[2], p2[2];
					pipe(p1);
					pipe(p2);
					int pid1 = fork();

					// Primer proceso
					if (pid1 == 0)
					{
						// Redirecciona la entrada
						if (strcmp(filev[0], "0") != 0)
						{
							int input_fd = open(filev[0], O_RDONLY);
							if (input_fd < 0)
							{
								perror("Error: No se pudo abrir el archivo de entrada.\n");
								exit(1);
							}
							dup2(input_fd, STDIN_FILENO);
							close(input_fd);
						}

						// Redirecciona la salida
						dup2(p1[1], STDOUT_FILENO);

						close(p1[0]);
						close(p1[1]);
						close(p2[0]);
						close(p2[1]);
						getCompleteCommand(argvv, 0);
						execvp(argv_execvp[0], argv_execvp);
						perror("Error: No se pudo ejecutar el mandato.\n");
						exit(1);
					}

					// Segundo proceso
					int pid2 = fork();
					if (pid2 == 0)
					{
						dup2(p1[0], STDIN_FILENO);
						dup2(p2[1], STDOUT_FILENO);
						close(p1[0]);
						close(p1[1]);
						close(p2[0]);
						close(p2[1]);
						getCompleteCommand(argvv, 1);
						execvp(argv_execvp[0], argv_execvp);
						perror("Error: No se pudo ejecutar el mandato.\n");
						exit(1);
					}

					// Tercer proceso
					int pid3 = fork();
					if (pid3 == 0)
					{
						dup2(p2[0], STDIN_FILENO);

						// Redirecciona la entrada
						if (strcmp(filev[1], "0") != 0)
						{
							int output_fd = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
							if (output_fd < 0)
							{
								perror("Error: No se pudo abrir el archivo de salida.\n");
								exit(1);
							}
							dup2(output_fd, STDOUT_FILENO);
							close(output_fd);
						}

						// Redirecciona la salida
						if (strcmp(filev[2], "0") != 0)
						{
							int error_fd = open(filev[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
							if (error_fd < 0)
							{
								perror("Error: No se pudo abrir el archivo de error.\n");
								exit(1);
							}
							dup2(error_fd, STDERR_FILENO);
							close(error_fd);
						}

						close(p1[0]);
						close(p1[1]);
						close(p2[0]);
						close(p2[1]);
						getCompleteCommand(argvv, 2);
						execvp(argv_execvp[0], argv_execvp);
						perror("Error: No se pudo ejecutar el mandato.\n");
						exit(1);
					}
					close(p1[0]);
					close(p1[1]);
					close(p2[0]);
					close(p2[1]);

					// Esperer a que terminen los hijos

					if (!in_background)
					{
						wait(NULL);
						wait(NULL);
						wait(NULL);
					}
					else
					{
						printf("[%d]\n", pid3);
					}
				}

				else if (command_counter == 2)
				{
					// Para dos comandos
					int p[2];
					pipe(p);

					int pid1 = fork();
					if (pid1 == 0)
					{
						// Redirecciona la entrada
						if (strcmp(filev[0], "0"))
						{
							int input_fd = open(filev[0], O_RDONLY);
							if (input_fd < 0)
							{
								perror("Error: No se pudo abrir el archivo de entrada.\n");
								exit(1);
							}
							dup2(input_fd, STDIN_FILENO);
							close(input_fd);
						}

						// Redirecciona la salida
						dup2(p[1], STDOUT_FILENO);

						close(p[0]);
						close(p[1]);
						getCompleteCommand(argvv, 0);
						execvp(argv_execvp[0], argv_execvp);
						perror("Error: No se pudo ejecutar el mandato.\n");
						exit(1);
					}

					// Segundo proceso
					int pid2 = fork();
					if (pid2 == 0)
					{
						dup2(p[0], STDIN_FILENO);

						// Redirecciona la salida
						if (strcmp(filev[1], "0"))
						{
							int output_fd = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
							if (output_fd < 0)
							{
								perror("Error: No se pudo abrir el archivo de salida.\n");
								exit(1);
							}
							dup2(output_fd, STDOUT_FILENO);
							close(output_fd);
						}

						// Redirecciona el error
						if (strcmp(filev[2], "0"))
						{
							int error_fd = open(filev[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
							if (error_fd < 0)
							{
								perror("Error: No se pudo abrir el archivo de error.\n");
								exit(1);
							}
							dup2(error_fd, STDERR_FILENO);
							close(error_fd);
						}

						close(p[0]);
						close(p[1]);
						getCompleteCommand(argvv, 1);
						execvp(argv_execvp[0], argv_execvp);
						perror("Error: No se pudo ejecutar el mandato.\n");
						exit(1);
					}

					close(p[0]);
					close(p[1]);

					// Espera a que los hijos terminen

					if (!in_background)
					{
						wait(NULL);
						wait(NULL);
					}
					else
					{
						printf("[%d]\n", pid2);
					}
				}
			}
		}
	}
	return 0;
}