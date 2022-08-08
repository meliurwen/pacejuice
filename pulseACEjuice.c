
// Compile: gcc pulseaudio-device-list.c -o pulseaudio-device-list $(pkg-config libpulse --cflags --libs)

#include <pulse/pulseaudio.h>
#include <stdio.h>
#include <string.h>

// Define our pulse audio loop and connection variables
pa_mainloop *pa_ml;
pa_mainloop_api *pa_ml_api;
pa_operation *pa_op;
pa_context *pa_ctx;


const char* default_sink_name;
const char* default_source_name;
uint32_t default_sink_index;
uint32_t default_source_index;

int mode = 0;

void context_state_callback(pa_context *ctx, void *userdata);
int pa_initialize_connection();

/**
* Exits the main loop with the specified return code.
*/
void quit(int ret) {

    if (pa_ml_api) {
        pa_ml_api->quit(pa_ml_api, ret);
    }

    if (pa_ctx) {
        pa_context_disconnect(pa_ctx);
        pa_context_unref(pa_ctx);
        pa_ctx = NULL;
    }

    if (pa_ml) {
        pa_mainloop_free(pa_ml);
        pa_ml = NULL;
        pa_ml_api = NULL;
    }

    exit(ret);
}

/* Signal Handler for SIGINT */
void sigintHandler(int sig_num) {
    fprintf(stderr, "Keystroke CTRL+C signal detected. Terminating...\n");
    quit(0);
}

void print_sink(int mute, float volume, uint32_t index, const char* name){
  if (index == default_sink_index) {
    printf("SINKVOL %s %.0f %u %s\n", mute ? "muted" : "unmuted", volume, index, name);
    fflush(stdout);
  }
}

/*
* Called when the requested sink information is ready.
*/
static void sink_info_callback(pa_context *ctx, const pa_sink_info *info, int eol, void *userdata) {
    if (info != NULL) {
        float volume = ((float)pa_cvolume_avg(&info->volume) / PA_VOLUME_NORM) * 100.0F;
        int mute = info->mute;
        print_sink(mute, volume, info->index, info->name);
        //printf("SINKVOL %s %.0f %u %s\n", mute ? "muted" : "unmuted", volume, info->index, info->name);
        //fflush(stdout);
    }
}
static void sink_info_callback_default(pa_context *ctx, const pa_sink_info *info, int eol, void *userdata) {
    if (info != NULL) {
        float volume = ((float)pa_cvolume_avg(&info->volume) / PA_VOLUME_NORM) * 100.0F;
        int mute = info->mute;
        default_sink_index = info->index;
        print_sink(mute, volume, info->index, info->name);
        //printf("SINKVOL %s %.0f %u %s\n", mute ? "muted" : "unmuted", volume, info->index, info->name);
        //fflush(stdout);
    }
}

void print_source(int mute, float volume, uint32_t index, const char* name){
    if (index == default_source_index) {
        printf("SOURCEVOL %s %.0f %u %s\n", mute ? "muted" : "unmuted", volume, index, name);
        fflush(stdout);
    }
}

/*
* Called when the requested source information is ready.
*/
static void source_info_callback(pa_context *ctx, const pa_source_info *info, int eol, void *userdata) {
    if (info != NULL) {
        float volume = ((float)pa_cvolume_avg(&info->volume) / PA_VOLUME_NORM) * 100.0F;
        int mute = info->mute;
        print_source(mute, volume, info->index, info->name);
        //printf("SOURCEVOL %s %.0f %u %s\n", mute ? "muted" : "unmuted", volume, info->index, info->name);
        //fflush(stdout);
    }
}
static void source_info_callback_default(pa_context *ctx, const pa_source_info *info, int eol, void *userdata) {
    if (info != NULL) {
        float volume = ((float)pa_cvolume_avg(&info->volume) / PA_VOLUME_NORM) * 100.0F;
        int mute = info->mute;
        default_source_index = info->index;
        print_source(mute, volume, info->index, info->name);
        //printf("SOURCEVOL %s %.0f %u %s\n", mute ? "muted" : "unmuted", volume, info->index, info->name);
        //fflush(stdout);
    }
}

/*
* Called when the requested information on the server is ready. This is
* used to find the default PulseAudio sink.
*/
static void server_info_callback(pa_context *ctx, const pa_server_info *info, void *userdata) {
    default_sink_name = info->default_sink_name;
    default_source_name = info->default_source_name;
    //printf("DEFAULT_SINK_NAME %s\n", default_sink_name);
    //printf("DEFAULT_SOURCE_NAME %s\n", default_source_name);
    //fflush(stdout);
    pa_context_get_sink_info_by_name(ctx, default_sink_name, sink_info_callback_default, userdata);
    pa_context_get_source_info_by_name(ctx, default_source_name, source_info_callback_default, userdata);
}

/*
* Called when an event we subscribed to occurs.
*/
static void subscribe_callback(pa_context *ctx, pa_subscription_event_type_t type, uint32_t index, void* userdata) {

    pa_operation *op = NULL;
    //printf("%" PRIu32 "\n", index);

    switch (type & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) {
        case PA_SUBSCRIPTION_EVENT_SINK:
            op = pa_context_get_sink_info_by_index(ctx, index, sink_info_callback, userdata);
            break;
        case PA_SUBSCRIPTION_EVENT_SOURCE:
            op = pa_context_get_source_info_by_index(ctx, index, source_info_callback, userdata);
            break;
        case PA_SUBSCRIPTION_EVENT_SERVER:
            op = pa_context_get_server_info(ctx, server_info_callback, userdata);
            break;
        default:
            assert(0); // Got event we aren't expecting.
            break;
    }

    if (op){
        pa_operation_unref(op);
    }
}

