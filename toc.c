/* 井字棋搜索
 * minimax/negamax
 */
#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

typedef unsigned int uint;
typedef int bool;
typedef enum { EMPTY, BLACK, WHITE    } chessman_t;
typedef enum { DRAW,  WIN=1, LOST=-1  } result_t;
    /* 井字棋的搜索空间小，可以完全搜索
     * 所以评价结果是离散的 */

chessman_t board[9]; /* chessboard[row*3 + col] */
char *dot_filename = "toc.dot";
FILE *fdot;

int max(int a, int b)
{
    return a > b ? a : b;
}

uint pos(uint row, uint col)
{
    return row * 3 + col;
}

bool eq3(uint a, uint b, uint c)
{
    return a == b && b == c;
}

chessman_t bd(uint row, uint col)
{
    return board[pos(row, col)];
}

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
    int i;
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

uint encode_board(void)
{
    /* 将棋盘以3进数编码
     * 总空间：3^9 = 19683 */
    int i, pow = 1, sum = 0;
    for (i = 0; i < 9; ++i, pow *= 3)
        sum += pow * board[i];

    return sum;
}

void draw(chessman_t me, result_t result)
{
    uint i;
    for (i = 0; i < 9; ++i)
    {
        putchar(chessman_char(board[i]));

        if (i == 2)
            printf("  #%d", encode_board());
        else if (i == 5)
            printf("  轮到%c走棋", chessman_char(me));
        else if (i == 8)
            printf("  %s", fmtresult(result));

        if ((i + 1) % 3 == 0)
            putchar('\n');
    }
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
    if (*sub_board_p)
    {
        fprintf(fdot, "%d -> {", board_code);
        while (sub_board_p != sub_boards - 1)
            fprintf(fdot, "%d ", *sub_board_p--);
        fputs("};", fdot);
    }
    fdot_print_result(board_code, result);

    /* 输出棋盘 */
    draw(me, result);
    putchar('\n');

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
    fputs("digraph d {", fdot);
}

void close_fdot(void)
{
    fputs("}", fdot);
    fclose(fdot);
}

int main(void)
{
    double r;
    init_board();
    init_fdot();
    search(BLACK);
    close_fdot();
    return 0;
}
