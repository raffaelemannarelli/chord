#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "chord_arg_parser.h"

error_t chord_parser(int key, char *arg, struct argp_state *state) {
	struct chord_arguments *args = state->input;
	error_t ret = 0;
	switch(key) {

	// --port, -p bind to port
	case 'p':
	{
		/* Validate that port is correct and a number, etc!! */
		uint16_t port = atoi(arg);
		if (0 /* port is invalid */) {
			argp_error(state, "Invalid option for a port, must be a number");
		}
		args->my_address.sin_family = AF_INET;
		args->my_address.sin_addr.s_addr = INADDR_ANY;
        args->my_address.sin_port = htons(port);
		break;
	}

    // --ja join chord node at address
	case 300:
	{
		/* validate that address parameter makes sense */
        args->join_address.sin_family = AF_INET;
        int success = inet_pton(AF_INET, arg, (void *)&(args->join_address.sin_addr.s_addr));
		if (success != 1) {
			argp_error(state, "Invalid address");
		}
		break;
	}

    // --jp join chord node at port
	case 301:
	{
		/* Validate that port is correct and a number, etc!! */
		uint16_t port = atoi(arg);
		if (0 /* port is invalid */) {
			argp_error(state, "Invalid option for a port, must be a number");
		}
        args->join_address.sin_port = htons(port);
		break;
	}

    // --sp stablize period
    case 400:
	{
        int ts_arg = atoi(arg);
        if (0 /*number is invalid*/) {
			argp_error(state, "Invalid option for a stablize period");
        } else {
            args->stablize_period = (uint16_t)ts_arg;
        }
        break;
	}

    // --ffp fix fingers period
    case 401:
	{
        int ts_arg = atoi(arg);
        if (0 /*number is invalid*/) {
			argp_error(state, "Invalid option for a fix fingers period");
        } else {
            args->fix_fingers_period = (uint16_t)ts_arg;
        }
        break;
	}

    // --cpp check precedessor period
    case 402:
	{
        int ts_arg = atoi(arg);
        if (0 /*number is invalid*/) {
			argp_error(state, "Invalid option for a check predecessor period");
        } else {
            args->check_predecessor_period = (uint16_t)ts_arg;
        }
        break;
	}

    // --successors, -r number of sucessors
    case 'r':
	{
        if (0 /*string is an invalid identification*/) {
			argp_error(state, "Invalid option for successor count");
        } else {
            args->num_successors = (uint8_t)atoi(arg);
        }
		break;
	}

	default:
		ret = ARGP_ERR_UNKNOWN;
		break;
	}
	return ret;
}

struct chord_arguments chord_parseopt(int argc, char *argv[]) {
	struct argp_option options[] = {
		{ "port", 'p', "port", 0, "The port that is being used at the chord node", 0},
		{ "sp", 400, "stabilize_period", 0, "The time between invocations of 'stablize'", 0},
		{ "ffp", 401, "fix_fingers_period", 0, "The time between invocations of 'fix_fingers'", 0},
		{ "cpp", 402, "check_predecessor_period", 0, "The time between invocations of 'check_predecessor'", 0},
		{ "successors", 'r', "successors", 0, "The number of successors maintained", 0},
		{ "ja", 300, "join_addr", 0, "The IP address the chord node to join", 0},
		{ "jp", 301, "join_port", 0, "The port that chord node we're joining is listening on", 0},
		{0}
	};

	struct argp argp_settings = { options, chord_parser, 0, 0, 0, 0, 0 };

	struct chord_arguments args;
	memset(&args, 0, sizeof(args));

	if (argp_parse(&argp_settings, argc, argv, 0, NULL, &args) != 0) {
		fprintf(stderr, "Error while parsing\n");
        exit(1);
	}

	// TODO: ADDITIONAL ARGUMENT VALIDATION

    return args;
}
