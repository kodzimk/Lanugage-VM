#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>


#define ARRAY_SIZE(xs) (sizeof(xs) / sizeof((xs)[0]))
#define BM_STACK_CAPACITY 1024
#define BM_PROGRAM_CAPACITY 1024
#define BM_EXECUTION_LIMIT 69

typedef enum {
    ERR_OK = 0,
    ERR_STACK_OVERFLOW,
    ERR_STACK_UNDERFLOW,
    ERR_ILLEGAL_INST,
    ERR_ILLEGAL_INST_ACCESS,
    ERR_ILLEGAL_OPERAND,
    ERR_DIV_BY_ZERO,
} Err;

const char* err_as_cstr(Err err)
{
    switch (err) {
    case ERR_OK:
        return "ERR_OK";
    case ERR_STACK_OVERFLOW:
        return "ERR_STACK_OVERFLOW";
    case ERR_STACK_UNDERFLOW:
        return "ERR_STACK_UNDERFLOW";
    case ERR_ILLEGAL_INST:
        return "ERR_ILLEGAL_INST";
    case ERR_ILLEGAL_OPERAND:
        return "ERR_ILLEGAL_OPERAND";
    case ERR_ILLEGAL_INST_ACCESS:
        return "ERR_ILLEGAL_INST_ACCESS";
    case ERR_DIV_BY_ZERO:
        return "ERR_DIV_BY_ZERO";
    default:
        assert(0 && "err_as_cstr: Unreachable");
    }
}

typedef int64_t Word;

typedef enum {
    INST_NOP = 0,
    INST_PUSH,
    INST_DUP,
    INST_PLUS,
    INST_MINUS,
    INST_MULT,
    INST_DIV,
    INST_JMP,
    INST_JMP_IF,
    INST_EQ,
    INST_HALT,
    INST_PRINT_DEBUG,
} Inst_Type;

const char* inst_type_as_cstr(Inst_Type type)
{
    switch (type) {
    case INST_NOP:         return "INST_NOP";
    case INST_PUSH:        return "INST_PUSH";
    case INST_PLUS:        return "INST_PLUS";
    case INST_MINUS:       return "INST_MINUS";
    case INST_MULT:        return "INST_MULT";
    case INST_DIV:         return "INST_DIV";
    case INST_JMP:         return "INST_JMP";
    case INST_HALT:        return "INST_HALT";
    case INST_JMP_IF:      return "INST_JMP_IF";
    case INST_EQ:          return "INST_EQ";
    case INST_PRINT_DEBUG: return "INST_PRINT_DEBUG";
    case INST_DUP:         return "INST_DUP";
    default: assert(0 && "inst_type_as_cstr: unreachable");
    }
}

typedef struct {
    Inst_Type type;
    Word operand;
} Inst;

typedef struct {
    Word stack[BM_STACK_CAPACITY];
    Word stack_size;

    Inst program[BM_PROGRAM_CAPACITY];
    Word program_size;
    Word ip;

    int halt;
} Bm;

#define MAKE_INST_PUSH(value) {.type = INST_PUSH, .operand = (value)}
#define MAKE_INST_PLUS        {.type = INST_PLUS}
#define MAKE_INST_MINUS       {.type = INST_MINUS}
#define MAKE_INST_MULT        {.type = INST_MULT}
#define MAKE_INST_DIV         {.type = INST_DIV}
#define MAKE_INST_JMP(addr)   {.type = INST_JMP, .operand = (addr)}
#define MAKE_INST_DUP(addr)   {.type = INST_DUP, .operand = (addr)}
#define MAKE_INST_HALT        {.type = INST_HALT, .operand = (addr)}

