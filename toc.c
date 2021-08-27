/* 井字棋搜索
 * minimax/negamax
 */
#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef unsigned int uint;
typedef int bool;
typedef enum { EMPTY, BLACK, WHITE    } chessman_t;
typedef enum { DRAW,  WIN=1, LOST=-1  } result_t;
    /* 井字棋的搜索空间小，可以完全搜索
     * 所以局面评价是离散的 */

chessman_t board[9]; /* [row*3 + col] */
char *dot_filename = "toc.dot",  *out_filename = "toc.out";
FILE *fdot, *fout;

int max(int a, int b) { return a > b ? a : b; }
int min(int a, int b) { return a < b ? a : b; }
uint pos(uint row, uint col) { return row * 3 + col; }
bool eq3(uint a, uint b, uint c) { return a == b && b == c; }
chessman_t bd(uint row, uint col) { return board[pos(row, col)]; }

bool eq3bd(uint r1, uint c1, uint r2, uint c2, uint r3, uint c3)
{
    return eq3(bd(r1, c1), bd(r2, c2), bd(r3, c3));
}

chessman_t situation(void) /* 返回胜方
                              如果还不是终局，则返回EMPTY */
{
    uint i;

    /* 行 & 列 */
    for (i = 0; i < 3; ++i)
    {
        if (bd(i, 0) && eq3bd(i, 0, i, 1, i, 2)) /* 行 */
           return bd(i, 0);

        if (bd(0, i) && eq3bd(0, i, 1, i, 2, i)) /* 列 */
           return bd(0, i);
    }

    /* 对角线 */
    if (    (bd(0, 0) && eq3bd(0, 0, 1, 1, 2, 2)) ||
            (bd(0, 2) && eq3bd(0, 2, 1, 1, 2, 0)))
        return bd(1, 1);

    return EMPTY;
}

bool full(void) /* 棋盘是否已满 */
{
    uint i;
    for (i = 0; i < 9; ++i)
        if (!board[i])
            return 0;
    return 1;
}

void init_board(void) /* 初始化棋盘 */
{
    memset(board, 0, sizeof board);
}

chessman_t turn(chessman_t a) /* 返回另一方 */
{
    return a == BLACK ? WHITE : BLACK;
}

char chessman_char(chessman_t c)
{
    return c == BLACK ? 'X' : c == WHITE ? 'O' : '.';
}

char *fmtresult(result_t r)
{
    char *text[] = { "-lost", "0draw", "+win" };
    return text[r+1];
}

uint encode_board_real(chessman_t *board)
{
    /* 将棋盘以3进数编码
     * 总空间：3^9 = 19683 */
    uint i, pow = 1, code = 0;

    for (i = 0; i < 9; ++i, pow *= 3)
        code += pow * board[i];

    return code;
}

chessman_t *decode_board_RSTATIC(uint code)
{
    static chessman_t board[9];
    uint i;

    for (i = 0; i < 9; ++i)
    {
        board[i] = code % 3;
        code /= 3;
    }

    return board;
}

void chessman_swap(chessman_t *a, chessman_t *b)
{
    chessman_t t = *a;
    *a = *b;
    *b = t;
}

#if 0
123   321    789    987
456   654 v  456 h  654 hv=vh=X   Xh=v
789   987    123    321           Xv=h
 s
      741    987
      852 r  654 rr=X  rh=d1, therefore rrl=Xl=r
      963    321

      369    987
      258 l  654 ll=X therefore llr=Xr=l
      147    321

      147    963
      258 d1 852 d2=lv=rh
      369    741

s, h, v,
r, l,
X=hv=vh=rr=ll
d1=rv=lh, d2=lv=rh  (d1v=lhv=lX=r, d2v=rhv=rX=l)

    rv = lh
    rvh = l
    rX = l


Sum up:
s, h, v, r,
l, X, d1, d2
[8]
#endif

