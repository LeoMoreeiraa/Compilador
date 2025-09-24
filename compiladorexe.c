/*
 * TRABALHO DE COMPILADORES - FASE 1: Análise Léxica e Sintática
 *
 * Implementação de um compilador para a linguagem PasKenzie,
 * conforme as especificações do trabalho e alinhado com o código de referência.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

// Enumeração de todos os tokens da linguagem PasKenzie
typedef enum {
    ERRO, IDENTIFICADOR, NUMERO, COMENTARIO, DIV, OR, AND, NOT, IF, THEN, ELSE,
    WHILE, DO, BEGIN, END, READ, WRITE, VAR, PROGRAM, TRUE_TOKEN, FALSE_TOKEN,
    CHAR, INTEGER, BOOLEAN, PONTO_VIRGULA, VIRGULA, DOIS_PONTOS, ASTERISCO,
    ABRE_PAR, FECHA_PAR, MAIS, MENOS, PONTO, IGUAL, ATRIBUICAO,
    MENOR, MAIOR, MENOR_IGUAL, MAIOR_IGUAL, NEGACAO, CONSTCHAR, CONSTINT, EOS
} TAtomo;

// Tabela de strings para nomes de átomos
const char *TAtomo_str[] = {
    "ERRO", "identifier", "numero", "comentario", "div", "or", "and", "not", "if", "then", "else",
    "while", "do", "begin", "end", "read", "write", "var", "program", "true", "false",
    "char", "integer", "boolean", "ponto_virgula", "virgula", "dois_pontos", "asterisco",
    "abre_par", "fecha_par", "mais", "menos", "ponto", "igual", "atribuicao",
    "menor", "maior", "menor_igual", "maior_igual", "negacao", "constchar", "constint", "EOS"
};

// Estrutura para armazenar informações do átomo
typedef struct {
    TAtomo atomo;
    int linha;
    union {
        int numero;
        char id[16];
        char ch;
    } atributo;
} TInfoAtomo;

// Variáveis globais
char *buffer;
int nLinha;
TInfoAtomo lookahead;

// Protótipos das Funções
TInfoAtomo obter_atomo();
void reconhece_numero(TInfoAtomo *infoAtomo);
void reconhece_id(TInfoAtomo *infoAtomo);
void reconhece_constchar(TInfoAtomo *infoAtomo);
void reconhece_qualquer(TInfoAtomo *infoAtomo);
void reconhece_comentario(TInfoAtomo *infoAtomo);
void consome(TAtomo esperado);
const char* nome_atomo(TAtomo a);
void program();
void block();
void variable_declaration_part();
void variable_declaration();
void type();
void statement_part();
void statement();
void assignment_statement();
void read_statement();
void write_statement();
void if_statement();
void while_statement();
void expression();
void simple_expression();
void term();
void factor();
void relational_operator();
void adding_operator();
void multiplying_operator();

// =================================================================
// FUNÇÃO PRINCIPAL
// =================================================================
int main() {
    FILE *f = fopen("compilador.txt", "r");
    if (f == NULL) {
        perror("Erro ao abrir o arquivo 'compilador.txt'");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long tamanho = ftell(f);
    fseek(f, 0, SEEK_SET);

    buffer = (char*)malloc(tamanho + 1);
    if(buffer == NULL){
        printf("Erro ao alocar memoria.\n");
        fclose(f);
        return 1;
    }
    fread(buffer, 1, tamanho, f);
    buffer[tamanho] = '\0';
    fclose(f);

    nLinha = 1;

    lookahead = obter_atomo();
    program();

    consome(EOS);
    printf("%d linhas analisadas, programa sintaticamente correto\n", nLinha);

    free(buffer);
    return 0;
}

// =================================================================
// ANALISADOR LÉXICO
// =================================================================

const char* nome_atomo(TAtomo a) {
    if (a >= 0 && a <= EOS) return TAtomo_str[a];
    return "TOKEN_DESCONHECIDO";
}

TInfoAtomo obter_atomo() {
    TInfoAtomo infoAtomo;
    infoAtomo.atomo = ERRO;

    while (*buffer == ' ' || *buffer == '\t' || *buffer == '\n' || *buffer == '\r' ) {
        if (*buffer == '\n') nLinha++;
        buffer++;
    }

    infoAtomo.linha = nLinha;

    if (*buffer == '\0') {
        infoAtomo.atomo = EOS;
        return infoAtomo;
    }
    if (*buffer == '(' && *(buffer + 1) == '*') {
        reconhece_comentario(&infoAtomo);
        return infoAtomo;
    }
    if (isdigit(*buffer)) {
        reconhece_numero(&infoAtomo);
        return infoAtomo;
    }
    if (isalpha(*buffer) || *buffer == '_') {
        reconhece_id(&infoAtomo);
        return infoAtomo;
    }
    if (*buffer == '\'') {
        reconhece_constchar(&infoAtomo);
        return infoAtomo;
    }

    reconhece_qualquer(&infoAtomo);
    return infoAtomo;
}

void reconhece_comentario(TInfoAtomo *infoAtomo) {
    buffer += 2; // pula "(*"
    while (*buffer != '\0') {
        if (*buffer == '\n') nLinha++;
        if (*buffer == '*' && *(buffer + 1) == ')') {
            buffer += 2; // pula "*)"
            infoAtomo->atomo = COMENTARIO;
            return;
        }
        buffer++;
    }
    printf("# %d: erro lexico, comentario nao fechado.\n", infoAtomo->linha);
    exit(1);
}

void reconhece_numero(TInfoAtomo *infoAtomo) {
    char *ini_lexema = buffer;
    char lexema_base[50], lexema_expoente[10];
    int sinal_expoente = 1;

    while(isdigit(*buffer)) buffer++;

    int tamanho_base = buffer - ini_lexema;
    strncpy(lexema_base, ini_lexema, tamanho_base);
    lexema_base[tamanho_base] = '\0';

    double valor_final = atof(lexema_base);

    if (tolower(*buffer) == 'd') {
        buffer++;
        if (*buffer == '-') {
            sinal_expoente = -1;
            buffer++;
        } else if (*buffer == '+') {
            buffer++;
        }

        if (!isdigit(*buffer)) {
             printf("# %d: erro lexico, 'd' de expoente deve ser seguido por um digito.\n", infoAtomo->linha);
             exit(1);
        }

        char *ini_expoente = buffer;
        while(isdigit(*buffer)) buffer++;
        int tamanho_expoente = buffer - ini_expoente;
        strncpy(lexema_expoente, ini_expoente, tamanho_expoente);
        lexema_expoente[tamanho_expoente] = '\0';

        int expoente = atoi(lexema_expoente);
        valor_final = valor_final * pow(10, expoente * sinal_expoente);
    }

    infoAtomo->atributo.numero = (int)valor_final;
    infoAtomo->atomo = CONSTINT;
}

void reconhece_id(TInfoAtomo *infoAtomo){
    char *ini_lexema = buffer;
    while(isalnum(*buffer) || *buffer == '_') buffer++;

    int tamanho = buffer - ini_lexema;
    if (tamanho > 15) {
        printf("# %d: erro lexico, identificador com mais de 15 caracteres.\n", infoAtomo->linha);
        exit(1);
    }

    strncpy(infoAtomo->atributo.id, ini_lexema, tamanho);
    infoAtomo->atributo.id[tamanho] = '\0';

    if (strcmp(infoAtomo->atributo.id, "div") == 0) infoAtomo->atomo = DIV;
    else if (strcmp(infoAtomo->atributo.id, "or") == 0) infoAtomo->atomo = OR;
    else if (strcmp(infoAtomo->atributo.id, "and") == 0) infoAtomo->atomo = AND;
    else if (strcmp(infoAtomo->atributo.id, "not") == 0) infoAtomo->atomo = NOT;
    else if (strcmp(infoAtomo->atributo.id, "if") == 0) infoAtomo->atomo = IF;
    else if (strcmp(infoAtomo->atributo.id, "then") == 0) infoAtomo->atomo = THEN;
    else if (strcmp(infoAtomo->atributo.id, "else") == 0) infoAtomo->atomo = ELSE;
    else if (strcmp(infoAtomo->atributo.id, "while") == 0) infoAtomo->atomo = WHILE;
    else if (strcmp(infoAtomo->atributo.id, "do") == 0) infoAtomo->atomo = DO;
    else if (strcmp(infoAtomo->atributo.id, "begin") == 0) infoAtomo->atomo = BEGIN;
    else if (strcmp(infoAtomo->atributo.id, "end") == 0) infoAtomo->atomo = END;
    else if (strcmp(infoAtomo->atributo.id, "read") == 0) infoAtomo->atomo = READ;
    else if (strcmp(infoAtomo->atributo.id, "write") == 0) infoAtomo->atomo = WRITE;
    else if (strcmp(infoAtomo->atributo.id, "var") == 0) infoAtomo->atomo = VAR;
    else if (strcmp(infoAtomo->atributo.id, "program") == 0) infoAtomo->atomo = PROGRAM;
    else if (strcmp(infoAtomo->atributo.id, "true") == 0) infoAtomo->atomo = TRUE_TOKEN;
    else if (strcmp(infoAtomo->atributo.id, "false") == 0) infoAtomo->atomo = FALSE_TOKEN;
    else if (strcmp(infoAtomo->atributo.id, "char") == 0) infoAtomo->atomo = CHAR;
    else if (strcmp(infoAtomo->atributo.id, "integer") == 0) infoAtomo->atomo = INTEGER;
    else if (strcmp(infoAtomo->atributo.id, "boolean") == 0) infoAtomo->atomo = BOOLEAN;
    else infoAtomo->atomo = IDENTIFICADOR;
}

void reconhece_constchar(TInfoAtomo *infoAtomo){
    buffer++; // consome o '
    if (*buffer != '\0' && *(buffer + 1) == '\'') {
        infoAtomo->atributo.ch = *buffer;
        buffer += 2;
        infoAtomo->atomo = CONSTCHAR;
    } else {
        printf("# %d: erro lexico, constante char mal formada.\n", infoAtomo->linha);
        exit(1);
    }
}

void reconhece_qualquer(TInfoAtomo *infoAtomo){
    switch(*buffer) {
        case '+': infoAtomo->atomo = MAIS; buffer++; break;
        case '-': infoAtomo->atomo = MENOS; buffer++; break;
        case '*': infoAtomo->atomo = ASTERISCO; buffer++; break;
        case ';': infoAtomo->atomo = PONTO_VIRGULA; buffer++; break;
        case ',': infoAtomo->atomo = VIRGULA; buffer++; break;
        case '.': infoAtomo->atomo = PONTO; buffer++; break;
        case '(': infoAtomo->atomo = ABRE_PAR; buffer++; break;
        case ')': infoAtomo->atomo = FECHA_PAR; buffer++; break;
        case '=': infoAtomo->atomo = IGUAL; buffer++; break;
        case ':':
            if (*(buffer + 1) == '=') { infoAtomo->atomo = ATRIBUICAO; buffer += 2; }
            else { infoAtomo->atomo = DOIS_PONTOS; buffer++; }
            break;
        case '<':
            if (*(buffer + 1) == '=') { infoAtomo->atomo = MENOR_IGUAL; buffer += 2; }
            else if (*(buffer + 1) == '>') { infoAtomo->atomo = NEGACAO; buffer += 2; }
            else { infoAtomo->atomo = MENOR; buffer++; }
            break;
        case '>':
            if (*(buffer + 1) == '=') { infoAtomo->atomo = MAIOR_IGUAL; buffer += 2; }
            else { infoAtomo->atomo = MAIOR; buffer++; }
            break;
        default:
            printf("# %d: erro lexico, simbolo desconhecido: %c\n", infoAtomo->linha, *buffer);
            exit(1);
    }
}

// =================================================================
// ANALISADOR SINTÁTICO
// =================================================================
void consome(TAtomo esperado) {
     while(lookahead.atomo==COMENTARIO){
        printf("# %d:comentario\n", lookahead.linha);
        lookahead = obter_atomo();
        
    }
    if (lookahead.atomo == esperado) {
        // Imprime o token ANTES de obter o próximo
        switch (esperado) {
            case PROGRAM: printf("# %d:program\n", lookahead.linha); break;
            case IDENTIFICADOR: printf("# %d:identifier : %s\n", lookahead.linha, lookahead.atributo.id); break;
            case VAR: printf("# %d:var\n", lookahead.linha); break;
            case INTEGER: printf("# %d:integer\n", lookahead.linha); break;
            case CHAR: printf("# %d:char\n", lookahead.linha); break;
            case BOOLEAN: printf("# %d:boolean\n", lookahead.linha); break;
            case BEGIN: printf("# %d:begin\n", lookahead.linha); break;
            case END: printf("# %d:end\n", lookahead.linha); break;
            case READ: printf("# %d:read\n", lookahead.linha); break;
            case WRITE: printf("# %d:write\n", lookahead.linha); break;
            case IF: printf("# %d:if\n", lookahead.linha); break;
            case THEN: printf("# %d:then\n", lookahead.linha); break;
            case ELSE: printf("# %d:else\n", lookahead.linha); break;
            case WHILE: printf("# %d:while\n", lookahead.linha); break;
            case DO: printf("# %d:do\n", lookahead.linha); break;
            case TRUE_TOKEN: printf("# %d:true\n", lookahead.linha); break;
            case FALSE_TOKEN: printf("# %d:false\n", lookahead.linha); break;

            case PONTO_VIRGULA: printf("# %d:ponto_virgula\n", lookahead.linha); break;
            case VIRGULA: printf("# %d:virgula\n", lookahead.linha); break;
            case DOIS_PONTOS: printf("# %d:dois_pontos\n", lookahead.linha); break;
            case PONTO: printf("# %d:ponto\n", lookahead.linha); break;
            case ABRE_PAR: printf("# %d:abre_par\n", lookahead.linha); break;
            case FECHA_PAR: printf("# %d:fecha_par\n", lookahead.linha); break;

            case MAIS: printf("# %d:mais\n", lookahead.linha); break;
            case MENOS: printf("# %d:menos\n", lookahead.linha); break;
            case ASTERISCO: printf("# %d:asterisco\n", lookahead.linha); break;
            case BARRA: printf("# %d:barra\n", lookahead.linha); break;
            case IGUAL: printf("# %d:igual\n", lookahead.linha); break;
            case ATRIBUICAO: printf("# %d:atribuicao\n", lookahead.linha); break;
            case MENOR: printf("# %d:menor\n", lookahead.linha); break;
            case MAIOR: printf("# %d:maior\n", lookahead.linha); break;
            case MENOR_IGUAL: printf("# %d:menor_igual\n", lookahead.linha); break;
            case MAIOR_IGUAL: printf("# %d:maior_igual\n", lookahead.linha); break;
            case NEGACAO: printf("# %d:negacao\n", lookahead.linha); break;

            case DIV: printf("# %d:div\n", lookahead.linha); break;
            case OR: printf("# %d:or\n", lookahead.linha); break;
            case AND: printf("# %d:and\n", lookahead.linha); break;
            case NOT: printf("# %d:not\n", lookahead.linha); break;

            case NUMERO: printf("# %d:constint : %d\n", lookahead.linha, lookahead.atributo.numero); break;
            case CONSTCHAR: printf("# %d:constchar : '%c'\n", lookahead.linha, lookahead.atributo.ch); break;

            case COMENTARIO: printf("# %d:comentario\n", lookahead.linha); break;case EOS: break; // Não imprime nada para o fim do arquivo
            case EOS: printf("# %d:end_of_stream\n", lookahead.linha); break;

            default: printf("# %d:%s\n", lookahead.linha, nome_atomo(lookahead.atomo)); break;
        }
        // Obtém o próximo token apenas se não for o fim
        if (lookahead.atomo != EOS) {
            lookahead = obter_atomo();
        }
    } else {
        printf("# %d:erro sintatico, esperado [%s] encontrado [%s]\n", lookahead.linha, nome_atomo(esperado), nome_atomo(lookahead.atomo));
        exit(1);
    }
}

void program() {
    while(lookahead.atomo == COMENTARIO) {
        printf("# %d:comentario\n", lookahead.linha);
        lookahead = obter_atomo();
    }
    consome(PROGRAM);
    consome(IDENTIFICADOR);
    consome(PONTO_VIRGULA);
    block();
    consome(PONTO);
}

void block(){
    while(lookahead.atomo == COMENTARIO) {
        printf("# %d:comentario\n", lookahead.linha);
        lookahead = obter_atomo();
    }
    variable_declaration_part();
    statement_part();
}

void variable_declaration_part() {
    while(lookahead.atomo == COMENTARIO) {
        printf("# %d:comentario\n", lookahead.linha);
        lookahead = obter_atomo();
    }
    if (lookahead.atomo == VAR) {
        consome(VAR);
        variable_declaration();
        consome(PONTO_VIRGULA);
        while (lookahead.atomo == IDENTIFICADOR) {
            variable_declaration();
            consome(PONTO_VIRGULA);
        }
    }
}

void variable_declaration() {
    while(lookahead.atomo == COMENTARIO) {
        printf("# %d:comentario\n", lookahead.linha);
        lookahead = obter_atomo();
    }
    consome(IDENTIFICADOR);
    while (lookahead.atomo == VIRGULA) {
        consome(VIRGULA);
        consome(IDENTIFICADOR);
    }
    consome(DOIS_PONTOS);
    type();
}

void type() {
    while(lookahead.atomo == COMENTARIO) {
        printf("# %d:comentario\n", lookahead.linha);
        lookahead = obter_atomo();
    }
    if (lookahead.atomo == CHAR) consome(CHAR);
    else if (lookahead.atomo == INTEGER) consome(INTEGER);
    else if (lookahead.atomo == BOOLEAN) consome(BOOLEAN);
    else {
        printf("# %d:erro sintatico, tipo invalido esperado [char, integer, boolean] mas encontrado [%s]\n", lookahead.linha, nome_atomo(lookahead.atomo));
        exit(1);
    }
}

void statement_part() {
    while(lookahead.atomo == COMENTARIO) {
        printf("# %d:comentario\n", lookahead.linha);
        lookahead = obter_atomo();
    }
    consome(BEGIN);
    statement();
    while (lookahead.atomo == PONTO_VIRGULA) {
        consome(PONTO_VIRGULA);
        statement();
    }
    consome(END);
}

void statement() {
    while(lookahead.atomo == COMENTARIO) {
        printf("# %d:comentario\n", lookahead.linha);
        lookahead = obter_atomo();
    }
    if (lookahead.atomo == IDENTIFICADOR) assignment_statement();
    else if (lookahead.atomo == READ) read_statement();
    else if (lookahead.atomo == WRITE) write_statement();
    else if (lookahead.atomo == IF) if_statement();
    else if (lookahead.atomo == WHILE) while_statement();
    else if (lookahead.atomo == BEGIN) statement_part();
    else { /* Instrução Vazia */ }
}