Err bm_execute_inst(Bm* bm)
{
    if (bm->ip < 0 || bm->ip >= bm->program_size) {
        return ERR_ILLEGAL_INST_ACCESS;
    }

    Inst inst = bm->program[bm->ip];

    switch (inst.type) {
    case INST_NOP:
        bm->ip += 1;
        break;

    case INST_PUSH:
        if (bm->stack_size >= BM_STACK_CAPACITY) {
            return ERR_STACK_OVERFLOW;
        }
        bm->stack[bm->stack_size++] = inst.operand;
        bm->ip += 1;
        break;

    case INST_PLUS:
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }
        bm->stack[bm->stack_size - 2] += bm->stack[bm->stack_size - 1];
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_MINUS:
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }
        bm->stack[bm->stack_size - 2] -= bm->stack[bm->stack_size - 1];
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_MULT:
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }
        bm->stack[bm->stack_size - 2] *= bm->stack[bm->stack_size - 1];
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_DIV:
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }

        if (bm->stack[bm->stack_size - 1] == 0) {
            return ERR_DIV_BY_ZERO;
        }

        bm->stack[bm->stack_size - 2] /= bm->stack[bm->stack_size - 1];
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_JMP:
        bm->ip = inst.operand;
        break;

    case INST_HALT:
        bm->halt = 1;
        break;

    case INST_EQ:
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }

        bm->stack[bm->stack_size - 2] = bm->stack[bm->stack_size - 1] == bm->stack[bm->stack_size - 2];
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_JMP_IF:
        if (bm->stack_size < 1) {
            return ERR_STACK_UNDERFLOW;
        }

        if (bm->stack[bm->stack_size - 1]) {
            bm->stack_size -= 1;
            bm->ip = inst.operand;
        }
        else {
            bm->ip += 1;
        }
        break;

    case INST_PRINT_DEBUG:
        if (bm->stack_size < 1) {
            return ERR_STACK_UNDERFLOW;
        }
        printf("%ld\n", bm->stack[bm->stack_size - 1]);
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_DUP:
        if (bm->stack_size >= BM_STACK_CAPACITY) {
            return ERR_STACK_OVERFLOW;
        }

        if (bm->stack_size - inst.operand <= 0) {
            return ERR_STACK_UNDERFLOW;
        }

        if (inst.operand < 0) {
            return ERR_ILLEGAL_OPERAND;
        }

        bm->stack[bm->stack_size] = bm->stack[bm->stack_size - 1 - inst.operand];
        bm->stack_size += 1;
        bm->ip += 1;
        break;

    default:
        return ERR_ILLEGAL_INST;
    }

    return ERR_OK;
}

void bm_dump_stack(FILE* stream, const Bm* bm)
{
    fprintf(stream, "Stack:\n");
    if (bm->stack_size > 0) {
        for (Word i = 0; i < bm->stack_size; ++i) {
            fprintf(stream, "  %ld\n", bm->stack[i]);
        }
    }
    else {
        fprintf(stream, "  [empty]\n");
    }
}

void bm_load_program_from_memory(Bm* bm, Inst* program, size_t program_size)
{
    assert(program_size < BM_PROGRAM_CAPACITY);
    memcpy(bm->program, program, sizeof(program[0]) * program_size);
    bm->program_size = program_size;
}

void bm_load_program_from_file(Bm* bm, const char* file_path)
{
    FILE* f = fopen(file_path, "rb");
    if (f == NULL) {
        fprintf(stderr, "ERROR: Could not open file `%s`: %s\n",
            file_path, strerror(errno));
        exit(1);
    }

    if (fseek(f, 0, SEEK_END) < 0) {
        fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
            file_path, strerror(errno));
        exit(1);
    }

    long m = ftell(f);
    if (m < 0) {
        fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
            file_path, strerror(errno));
        exit(1);
    }

    assert(m % sizeof(bm->program[0]) == 0);
    assert((size_t)m <= BM_PROGRAM_CAPACITY * sizeof(bm->program[0]));

    if (fseek(f, 0, SEEK_SET) < 0) {
        fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
            file_path, strerror(errno));
        exit(1);
    }

    bm->program_size = fread(bm->program, sizeof(bm->program[0]), m / sizeof(bm->program[0]), f);

    if (ferror(f)) {
        fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
            file_path, strerror(errno));
        exit(1);
    }

    fclose(f);
}

void bm_save_program_to_file(Inst* program, size_t program_size,
    const char* file_path)
{
    FILE* f = fopen(file_path, "wb");
    if (f == NULL) {
        fprintf(stderr, "ERROR: Could not open file `%s`: %s\n",
            file_path, strerror(errno));
        exit(1);
    }

    fwrite(program, sizeof(program[0]), program_size, f);

    if (ferror(f)) {
        fprintf(stderr, "ERROR: Could not write to file `%s`: %s\n",
            file_path, strerror(errno));
        exit(1);
    }

    fclose(f);
}

Bm bm = { 0 };

typedef struct {
    size_t count;
     char* data;
} String_View;

String_View cstr_as_sv(char* cstr)
{
    return (String_View) {
        .count = strlen(cstr),
         .data = cstr,
    };
}

