#pragma once 

#include "rmkit.h"
#include "drawing.hpp"
#include "common.hpp"

enum {
  ROUND_PEN = 0,
  VERT_PEN,
  HORIZ_PEN,

  NUM_PEN
};

extern const char* pen_names[];

struct stroke{
  int ax,ay,bx,by;
  unsigned char width, color, type, etc;
  stroke& undraw(framebuffer::FB* fb,int y_scroll,int y,int off_x = 0,int off_y = 0);
  stroke& draw(framebuffer::FB* fb,int y_scroll,int y,int off_x = 0,int off_y = 0);  
};

struct file_link{
  int x,y,w;
  std::string file;
};
struct grid_row{
  std::vector<stroke> vect[16];
  std::vector<file_link> links;
};
extern const int link_size;
std::string link_render_text(const std::string& text);




struct grid{
  int row_h;
  int row_w;
  int w,h;
  int y,y_scroll;
  std::vector<grid_row> rows;
  bool loaded = false, edited = false, linksedited = false;
  std::string current_file = "Home";
  int current_page = 0;
  framebuffer::FB* fb;
  sqlite3* db = nullptr;
  sqlite3_stmt* read_s, *write_s, *clear_s, *shift_s, *page_s;
  sqlite3_stmt* read_l, *write_l, *clear_l, *shift_l, *page_l;
  sqlite3_stmt* read_d, *write_d, *clear_d, *shift_d, *page_d, *del_d;

  void move(std::string from, int from_page, std::string to, int to_page);
  void del(std::string file, int page);
  void open();
  void close();
  void init(int w,int h,int y,framebuffer::FB* FB);
  void save();
  void load(const std::string& file,int page);
  void unload();
  void add(stroke& st);
  void add_link(int x,int y,std::string file);
  file_link* get_link(int x,int y);
  void remove_link(int x,int y);
  void draw();
  void remove(int x,int y,int r);
  void draw_link(file_link& l);
  void redraw(int x,int y,int w,int h);
  void clear();
  void cut(int x,int y,int r,std::vector<stroke>& out);
  ~grid();
};
