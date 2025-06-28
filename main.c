/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  Main App
 *
 *        Version:  1.0
 *        Created:  28.06.2025 03:34
 *       Compiler:  musl-gcc
 *
 *         Author:  Fedya
 *
 * =====================================================================================
 */

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ROWS 100
#define MAX_COLS 10
#define CELL_WIDTH 15
#define CELL_HEIGHT 3

struct Cell {
  char text[256];
};

struct Table {
  struct Cell cells[MAX_ROWS][MAX_COLS];
  int row_count;
  int col_count;
  int cur_row;
  int cur_col;
  int view_start_row;
  int view_start_col;
  int visible_rows;
  int visible_cols;
};

void init_table(struct Table *table, int rows, int cols) {
  table->row_count = rows;
  table->col_count = cols;
  table->cur_row = 0;
  table->cur_col = 0;
  table->view_start_row = 0;
  table->view_start_col = 0;
  table->visible_rows = (LINES - 4) / CELL_HEIGHT;
  table->visible_cols = (COLS - 2) / (CELL_WIDTH + 1);

  for (int r = 0; r < MAX_ROWS; r++) {
    for (int c = 0; c < MAX_COLS; c++) {
      strcpy(table->cells[r][c].text, "");
    }
  }
}

void draw_table(WINDOW *win, struct Table *table) {
  werase(win);
  box(win, 0, 0);

  int max_visible_rows = (LINES - 4) / CELL_HEIGHT;
  int max_visible_cols = (COLS - 2) / (CELL_WIDTH + 1);
  table->visible_rows = max_visible_rows;
  table->visible_cols = max_visible_cols;

  if (table->view_start_col + table->visible_cols > table->col_count) {
    table->view_start_col = table->col_count - table->visible_cols;
    if (table->view_start_col < 0)
      table->view_start_col = 0;
  }

  if (table->view_start_row + table->visible_rows > table->row_count) {
    table->view_start_row = table->row_count - table->visible_rows;
    if (table->view_start_row < 0)
      table->view_start_row = 0;
  }

  for (int c = table->view_start_col;
       c < table->view_start_col + table->visible_cols && c < table->col_count;
       c++) {
    char header[3] = {'A' + c, '\0'};
    mvwprintw(win, 1, 2 + (c - table->view_start_col) * (CELL_WIDTH + 1), "%s",
              header);
  }

  for (int r = table->view_start_row;
       r < table->view_start_row + table->visible_rows && r < table->row_count;
       r++) {
    for (int c = table->view_start_col;
         c < table->view_start_col + table->visible_cols &&
         c < table->col_count;
         c++) {
      int y = 2 + (r - table->view_start_row) * CELL_HEIGHT;
      int x = 1 + (c - table->view_start_col) * (CELL_WIDTH + 1);

      if (r == table->cur_row && c == table->cur_col) {
        wattron(win, A_REVERSE);
      }

      for (int i = 0; i < CELL_HEIGHT; i++) {
        mvwaddch(win, y + i, x, '|');
        mvwaddch(win, y + i, x + CELL_WIDTH, '|');
      }
      mvwhline(win, y, x + 1, '-', CELL_WIDTH - 1);
      mvwhline(win, y + CELL_HEIGHT, x + 1, '-', CELL_WIDTH - 1);

      mvwprintw(win, y + 1, x + 2, "%.*s", CELL_WIDTH - 3,
                table->cells[r][c].text);

      if (r == table->cur_row && c == table->cur_col) {
        wattroff(win, A_REVERSE);
      }
    }
  }

  mvwprintw(win, LINES - 2, 1, "Current Position: [%d,%d] View: [%d-%d,%d-%d]",
            table->cur_row + 1, table->cur_col + 1, table->view_start_row + 1,
            table->view_start_row + table->visible_rows,
            table->view_start_col + 1,
            table->view_start_col + table->visible_cols);
  mvwprintw(win, LINES - 1, 1,
            "Arrows: Navigation | Enter: edit | F2: save | ESC: exit");

  wrefresh(win);
}

void edit_cell(WINDOW *main_win, struct Table *table) {
  int row = table->cur_row;
  int col = table->cur_col;

  WINDOW *edit_win =
      newwin(3, CELL_WIDTH, 2 + (row - table->view_start_row) * CELL_HEIGHT,
             2 + (col - table->view_start_col) * (CELL_WIDTH + 1));
  keypad(edit_win, TRUE);
  box(edit_win, 0, 0);

  echo();
  curs_set(1);
  mvwprintw(edit_win, 1, 1, "%s", table->cells[row][col].text);
  wmove(edit_win, 1, 1 + strlen(table->cells[row][col].text));
  mvwgetnstr(edit_win, 1, 1, table->cells[row][col].text, 255);
  curs_set(0);
  noecho();

  delwin(edit_win);
  touchwin(main_win);
}

void save_to_csv(struct Table *table) {
  FILE *fp = fopen("table.csv", "w");
  if (!fp)
    return;

  for (int r = 0; r < table->row_count; r++) {
    for (int c = 0; c < table->col_count; c++) {
      if (strchr(table->cells[r][c].text, ',') ||
          strchr(table->cells[r][c].text, '"')) {
        fprintf(fp, "\"");
        for (char *p = table->cells[r][c].text; *p; p++) {
          if (*p == '"')
            fputc('"', fp);
          fputc(*p, fp);
        }
        fprintf(fp, "\"");
      } else {
        fprintf(fp, "%s", table->cells[r][c].text);
      }
      if (c < table->col_count - 1)
        fputc(',', fp);
    }
    fputc('\n', fp);
  }

  fclose(fp);
}

int main() {
  int start_rows, start_cols;
  printf("Enter number of rows (1-%d): ", MAX_ROWS);
  scanf("%d", &start_rows);
  printf("Enter number of columns (1-%d): ", MAX_COLS);
  scanf("%d", &start_cols);

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  if (has_colors()) {
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
  }

  struct Table table;
  init_table(&table, start_rows, start_cols);

  WINDOW *main_win = newwin(LINES, COLS, 0, 0);
  keypad(main_win, TRUE);

  int ch;
  while ((ch = wgetch(main_win)) != KEY_F(10)) {
    switch (ch) {
    case KEY_UP:
      if (table.cur_row > 0) {
        table.cur_row--;
        if (table.cur_row < table.view_start_row)
          table.view_start_row = table.cur_row;
      }
      break;
    case KEY_DOWN:
      if (table.cur_row < table.row_count - 1) {
        table.cur_row++;
        if (table.cur_row >= table.view_start_row + table.visible_rows)
          table.view_start_row = table.cur_row - table.visible_rows + 1;
      }
      break;
    case KEY_LEFT:
      if (table.cur_col > 0) {
        table.cur_col--;
        if (table.cur_col < table.view_start_col)
          table.view_start_col = table.cur_col;
      }
      break;
    case KEY_RIGHT:
      if (table.cur_col < table.col_count - 1) {
        table.cur_col++;
        if (table.cur_col >= table.view_start_col + table.visible_cols)
          table.view_start_col = table.cur_col - table.visible_cols + 1;
      }
      break;
    case '\n':
      edit_cell(main_win, &table);
      break;
    case KEY_F(2):
      save_to_csv(&table);
      break;
    case KEY_EXIT:
    case 'q':
      goto exit;
      break;
    }
    draw_table(main_win, &table);
  }

exit:
  endwin();
  return 0;
}
