#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef struct __string_vec
{
	char **data;
	size_t length;
	size_t allocated;
} string_vec;

string_vec create_string_vec()
{
	string_vec v = {NULL, 0, 0};
	return v;
}

void append_string_vec(string_vec *v, char *value)
{
	if (v->allocated == v->length)
	{
		size_t new_allocated = v->allocated == 0 ? 1 : v->allocated * 2;
		char **tmp = realloc(v->data, sizeof(char *) * new_allocated);
		if (!tmp)
		{
			perror("Realloc on 'string_vec *v' failed");
			exit(1);
		}
		v->data = tmp;
		v->allocated = new_allocated;
	}

	if (value == NULL)
	{
		v->data[v->length++] = NULL;
	}
	else
	{
		v->data[v->length++] = strdup(value);
		if (!v->data[v->length - 1])
		{
			perror("strdup failed");
			exit(1);
		}
	}
}

void free_string_vec(string_vec *v)
{
	if (!v->data)
	{
		v->length = v->allocated = 0;
		return;
	}
	for (size_t i = 0; i < v->length; ++i)
	{
		free(v->data[i]);
	}
	free(v->data);
	v->data = NULL;
	v->length = 0;
	v->allocated = 0;
}

char *trim(char *str)
{
	char *end;

	while (isspace((unsigned char)*str))
		str++;

	if (*str == 0)
		return str;

	end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end))
		end--;
	end[1] = '\0';

	return str;
}

string_vec split(char *line, const char *delim)
{
	string_vec result = create_string_vec();
	char *token;
	while ((token = strsep(&line, delim)) != NULL)
	{
		token = trim(token);
		if (strlen(token) > 0)
		{
			append_string_vec(&result, token);
		}
	}
	return result;
}

void print_error()
{
	char error_message[30] = "An error has occurred\n";
	write(STDERR_FILENO, error_message, strlen(error_message));
}

void cd(string_vec *args)
{
	if (args->length != 2)
		print_error();
	else if (chdir(args->data[1]) != 0)
		print_error();
}

void set_path(string_vec *args, string_vec *path)
{
	if (path->data)
		free_string_vec(path);

	*path = create_string_vec();
	if (args->length < 2)
	{
		return;
	}
	for (size_t i = 1; i < args->length; i++)
		append_string_vec(path, strdup(args->data[i]));
}

void execute_external(string_vec *args, string_vec *path, char *output_file)
{
	for (size_t i = 0; i < path->length; i++)
	{
		char candidate[PATH_MAX];
		snprintf(candidate, sizeof(candidate), "%s/%s", path->data[i], args->data[0]);
		if (!access(candidate, X_OK))
		{
			if (output_file)
			{
				int fd = open(output_file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

				if (fd < 0)
				{
					print_error();
					exit(1);
				}

				dup2(fd, 1);
				dup2(fd, 2);

				close(fd);
			}
			append_string_vec(args, NULL);
			execv(candidate, args->data);
			print_error();
			exit(1);
		}
	}
	print_error();
	exit(1);
}

void runtime(FILE *input_stream)
{
	string_vec path = create_string_vec();
	append_string_vec(&path, "/bin");
	while (1)
	{
		if (input_stream == stdin)
		{
			fprintf(stderr, "witsshell> ");
			fflush(stdout);
		}

		char *line = NULL;
		size_t line_len = 0;

		if (getline(&line, &line_len, input_stream) != -1)
		{
			line[strcspn(line, "\n")] = '\0';
			char *trimmed_line = trim(line);

			if (strlen(trimmed_line) == 0)
			{
				free(line);
				continue;
			}

			string_vec commands = split(line, "&");
			pid_t *pids = malloc(sizeof(pid_t) * commands.length);
			size_t pid_count = 0;
			for (size_t i = 0; i < commands.length; i++)
			{
				char *cmd_line = commands.data[i];
				char *cmd_file = NULL;
				char *redir = strchr(cmd_line, '>');

				string_vec output_redirect = split(cmd_line, ">");
				if (redir)
				{
					if (output_redirect.length == 2)
					{
						string_vec file_tokens = split(output_redirect.data[1], " ");
						if (file_tokens.length != 1)
						{
							print_error();
							free_string_vec(&file_tokens);
							free_string_vec(&output_redirect);
							continue;
						}
						cmd_file = strdup(file_tokens.data[0]);
						free_string_vec(&file_tokens);
					}
					else
					{
						print_error();
						free_string_vec(&output_redirect);
						continue;
					}
				}

				string_vec args = split(output_redirect.data[0], " ");
				free_string_vec(&output_redirect);

				if (args.length == 0)
				{
					free_string_vec(&args);
					free(cmd_file);
				}

				if (!strcmp(args.data[0], "exit"))
				{
					if (args.length == 1)
						exit(0);
					print_error();
				}
				else if (!strcmp(args.data[0], "cd"))
				{
					cd(&args);
				}
				else if (!strcmp(args.data[0], "path"))
				{
					set_path(&args, &path);
				}
				else
				{
					pid_t pid = fork();
					if (pid == 0)
					{
						execute_external(&args, &path, cmd_file);
						exit(0);
					}
					else if (pid > 0)
					{
						pids[pid_count++] = pid;
					}
					else
						print_error();
				}

				free_string_vec(&output_redirect);
				free_string_vec(&args);
				free(cmd_file);
			}
			for (size_t i = 0; i < pid_count; i++)
				waitpid(pids[i], NULL, 0);

			free(line);
			free(pids);
			free_string_vec(&commands);
		}
		else
		{
			exit(0);
		}
	}
}

int main(int argc, char **argv)
{
	FILE *input_stream;

	if (argc == 1)
	{
		input_stream = stdin;
	}
	else if (argc == 2)
	{
		input_stream = fopen(argv[1], "r");
		if (!input_stream)
		{
			print_error();
			exit(1);
		}
	}
	else
	{
		print_error();
		return 1;
	}

	runtime(input_stream);
	return 0;
}