// This callback gets called when our context changes state.  We really only
// care about when it's ready or if it has failed
void context_state_callback(pa_context *ctx, void *userdata) {
	pa_context_state_t state;
	int *pa_ready = userdata;

	state = pa_context_get_state(ctx);
	switch  (state) {
		// There are just here for reference
		case PA_CONTEXT_CONNECTING:
		case PA_CONTEXT_AUTHORIZING:
		case PA_CONTEXT_SETTING_NAME:
            break;
		case PA_CONTEXT_READY:
			*pa_ready = 1;
            fprintf(stderr, "PulseAudio connection established.\n");
            switch (mode) {
                case 0: 
                    break;
                case 1: 
                    fprintf(stderr, "Subscribe mode enabled.\n");
                    pa_context_get_server_info(ctx, server_info_callback, userdata);
                    // Subscribe to sink events from the server. This is how we get
                    // volume change notifications from the server.
                    pa_context_set_subscribe_callback(ctx, subscribe_callback, userdata);
                    pa_context_subscribe(ctx, (pa_subscription_mask_t)(PA_SUBSCRIPTION_MASK_SERVER | PA_SUBSCRIPTION_MASK_SINK | PA_SUBSCRIPTION_MASK_SOURCE), NULL, NULL);
                    break;
                default: 
                    break;
            }
			break;
		case PA_CONTEXT_TERMINATED:
            // We couldn't get a connection to the server, so exit out
            fprintf(stderr, "PulseAudio connection terminated.\n");
			*pa_ready = 2;
            quit(0);
			break;
        case PA_CONTEXT_FAILED:
		default:
            fprintf(stderr, "Connection failure: %s\n", pa_strerror(pa_context_errno(ctx)));
            quit(1);
			break;
	}
}

int pa_initialize_connection() {

    // We'll need these state variables to keep track of our requests
    int state = 0;
    int pa_ready = 0;

    // Create a mainloop API and connection to the default server
    pa_ml = pa_mainloop_new();
    if (!pa_ml) {
        fprintf(stderr, "pa_mainloop_new() failed.\n");
        return -1;
    }

    pa_ml_api = pa_mainloop_get_api(pa_ml);
    if (pa_signal_init(pa_ml_api) != 0) {
        fprintf(stderr, "pa_signal_init() failed\n");
        return -1;
    }

    pa_ctx = pa_context_new(pa_ml_api, "PulseAudio ACEjuice C Plugin");
    if (!pa_ctx) {
        fprintf(stderr, "pa_context_new() failed\n");
        return -1;
    }

    // This function connects to the pulse server
    if (pa_context_connect(pa_ctx, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL) < 0) {
        fprintf(stderr, "pa_context_connect() failed: %s\n", pa_strerror(pa_context_errno(pa_ctx)));
        return -1;
    }

    // This function connects to the pulse server
    pa_context_connect(pa_ctx, NULL, 0, NULL);

    // This function defines a callback so the server will tell us it's state.
    // Our callback will wait for the state to be ready.  The callback will
    // modify the variable to 1 so we know when we have a connection and it's
    // ready.
    // If there's an error, the callback will set pa_ready to 2
    pa_context_set_state_callback(pa_ctx, context_state_callback, &pa_ready);

    int ret = 0;

	switch (mode) {
		case 0: 
			// We can't do anything until PA is ready, so just wait
            while (pa_ready == 0) {
                //printf("%u\n",pa_ready);
	            pa_mainloop_iterate(pa_ml, 1, NULL);
            }
		    break;
		case 1: 
            if (pa_mainloop_run(pa_ml, &ret) < 0) {
                fprintf(stderr, "pa_mainloop_run() failed.\n");
            }
		    break;
		default: 
		    fprintf(stderr, "Unknown mode. Terminating...\n");
            quit(1);
		    break;
	}
    return ret;
}


void args(int argc, char *argv[]) {
    for (int x=1; x<argc; ++x) {
        //printf("ARG %s\n", argv[x]);
        if (!strcmp(argv[x], "--subscribe")) {
            mode = 1;
            while((x+1 < argc) && !(strncmp(argv[x+1], "--", 2) == 0)) {
                x++;
                if (!strcmp(argv[x], "sink")){
                    if((x+1 < argc) && !(strncmp(argv[x+1], "--", 2) == 0)) {
                        x++;
                        printf("SIIIIINK\n");
                    } else {
                        fprintf(stderr, "Badly formatted --subscribe argument \"%s\". Ignoring it...\n", argv[x]);
                    }
                } else if (!strcmp(argv[x], "source")) {
                    if((x+1 < argc) && !(strncmp(argv[x+1], "--", 2) == 0)) {
                        x++;
                        printf("SOOOURCE\n");
                    } else {
                        fprintf(stderr, "Badly formatted --subscribe argument \"%s\". Ignoring it...\n", argv[x]);
                    }
                } else {
                    fprintf(stderr, "Unknown --subscribe argument \"%s\". Ignoring it...\n", argv[x]);
                }
            }
        } else if (!strcmp(argv[x], "--help")) {
            printf("Help still TODO\n");
            quit(0);
        } else {
            fprintf(stderr, "Unknown argument \"%s\". Ignoring it...\n", argv[x]);
        }
    }
}

int main(int argc, char *argv[]) {

    args(argc, argv);

    /* Set the SIGINT (Ctrl-C) signal handler to sigintHandler
    Refer http://en.cppreference.com/w/c/program/signal */
    signal(SIGINT, sigintHandler);

    if (pa_initialize_connection() < 0) {
	    fprintf(stderr, "failed to get device list\n");
	    quit(1);
    }
    quit(0);
}

//@DEFAULT_SOURCE@
//@DEFAULT_SINK@
