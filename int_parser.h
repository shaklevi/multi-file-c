#ifndef ______INT_PARSER_H_______
#define ______INT_PARSER_H_______

/** \brief parses a c-string into an int
 *
 * \param s c-string to parse
 *
 * \param out pointer to int object to populate
 *
 * \returns `0` on error, `1` on success
 */
int parse_int(const char *s, int *out);

#endif // ______INT_PARSER_H_______