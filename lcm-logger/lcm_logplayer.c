#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

#include <lcm/lcm.h>


typedef struct logplayer logplayer_t;
struct logplayer
{
    lcm_t * lcm_in;
    lcm_t * lcm_out;
    int verbose;
};

void
handler (const lcm_recv_buf_t *rbuf, const char *channel, void *u)
{
    logplayer_t * l = u;

    if (l->verbose)
        printf ("%.3f Channel %-20s size %d\n", rbuf->recv_utime / 1000000.0,
                channel, rbuf->data_size);

    lcm_publish (l->lcm_out, channel, rbuf->data, rbuf->data_size);
}

static void
usage (char * cmd)
{
    fprintf (stderr, "\
Usage: %s [OPTION...] FILE\n\
  Reads packets from an LCM log file and publishes them to LCM.\n\
\n\
Options:\n\
  -v, --verbose       Print information about each packet.\n\
  -s, --speed=NUM     Playback speed multiplier.  Default is 1.0.\n\
  -p, --provider=PRV  LCM network provider where packets should be published.\n\
                      Default is \"udpm://?transmit_only=true\"\n\
  -e, --regexp=EXPR   POSIX regular expression of channels to play.\n\
  \n", cmd);
}

int
main(int argc, char ** argv)
{
    logplayer_t l;
    double speed = 1.0;
    char c;
    char * provider = NULL;
    char * expression = NULL;
    struct option long_opts[] = { 
        { "help", no_argument, 0, 'h' },
        { "provider", required_argument, 0, 'p' },
        { "speed", required_argument, 0, 's' },
        { "verbose", no_argument, 0, 'v' },
        { "regexp", required_argument, 0, 'e' },
        { 0, 0, 0, 0 }
    };

    memset (&l, 0, sizeof (logplayer_t));
    while ((c = getopt_long (argc, argv, "hp:s:ve:", long_opts, 0)) >= 0)
    {
        switch (c) {
            case 's':
                speed = strtod (optarg, NULL);
                break;
            case 'p':
                provider = strdup (optarg);
                break;
            case 'v':
                l.verbose = 1;
                break;
            case 'e':
                expression = strdup (optarg);
                break;
            case 'h':
            default:
                usage (argv[0]);
                return 1;
        };
    }

    if (optind != argc - 1) {
        usage (argv[0]);
        return 1;
    }

    char * file = argv[optind];
    printf ("Using playback speed %f\n", speed);
    if (!provider)
        provider = strdup ("udpm://?transmit_only=true");
    if (!expression)
        expression = strdup (".*");

    char url_in[strlen(file) + 64];
    sprintf (url_in, "file://%s?speed=%f", argv[optind], speed);
    l.lcm_in = lcm_create (url_in);
    if (!l.lcm_in) {
        fprintf (stderr, "Error: Failed to open %s\n", file);
        return 1;
    }

    l.lcm_out = lcm_create (provider);
    if (!l.lcm_out) {
        fprintf (stderr, "Error: Failed to create %s\n", provider);
        return 1;
    }

    lcm_subscribe (l.lcm_in, expression, handler, &l);

    while (!lcm_handle (l.lcm_in));

    lcm_destroy (l.lcm_in);
    lcm_destroy (l.lcm_out);
    free (provider);
    free (expression);
}
