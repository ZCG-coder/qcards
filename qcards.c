#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#define getchar _getch
#endif

struct Lines
{
    char **lines;
    size_t nlines;
};

struct Lines g_lines = {NULL, 0};

/*
 * Structure of .qcard file
 * ========================
 * 0 | Question / word
 * 1 | Answer
 *
 * Even number line : Question
 * Odd number line  : Answer
 */

void setup_console()
{
#ifndef _WIN32
#include <termios.h>

    struct termios info;
    tcgetattr(0, &info);          /* get current terminal attirbutes; 0 is the file descriptor for stdin */
    info.c_lflag &= ~ICANON;      /* disable canonical mode */
    info.c_lflag &= ~ECHO;        /* disable echo */
    info.c_cc[VMIN] = 1;          /* wait until at least one keystroke available */
    info.c_cc[VTIME] = 0;         /* no timeout */
    tcsetattr(0, TCSANOW, &info); /* set immediately */
#else
#endif
}

int free_lines(size_t n, char **lines)
{
    if (lines == NULL)
        return 0;

    for (size_t i = 0; i < n; ++i)
    {
        free(lines[i]);
    }

    return 1;
}

void cleanup_and_exit(int sig)
{
    printf("\033[?1049l");
    fflush(stdout);

    if (g_lines.lines != NULL)
    {
        free_lines(g_lines.nlines, g_lines.lines);
        free(g_lines.lines);
    }
    exit(0);
}

size_t get_random_sz(size_t max)
{
    size_t result;
    size_t range = max + 1;

    /* If range is 0, it means max was SIZE_MAX, so the range is the entire size_t spectrum */
    if (range == 0)
    {
        arc4random_buf(&result, sizeof(result));
        return result;
    }

    /* Unbiased rejection sampling for 64-bit variables */
    size_t limit = ((size_t)-1) - (((size_t)-1) % range);
    do
    {
        arc4random_buf(&result, sizeof(result));
    } while (result >= limit);

    return result % range;
}

int get_lines_file(FILE *fp, size_t *sz)
{
    if (fp == NULL)
        return 0;

    size_t lines = 0;

    while (!feof(fp))
    {
        char ch = fgetc(fp);
        if (ch == '\n')
        {
            ++lines;
        }
    }
    fseek(fp, 0, SEEK_SET);
    *sz = lines;
    return 1;
}

int read_lines(FILE *fp, char **lines)
{
    if (lines == NULL)
        return 0;

    size_t idx = 0;
    fseek(fp, 0, SEEK_SET);
    size_t line_len = 0;
    char *buf = 0;
    while (getline(&buf, &line_len, fp) > 0)
    {
        lines[idx] = (char *)malloc(line_len * sizeof(char *));
        strncpy(lines[idx], buf, line_len + 1);
        ++idx;
    }
    if (buf)
        free(buf);
    return 1;
}

void main_loop(FILE *fp)
{
    /* Output format
     * 1 | QUESTION/ANSWER ###
     * 2 | ###
     * 3 |
     * 4 | (space) for answer / (r)ight / (w)rong / (q)uit
     */

    size_t i = 0;
    size_t max;

    int running = 1;
    int question = 1;

    srand(time(NULL));
    assert(get_lines_file(fp, &max));

    if (max % 2 != 1)
    {
        fprintf(stderr, "Expect even number of lines\n");
        goto end;
    }
    if (max == 0)
    {
        fprintf(stderr, "Expect lines\n");
        goto end;
    }

    setup_console();

    printf("\x1b[?1049h");
    g_lines.lines = (char **)malloc(sizeof(char *) * max);
    read_lines(fp, g_lines.lines);
    size_t curr_idx = get_random_sz(max / 2) * 2;

    for (; running > 0;)
    {
        printf("\033[2J\033[H");
        printf("[%s %zu]\n", question ? "QUESTION" : "ANSWER", i + 1);

        char *line = question ? g_lines.lines[curr_idx] : g_lines.lines[curr_idx + 1];

        printf("%s\n", line);

        char c = getchar();

        switch (c)
        {
        case ' ': {
            question = question ? 0 : 1;
            break;
        }

        case 'r': {
            ++i;
            curr_idx = get_random_sz(max / 2) * 2;
            question = 1;
            continue;
        }

        case 'w': {
            ++i;
            curr_idx = get_random_sz(max / 2) * 2;
            question = 1;
            continue;
        }

        case 'q': {
            running = 0;
            break;
        }

        default: {
            break;
        }
        }
    }

end:
    printf("\x1b[?1049l");
    free_lines(g_lines.nlines, g_lines.lines);
    free(g_lines.lines);
}

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        printf("Usage: %s [qcard file]\n", argv[0]);
        return 1;
    }

    char *file = argv[1];
    printf("Using %s\n", file);

    FILE *fp = fopen(file, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Error opening %s\n", file);
        return 1;
    }

    signal(SIGINT, cleanup_and_exit);
    main_loop(fp);
    fclose(fp);

    return 0;
}