void assignment_statement(){
    while(lookahead.atomo == COMENTARIO) {
        printf("# %d:comentario\n", lookahead.linha);
        lookahead = obter_atomo();
    }
    consome(IDENTIFICADOR);
    consome(ATRIBUICAO);
    expression();
}

void read_statement() {
    while(lookahead.atomo == COMENTARIO) {
        printf("# %d:comentario\n", lookahead.linha);
        lookahead = obter_atomo();
    }
    consome(READ);
    consome(ABRE_PAR);
    consome(IDENTIFICADOR);
    while (lookahead.atomo == VIRGULA) {
        consome(VIRGULA);
        consome(IDENTIFICADOR);
    }
    consome(FECHA_PAR);
}

void write_statement() {
    while(lookahead.atomo == COMENTARIO) {
        printf("# %d:comentario\n", lookahead.linha);
        lookahead = obter_atomo();
    }
    consome(WRITE);
    consome(ABRE_PAR);
    consome(IDENTIFICADOR);
    while (lookahead.atomo == VIRGULA) {
        consome(VIRGULA);
        consome(IDENTIFICADOR);
    }
    consome(FECHA_PAR);
}

void if_statement() {
    while(lookahead.atomo == COMENTARIO) {
        printf("# %d:comentario\n", lookahead.linha);
        lookahead = obter_atomo();
    }
    consome(IF);
    expression();
    consome(THEN);
    statement();
    while(lookahead.atomo == COMENTARIO) {
        printf("# %d:comentario\n", lookahead.linha);
        lookahead = obter_atomo();
    }
    if (lookahead.atomo == ELSE) {
        consome(ELSE);
        statement();
    }
}