size_t sv_trim_left(String_View sv)
{
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[i])) {
        i += 1;
    }

    return i;
}

String_View sv_trim_right(String_View sv)
{
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[sv.count - 1 - i])) {
        i += 1;
    }

    String_View result = {
        .count = i + 1,
        .data = sv.data + i,
    };


    return result;
}
String_View sv_chop_by_delim(String_View* sv, char delim)
{
    size_t i = 0;
    while (i < sv->count && sv->data[i] != delim)
    {
        i += 1;
    }

    String_View line = {
    .count = i,
    .data =(char*)malloc(sizeof(char) * i + 1),
    };

    size_t j = 0;
    while (i > 0 && line != NULL)
    {
        line[j] = source[j];
        j += 1;
        i -= 1;
    }

       sv->count -= i;
       sv->data += i;
    
    return line;
}

int sv_eq(String_View a, String_View b)
{
    for (int i = 0, j = 0; i < b.count; i++, j += 2)
    {
        if (a.data[j] != b.data[i])
            return 0;
    }
    return 1;
}
int sv_to_int(String_View sv)
{
    int result = 0;

    for (size_t i = 0; i < sv.count; ++i) {
        if(isdigit(sv.data[i]))
          result = result * 10 + sv.data[i] - '0';
    }

    return result;
}

Inst bm_translate_line(String_View line)
{ 
    String_View inst_name = sv_chop_by_delim(&line, ' ');

    if (sv_eq(inst_name,cstr_as_sv("push"))) {
        int operand = sv_to_int(line);
        return (Inst) { .type = INST_PUSH, .operand = operand };
    }
    else if (sv_eq(inst_name, cstr_as_sv("dup"))) {

        int operand = sv_to_int(line);
        return (Inst) { .type = INST_DUP, .operand = operand };
    }
    else if (sv_eq(inst_name, cstr_as_sv("plus"))) {
        return (Inst) { .type = INST_PLUS };
    }
    else if (sv_eq(inst_name, cstr_as_sv("jmp"))) {
 
        int operand = sv_to_int(line);
        return (Inst) { .type = INST_JMP, .operand = operand };
    }
    else {
        fprintf(stderr, "ERROR: unknown instruction %s",inst_name.data);
        exit(1);
    }
}

size_t bm_translate_source(String_View source,
    Inst* program, size_t program_capacity)
{
    size_t program_size = 0;
    while (source.count > 0) {
        assert(program_size < program_capacity);
        String_View line = sv_chop_by_delim(&source, '\n');
        for (size_t i = 0; i < line.count; i++)
        {
            printf("%c", line.data[i]);
        }
        program[program_size++] = bm_translate_line(line);
    }
    return program_size;
}

String_View slurp_file(const char* file_path)
{
    FILE* f = fopen(file_path, "r");
    if (f == NULL) {
        fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
            file_path, strerror(errno));
        exit(1);
    }

    if (fseek(f, 0, SEEK_END) < 0) {
        fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
            file_path, strerror(errno));
        exit(1);
    }

    long m = ftell(f);
    if (m < 0) {
        fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
            file_path, strerror(errno));
        exit(1);
    }

    char* buffer = malloc(m);
    if (buffer == NULL) {
        fprintf(stderr, "ERROR: Could not allocate memory for file: %s\n",
            strerror(errno));
        exit(1);
    }

    if (fseek(f, 0, SEEK_SET) < 0) {
        fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
            file_path, strerror(errno));
        exit(1);
    }

    size_t n = fread(buffer, 1, m, f);
    if (ferror(f)) {
        fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
            file_path, strerror(errno));
        exit(1);
    }

    fclose(f);

    return (String_View) {
        .count = n,
            .data = buffer,
    };
}

int main(int argc, char** argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: ./ebasm <input.ebasm> <output.bm>\n");
        fprintf(stderr, "ERROR: expected input and output\n");
        exit(1);
    }

    const char* input_file_path = argv[1];
    const char* output_file_path = argv[2];

    String_View source = slurp_file(input_file_path);
    source.data += 2;
    source.count -= 2;

    bm.program_size = bm_translate_source(source,
        bm.program,
        BM_PROGRAM_CAPACITY);

    printf("%s", inst_type_as_cstr(bm.program[1].type));
    

    bm_save_program_to_file(bm.program, bm.program_size, output_file_path);

    return 0;
}