/* (x, y) = (row, col)
 * x' = 2-x, y' = 2-y
 * (x, y)' = (y, x)
 *
 * h: -> (2-x, y) = (x', y) = x'
 * v: -> (x, 2-y) = (x, y') = y'
 * r: -> (y, x)   = (x, y)' = (,)'
 * l: -> (2-y, x) = (x, y')' = vr
 *
 * hr -> (y, 2-x) = (x', y)' = hr
 * hl -> (2-y, 2-x) = (x', y')' = hvr
 * hv -> (2-x, 2-y) = (x', y')
 * rl -> (2-x, y) = (x', y) = h
 * s: (x, y)
 */
void mirror(chessman_t *board, bool x, bool y, bool q)
    /* 1: u, 0: u' */
{
    uint r, c;
    chessman_t b2[9];
    for (r = 0; r < 3; ++r)
        for (c = 0; c < 3; ++c)
        {
            uint r1 = x ? r : 2-r,
                 c1 = y ? c : 2-c;
            b2[pos(r, c)] = board[q ? pos(r1, c1) : pos(c1, r1)];
        }
    memcpy(board, b2, sizeof b2);
}

int board_to_equal_codes_RSTATIC(const chessman_t *board)
    /* 迭代，剪枝，返回-1结束 */
{
    static int code_list[9], *p;

    if (board)
        /* 首次调用 */
    {
        bool x, y, q;
        p = code_list;
        for (x = 0; x < 2; ++x)
            for (y = 0; y < 2; ++y)
                for (q = 0; q < 2; ++q)
                {
                    int c, *t;
                    bool dup = 0;
                    chessman_t b[9];

                    memcpy(b, board, sizeof b);
                    mirror(b, x, y, q);
                    c = encode_board_real(b);
                    for (t = code_list; t != p; ++t)
                        if (*t == c) /* 查重 */
                        {
                            dup = 1;
                            break;
                        }

                    if (!dup)
                        *p++ = c;
                }
        *p = -1;
        p = code_list;
    }

    return *p++;
}

int *board_to_equal_codes_a(chessman_t *board) /* alloc */
{
    int *code_list = malloc(sizeof (int[9])), *p = code_list;
    *p++ = board_to_equal_codes_RSTATIC(board);
    while ((*p++ = board_to_equal_codes_RSTATIC(0)) != -1);
    return code_list;
}

uint encode_board_min(chessman_t *board)
{
    uint code = board_to_equal_codes_RSTATIC(board);

    while (1)
    {
        int i = board_to_equal_codes_RSTATIC(0);
        if (i == -1)
            break;
        code = min(i, code);
    }

    return code;
}

uint encode_board(void)
{
    return encode_board_min(board);
}

void draw2(chessman_t *board, chessman_t me, result_t result,
           FILE *to, bool inf)
{
    uint i;

    /* 按照黑子方来看result */
    if (me == WHITE)
        result *= -1;

    for (i = 0; i < 9; ++i)
    {
        fputc(chessman_char(board[i]), to);

        if (i == 2)
            fprintf(to, "  #%d (-> %d)", encode_board_real(board),
                    encode_board_min(board));
        else if (inf)
        {
            if (i == 5)
                fprintf(to, "  轮到%c走棋", chessman_char(me));
            else if (i == 8)
                fprintf(to, "  %s", fmtresult(result));
        }

        if ((i + 1) % 3 == 0)
            fputc('\n', to);
    }
}

void draw(chessman_t me, result_t result)
{
    draw2(board, me, result, fout, 1);
}

void illustrate_code(uint code)
{
    int *codes = board_to_equal_codes_a(decode_board_RSTATIC(code)),
        *p = codes;

    printf("@%u\n", *p);
    draw2(decode_board_RSTATIC(*p++), -1, -1, stdout, 0);
    putchar('\n');

    for (; *p != -1;)
    {
        printf("@%u\n", *p);
        draw2(decode_board_RSTATIC(*p++), -1, -1, stdout, 0);
        putchar('\n');
    }

    free(codes);
}

result_t status(chessman_t me)
    /* 终局分数；调用者保证目前是终局
     * 如果不是终局的话，会返回0，和终局平局时的返回值一样 */
{
    chessman_t r = situation();
    if (r == me)
        return WIN;
    if (r == turn(me))
        return LOST;
    return DRAW;
}

