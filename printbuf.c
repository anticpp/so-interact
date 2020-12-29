/* Print buffer
 * printable => char
 * '\r'  => '\r'
 * '\n'  => '\n'
 * other => {HEX}
 * .... (to be added)
 */
static void printbuf(FILE *s, char *buf, size_t n) {
    char c;
    for(int i=0; i<n; i++) {
        c = buf[i];
        if( isprint(c) ) {
            fprintf(s, "%c", c);          
        } else if( c=='\r' ) {
            fprintf(s, "\\r");
        } else if( c=='\n' ) {
            fprintf(s, "\\n");
        } else {
            fprintf(s, "[%X]", c);
        }
    }
}