void while_statement() {
    while(lookahead.atomo == COMENTARIO) {
        printf("# %d:comentario\n", lookahead.linha);
        lookahead = obter_atomo();
    }
    consome(WHILE);
    expression();
    consome(DO);
    statement();
}

void expression() {
    while(lookahead.atomo == COMENTARIO) {
        printf("# %d:comentario\n", lookahead.linha);
        lookahead = obter_atomo();
    }
    simple_expression();
    while(lookahead.atomo == COMENTARIO) {
        printf("# %d:comentario\n", lookahead.linha);
        lookahead = obter_atomo();
    }
    if (lookahead.atomo == MENOR || lookahead.atomo == MAIOR || lookahead.atomo == MENOR_IGUAL ||
        lookahead.atomo == MAIOR_IGUAL || lookahead.atomo == NEGACAO || lookahead.atomo == IGUAL ||
        lookahead.atomo == OR || lookahead.atomo == AND ) {
        relational_operator();
        simple_expression();
    }
}

void relational_operator() {
    while(lookahead.atomo == COMENTARIO) {
        printf("# %d:comentario\n", lookahead.linha);
        lookahead = obter_atomo();
    }
    switch (lookahead.atomo) {
        case MENOR: consome(MENOR); break;
        case MAIOR: consome(MAIOR); break;
        case MENOR_IGUAL: consome(MENOR_IGUAL); break;
        case MAIOR_IGUAL: consome(MAIOR_IGUAL); break;
        case NEGACAO: consome(NEGACAO); break;
        case IGUAL: consome(IGUAL); break;
        case OR: consome(OR); break;
        case AND: consome(AND); break;
        default:
             printf("# %d:erro sintatico, operador relacional esperado mas encontrado [%s]\n", lookahead.linha, nome_atomo(lookahead.atomo));
             exit(1);
    }
}