result_t search(chessman_t next);
result_t predict(chessman_t me)
    /* 给出当前局面相对于我方来说的结果
     * 下一着是对方走棋 */
{
    result_t s = status(me);

    if (full() || s) /* 终局 */
        return s;
    return -search(turn(me));
}

result_t fdot_print_result(uint code, result_t result)
{
    char  *colour_table[] = { "red", "blue", "black" };
    uint fontsize_table[] = { 15, 15, 15 };


    fprintf(fdot, "%u [label=\"%u (%s)\", color=%s, fontsize=%u];\n",
            code, code, fmtresult(result),
            colour_table[result + 1], fontsize_table[result + 1]);

    return result;
}

result_t search(chessman_t me) /* 搜索走法，给出当前局面的结果
                                  下一着是我方走棋
                                  调用者保证目前不是终局 */
{
    uint i, board_code = encode_board(),
         sub_boards[9], *sub_board_p = sub_boards;
    result_t result = LOST;
    memset(sub_boards, 0, sizeof sub_boards);
        /* 只要走了棋，局面的3进数编码就不可能是0
         * 所以用0代表没有数据 */

    /* 因为不是终局，所以棋盘肯定不满，于是总是有棋可走 */
    for (i = 0; i < 9; ++i)
        if (!board[i])
        {
            board[i] = me;
            *sub_board_p++ = encode_board();
            result = max(result, predict(me));
                /* 这里predict可能会再调用search */
            board[i] = EMPTY;
        }

    /* 输出局面关系 */
    if (*--sub_board_p)
    {
        fprintf(fdot, "%d -> {", board_code);
        while (sub_board_p != sub_boards - 1)
            fprintf(fdot, "%d ", *sub_board_p--);
        fputs("};\n", fdot);
    }
    /* fdot_print_result(board_code, result); */

    /* 输出棋盘 */
    draw(me, result);
    fputc('\n', fout);

    return result;
}

result_t test_predict(void)
{
#if 0
XOX
OXX
.OO
#endif
    board[0] = board[2] = board[4] = board[5] = BLACK;
    board[1] = board[3] = board[7] = board[8] = WHITE;
    return predict(BLACK);
}

void init_fdot(void)
{
    fdot = fopen(dot_filename, "w");
    fputs("digraph d {\n", fdot);
    fout = fopen(out_filename, "w");
}

void close_fdot(void)
{
    fputs("}\n", fdot);
    fclose(fdot);
    fclose(fout);
}

void do_search(void)
{
    init_board();
    init_fdot();
    search(BLACK);
    close_fdot();
}

void test_coding(void) /* 棋盘编码测试 */
{
    chessman_t board[9];
    uint i, c;

    for (c = 0; ; ++c)
    {
        uint code;
        for (i = 0; i < 9; ++i)
            board[i] = rand() % 3;
        code = encode_board_real(board);
        printf("\r%u", code);

        if (memcmp(board, decode_board_RSTATIC(code), sizeof board)
                != 0)
        {
            puts(" fails");
            break;
        }
    }
}

void test_encode(void)
{
    chessman_t b[9];
    b[0] = b[2] = b[3] = b[6] = b[7] = EMPTY; /* #7131 */
    b[1] = b[4] = b[8] = BLACK;
    b[5] = WHITE;

    printf("Encode: %u\nEncode-min: %u\n\n",
            encode_board_real(b),
            encode_board_min(b));
    draw2(b, -1, -1, stdout, 0);
}

int main(int argc, char **argv)
{
    srand(time(0));
    if (argc == 1)
        do_search();
    else if (argc == 2)
    {
        switch (argv[1][0])
        {
        case 't':
            test_coding();
            break;
        case 'T':
            test_encode();
            break;
        default:
            {
            uint code;
            sscanf(argv[1], "%d", &code);
            illustrate_code(code);
            break;
            }
        }
    }

    return 0;
}
