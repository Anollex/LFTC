#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "lexer.h"

#include <stdlib.h>

#include "utils.h"

Token *tokens; // single linked list of tokens
Token *lastTk; // the last token in list

int line = 1; // the current line in the input file

// adds a token to the end of the tokens list and returns it
// sets its code and line
Token *addTk(int code) {
	Token *tk = safeAlloc(sizeof(Token));
	tk->code = code;
	tk->line = line;
	tk->next = NULL;
	if (lastTk) {
		lastTk->next = tk;
	} else {
		tokens = tk;
	}
	lastTk = tk;
	return tk;
}

char *extract(const char *begin, const char *end) {
	// err("extract needs to be implemented");
	long len = end - begin;
	char *aux = (char *) safeAlloc(len + 1);
	memcpy(aux, begin, len);
	aux[len] = '\0';
	return aux;
}

char *processEscapes(const char *text) {
	int len = strlen(text);
	char *result = safeAlloc(len + 1);
	int j = 0;
	for (int i = 0; i < len; i++) {
		if (text[i] == '\\' && i + 1 < len) {
			i++;
			switch (text[i]) {
				case 'a': result[j++] = '\a';
					break;
				case 'b': result[j++] = '\b';
					break;
				case 'f': result[j++] = '\f';
					break;
				case 'n': result[j++] = '\n';
					break;
				case 'r': result[j++] = '\r';
					break;
				case 't': result[j++] = '\t';
					break;
				case 'v': result[j++] = '\v';
					break;
				case '\\': result[j++] = '\\';
					break;
				case '\'': result[j++] = '\'';
					break;
				case '"': result[j++] = '"';
					break;
				case '0': result[j++] = '\0';
					break;
				default: err("invalid escape sequence on line %d", line);
			}
		} else {
			result[j++] = text[i];
		}
	}
	result[j] = '\0';
	return result;
}

Token *tokenize(const char *pch) {
	const char *start;
	char *text;
	Token *tk;
	for (;;) {
		switch (*pch) {
			case ' ':
			case '\t': pch++;
				break;
			case '\r': // handles different kinds of newlines (Windows: \r\n, Linux: \n, MacOS, OS X: \r or \n)
				if (pch[1] == '\n')pch++;
			// fallthrough to \n
			case '\n':
				line++;
				pch++;
				break;
			case EOF:
			case '\0': addTk(END);
				return tokens;
			case ',': addTk(COMMA);
				pch++;
				break;
			case ';': addTk(SEMICOLON);
				pch++;
				break;
			case '(': addTk(LPAR);
				pch++;
				break;
			case ')': addTk(RPAR);
				pch++;
				break;
			case '[': addTk(LBRACKET);
				pch++;
				break;
			case ']': addTk(RBRACKET);
				pch++;
				break;
			case '{': addTk(LACC);
				pch++;
				break;
			case '}': addTk(RACC);
				pch++;
				break;
			case '.': addTk(DOT);
				pch++;
				break;
			case '+': addTk(ADD);
				pch++;
				break;
			case '-': addTk(SUB);
				pch++;
				break;
			case '*': addTk(MUL);
				pch++;
				break;
			case '/':
				if (pch[1] == '/') {
					pch += 2;
					for (; *pch != '\n' && *pch != '\r' && *pch != '\0'; pch++);
				} else {
					addTk(DIV);
					pch++;
				}
				break;
			case '=':
				if (pch[1] == '=') {
					addTk(EQUAL);
					pch += 2;
				} else {
					addTk(ASSIGN);
					pch++;
				}
				break;
			case '!':
				if (pch[1] == '=') {
					addTk(NOTEQ);
					pch += 2;
				} else {
					addTk(NOT);
					pch++;
				}
				break;
			case '<':
				if (pch[1] == '=') {
					addTk(LESSEQ);
					pch += 2;
				} else {
					addTk(LESS);
					pch++;
				}
				break;
			case '>':
				if (pch[1] == '=') {
					addTk(GREATEREQ);
					pch += 2;
				} else {
					addTk(GREATER);
					pch++;
				}
				break;
			case '&':
				if (pch[1] == '&') {
					addTk(AND);
					pch += 2;
				} else {
					err("bitwise AND is not supported on line %d", line);
				}
				break;
			case '|':
				if (pch[1] == '|') {
					addTk(OR);
					pch += 2;
				} else {
					err("bitwise OR is not supported on line %d", line);
				}
				break;
			case '\'':
				pch++;
				tk = addTk(CHAR);
				if (*pch == '\\') {
					pch++;
					switch (*pch) {
						case'a': tk->c = '\a';
							break;
						case'b': tk->c = '\b';
							break;
						case'f': tk->c = '\f';
							break;
						case'n': tk->c = '\n';
							break;
						case'r': tk->c = '\r';
							break;
						case't': tk->c = '\t';
							break;
						case'v': tk->c = '\v';
							break;
						case'\\': tk->c = '\\';
							break;
						case'\'': tk->c = '\'';
							break;
						case'\"': tk->c = '\"';
							break;
						case'0': tk->c = '\0';
							break;
						default: err("invalid escape sequence on line %d", line);
					}
					pch++;
					if (*pch != '\'') err("missing closing apostrophe on line %d", line);
					pch++;
				} else if (*pch != '\'' && pch[1] == '\'') {
					tk->c = *pch;
					pch += 2;
				} else {
					err("invalid char constant on line %d", line);
				}
				break;
			case '\"':
				pch++;
				for (start = pch; *pch != '\"'; pch++) {
					if (*pch == '\n' || *pch == '\r' || *pch == '\0') {
						err("constant string not closed on line %d", line);
					}
					if (*pch == '\\') {
						pch++;
						switch (*pch) {
							case'a':
							case'b':
							case'f':
							case'n':
							case'r':
							case't':
							case'v':
							case'\\':
							case'\'':
							case'\"':
							case'0':
								break;
							default: err("invalid escape sequence on line %d", line);
						}
					}
				}
				text = extract(start, pch);
				char *processedText = processEscapes(text);
				free(text);
				tk = addTk(STRING);
				tk->text = processedText;
				// tk->text=text;
				pch++;
				break;
			default:
				if (isdigit(*pch)) {
					for (start = pch; isdigit(*pch); pch++) {
					}
					if (*pch == '.' && isdigit(pch[1])) {
						pch++;
						for (; isdigit(*pch); pch++);
					}
					if (*pch == 'e' || *pch == 'E') {
						pch++;
						if (*pch == '+' || *pch == '-') pch++;
						if (!isdigit(*pch)) err("missing exponent digits");
						for (; isdigit(*pch); pch++);
					}
					text = extract(start, pch);
					if (strchr(text, '.') || strchr(text, 'e') || strchr(text, 'E')) {
						tk = addTk(DOUBLE);
						tk->d = strtod(text, NULL);
					} else {
						tk = addTk(INT);
						tk->i = (int) strtol(text, NULL, 10);
					}
					free(text);
				} else if (isalpha(*pch) || *pch == '_') {
					for (start = pch++; isalnum(*pch) || *pch == '_'; pch++) {
					}
					text = extract(start, pch);
					if (strcmp(text, "char") == 0) {
						addTk(TYPE_CHAR);
						free(text);
					} else if (strcmp(text, "double") == 0) {
						addTk(TYPE_DOUBLE);
						free(text);
					} else if (strcmp(text, "else") == 0) {
						addTk(ELSE);
						free(text);
					} else if (strcmp(text, "if") == 0) {
						addTk(IF);
						free(text);
					} else if (strcmp(text, "int") == 0) {
						addTk(TYPE_INT);
						free(text);
					} else if (strcmp(text, "return") == 0) {
						addTk(RETURN);
						free(text);
					} else if (strcmp(text, "struct") == 0) {
						addTk(STRUCT);
						free(text);
					} else if (strcmp(text, "void") == 0) {
						addTk(VOID);
						free(text);
					} else if (strcmp(text, "while") == 0) {
						addTk(WHILE);
						free(text);
					} else {
						tk = addTk(ID);
						tk->text = text;
					}
				} else err("invalid char: %c (%d)", *pch, *pch);
		}
	}
}