void simple_expression() { //S
    while(lookahead.atomo == COMENTARIO) {
        printf("# %d:comentario\n", lookahead.linha);
        lookahead = obter_atomo();
    }
    term();
    while (lookahead.atomo == MAIS || lookahead.atomo == MENOS) {
        adding_operator();
        term();
    }
}

void adding_operator() { //S
    while(lookahead.atomo == COMENTARIO) {
        printf("# %d:comentario\n", lookahead.linha);
        lookahead = obter_atomo();
    }
    if (lookahead.atomo == MAIS) consome(MAIS);
    else if (lookahead.atomo == MENOS) consome(MENOS);
    else {
        printf("# %d:erro sintatico, Esperado: %s, Encontrado: %s\n", lookahead.linha, nome_atomo(esperado), nome_atomo(lookahead.atomo));
        exit(1);
    }
}

void term() {
    while(lookahead.atomo == COMENTARIO) {
        printf("# %d:comentario\n", lookahead.linha);
        lookahead = obter_atomo();
    }
    factor();
    while (lookahead.atomo == ASTERISCO || lookahead.atomo == DIV) {
        multiplying_operator();
        factor();
    }
}

void multiplying_operator() { //s
    while(lookahead.atomo == COMENTARIO) {
        printf("# %d:comentario\n", lookahead.linha);
        lookahead = obter_atomo();
    }
    if (lookahead.atomo == ASTERISCO) consome(ASTERISCO);
    else if (lookahead.atomo == DIV) consome(DIV);
    else {
             printf("# %d:erro sintatico, Esperado: %s, Encontrado: %s\n", lookahead.linha, nome_atomo(esperado), nome_atomo(lookahead.atomo));
        exit(1);
    }
}

void factor() {
    while(lookahead.atomo == COMENTARIO) {
        printf("# %d:comentario\n", lookahead.linha);
        lookahead = obter_atomo();
    }
    if (lookahead.atomo == IDENTIFICADOR) consome(IDENTIFICADOR);
    else if (lookahead.atomo == CONSTINT) consome(CONSTINT);// constint
    else if (lookahead.atomo == NUMERO)   consome(NUMERO);
    else if (lookahead.atomo == CONSTCHAR) consome(CONSTCHAR);
    else if (lookahead.atomo == ABRE_PAR) {
        consome(ABRE_PAR);
        expression();
        consome(FECHA_PAR);
    }
    else if (lookahead.atomo == NOT) {
        consome(NOT);
        factor();
    }
    else if (lookahead.atomo == TRUE_TOKEN) consome(TRUE_TOKEN);
    else if (lookahead.atomo == FALSE_TOKEN) consome(FALSE_TOKEN);
    else {
             printf("# %d:erro sintatico, Esperado: %s, Encontrado: %s\n", lookahead.linha, nome_atomo(esperado), nome_atomo(lookahead.atomo));
        exit(1);
    }
}


