typedef int (*command_handler)(int argc, char **args);

typedef struct {
    const char *name;
    int argc; // Arguments required

    /* For help message */
    const char *help_args;   // 0 or "" if no additional arguments
    const char *help_message;

    command_handler handler;
} command_t; 

static command_t* lookup_command(const char* name, command_t *commands, int size) {
    command_t *cmd = 0;
    for( int i=0; i<size; i++ ) {
        if( strcasecmp(name, commands[i].name)!=0 ) {
            continue;
        }
        cmd = &commands[i];
        break;
    }
    return cmd;
}