const char *tkCodeName(int code) {
	switch (code) {
		case ID: return "ID";
		case TYPE_CHAR: return "TYPE_CHAR";
		case TYPE_DOUBLE: return "TYPE_DOUBLE";
		case ELSE: return "ELSE";
		case IF: return "IF";
		case TYPE_INT: return "TYPE_INT";
		case RETURN: return "RETURN";
		case STRUCT: return "STRUCT";
		case VOID: return "VOID";
		case WHILE: return "WHILE";
		case INT: return "INT";
		case DOUBLE: return "DOUBLE";
		case CHAR: return "CHAR";
		case STRING: return "STRING";
		case COMMA: return "COMMA";
		case SEMICOLON: return "SEMICOLON";
		case LPAR: return "LPAR";
		case RPAR: return "RPAR";
		case LBRACKET: return "LBRACKET";
		case RBRACKET: return "RBRACKET";
		case LACC: return "LACC";
		case RACC: return "RACC";
		case END: return "END";
		case ADD: return "ADD";
		case SUB: return "SUB";
		case MUL: return "MUL";
		case DIV: return "DIV";
		case DOT: return "DOT";
		case AND: return "AND";
		case OR: return "OR";
		case NOT: return "NOT";
		case ASSIGN: return "ASSIGN";
		case EQUAL: return "EQUAL";
		case NOTEQ: return "NOTEQ";
		case LESS: return "LESS";
		case LESSEQ: return "LESSEQ";
		case GREATER: return "GREATER";
		case GREATEREQ: return "GREATEREQ";
		default: return "UNKNOWN";
	}
}

void showTokens(const Token *tokens) {
	for (const Token *tk = tokens; tk; tk = tk->next) {
		printf("%d\t%s", tk->line, tkCodeName(tk->code));
		switch (tk->code) {
			case ID:
				printf(":%s", tk->text);
				break;
			case INT:
				printf(":%d", tk->i);
				break;
			case DOUBLE:
				printf(":%g", tk->d);
				break;
			case CHAR:
				printf(":%c", tk->c);
				break;
			case STRING:
				printf(":%s", tk->text);
				break;
		}
		printf("\n");
	}
}
