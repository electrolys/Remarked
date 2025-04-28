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

const char* pen_names[] = {
  "Round",
  "Vertical",
  "Horizontal",
};

struct stroke{
  int ax,ay,bx,by;
  unsigned char width, color, type, etc;

  stroke& undraw(framebuffer::FB* fb,int y_scroll,int y,int off_x = 0,int off_y = 0){
    if (ay+off_y-width/2 < y_scroll || by+off_y-width/2 < y_scroll) return *this;

    switch (type){
    case ROUND_PEN:
      my_draw_line_circle(fb,ax+off_x,y+off_y+ay-y_scroll,bx+off_x,y+off_y+by-y_scroll,width,WHITE);
      break;
    case VERT_PEN:
      my_draw_line_vert(fb,ax+off_x,y+off_y+ay-y_scroll,bx+off_x,y+off_y+by-y_scroll,width,WHITE);
      break;
    case HORIZ_PEN:
      my_draw_line_horiz(fb,ax+off_x,y+off_y+ay-y_scroll,bx+off_x,y+off_y+by-y_scroll,width,WHITE);
      break;
    // case FLAT_PEN:
      // my_draw_line_line(fb,ax+off_x,y+off_y+ay-y_scroll,bx+off_x,y+off_y+by-y_scroll,width,width,WHITE);
      // break;
    }
    return *this;
  }
  
  stroke& draw(framebuffer::FB* fb,int y_scroll,int y,int off_x = 0,int off_y = 0){
    if (ay+off_y-width/2 < y_scroll || by+off_y-width/2 < y_scroll) return *this;
    switch (type){
    case ROUND_PEN:
      my_draw_line_circle(fb,ax+off_x,y+off_y+ay-y_scroll,bx+off_x,y+off_y+by-y_scroll,width,color::SCALE_16[(int)color]);
      break;
    case VERT_PEN:
      my_draw_line_vert(fb,ax+off_x,y+off_y+ay-y_scroll,bx+off_x,y+off_y+by-y_scroll,width,color::SCALE_16[(int)color]);
      break;
    case HORIZ_PEN:
      my_draw_line_horiz(fb,ax+off_x,y+off_y+ay-y_scroll,bx+off_x,y+off_y+by-y_scroll,width,color::SCALE_16[(int)color]);
      break;
    // case FLAT_PEN:
      // my_draw_line_line(fb,ax+off_x,y+off_y+ay-y_scroll,bx+off_x,y+off_y+by-y_scroll,width,width,color::SCALE_16[(int)color]);
      // break;
    }
    return *this;
  }
};

struct file_link{
  int x,y,w;
  std::string file;
};
struct grid_row{
  std::vector<stroke> vect[16];
  std::vector<file_link> links;
};





const char* read_st_str = "select ax, ay, bx, by, size, color, type, etc from pen_strokes where file=? and page=?;";
const char* write_st_str = "insert into pen_strokes (file, page, ax, ay, bx, by, size, color, type, etc) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

const char* write_link_str = "insert into file_links (file, page, to_file, x, y) values (?, ?, ?, ?, ?);";
const char* read_link_str = "select to_file, x, y from file_links where file=? and page=?;";

const char* write_dat_str = "replace into page_data (file, page, key, value) values (?, ?, ?, ?);";
const char* read_dat_str = "select value from page_data where file=? and page=? and key=?;";

const char* clear_st_str = 
"delete from pen_strokes where file=? and page=?;";
const char* clear_link_str =
"delete from file_links where file=? and page=?;";
const char* clear_dat_str =
"delete from page_data where file=? and page=?;";

const char* init_st_str = 
"PRAGMA synchronous = OFF;\n"
"PRAGMA journal_mode = MEMORY;\n"
"create table if not exists file_links (file text, page int, to_file text, x int, y int) strict;\n"
"create table if not exists page_data (file text, page int, key text, value any, unique(file, page, key));\n"
"create table if not exists pen_strokes (file text, page int, ax int, ay int, bx int, by int, size int, color int, type int, etc int) strict;";


const char* shift_st_str = 
"update pen_strokes set page = page + ?3 where file = ?1 and page >= ?2;";

const char* setpage_st_str = 
"update pen_strokes set file = ?3, page = ?4 where file = ?1 and page = ?2;";

const char* shift_link_str = 
"update file_links set page = page + ?3 where file = ?1 and page >= ?2;";

const char* setpage_link_str = 
"update file_links set file = ?3, page = ?4 where file = ?1 and page = ?2;";

const char* shift_dat_str = 
"update or ignore page_data set page = page + ?3 where file = ?1 and page >= ?2;";

const char* setpage_dat_str = 
"update page_data set file = ?3, page = ?4 where file = ?1 and page = ?2;";


const int link_size = 32;






struct grid{
  int row_h;
  int row_w;
  int h;
  int y,y_scroll;
  std::vector<grid_row> rows;
  bool loaded = false, edited = false, linksedited = false;
  std::string current_file = "Home";
  int current_page = 0;
  framebuffer::FB* fb;
  sqlite3* db = nullptr;
  sqlite3_stmt* read_s, *write_s, *clear_s, *shift_s, *page_s;
  sqlite3_stmt* read_l, *write_l, *clear_l, *shift_l, *page_l;
  sqlite3_stmt* read_d, *write_d, *clear_d, *shift_d, *page_d;

  void move(std::string from, int from_page, std::string to, int to_page) {
    if (from == to) {
      sql_run(page_s,"sisi",from.c_str(),abs(from_page),to.c_str(),-1);
      sql_run(page_l,"sisi",from.c_str(),abs(from_page),to.c_str(),-1);
      sql_run(page_d,"sisi",from.c_str(),abs(from_page),to.c_str(),-1);

      sql_run(shift_s,"sii",from.c_str(),abs(from_page)+1,-1);
      sql_run(shift_l,"sii",from.c_str(),abs(from_page)+1,-1);
      sql_run(shift_d,"sii",from.c_str(),abs(from_page)+1,-1);

      sql_run(shift_s,"sii",to.c_str(),abs(to_page),1);
      sql_run(shift_l,"sii",to.c_str(),abs(to_page),1);
      sql_run(shift_d,"sii",to.c_str(),abs(to_page),1);

      sql_run(page_s,"sisi",from.c_str(),-1,to.c_str(),abs(to_page));
      sql_run(page_l,"sisi",from.c_str(),-1,to.c_str(),abs(to_page));
      sql_run(page_d,"sisi",from.c_str(),-1,to.c_str(),abs(to_page));
      return;
    }
    sql_run(shift_s,"sii",to.c_str(),abs(to_page),1);
    sql_run(shift_l,"sii",to.c_str(),abs(to_page),1);
    sql_run(shift_d,"sii",to.c_str(),abs(to_page),1);
    
    sql_run(page_s,"sisi",from.c_str(),abs(from_page),to.c_str(),abs(to_page));
    sql_run(page_l,"sisi",from.c_str(),abs(from_page),to.c_str(),abs(to_page));
    sql_run(page_d,"sisi",from.c_str(),abs(from_page),to.c_str(),abs(to_page));
    
    sql_run(shift_s,"sii",from.c_str(),abs(from_page)+1,-1);
    sql_run(shift_l,"sii",from.c_str(),abs(from_page)+1,-1);
    sql_run(shift_d,"sii",from.c_str(),abs(from_page)+1,-1);
  }

  void del(std::string file, int page) {
    sql_run(clear_s,"si",file.c_str(),page);
    sql_run(clear_l,"si",file.c_str(),page);
    sql_run(clear_d,"si",file.c_str(),page);

    sql_run(shift_s,"sii",file.c_str(),abs(page)+1,-1);
    sql_run(shift_l,"sii",file.c_str(),abs(page)+1,-1);
    sql_run(shift_d,"sii",file.c_str(),abs(page)+1,-1);
  }
  
  void open(){
    if (db) return;
    sqlite3_open("/home/root/notes.db",&db);
    char* err = nullptr;
    sqlite3_exec(db,init_st_str,NULL,NULL,&err);
    std::cout << err << "\n";
    if (err){
      error_msg(fb,err);
      sqlite3_free(err);
    }
    #define prepare(str,stmt) if (sqlite3_prepare_v2(db,str,-1,&stmt,NULL)) error_msg(fb,(const char*)sqlite3_errmsg(db));

    prepare(read_st_str,read_s);
    prepare(read_link_str,read_l);
    prepare(read_dat_str,read_d);

    prepare(write_st_str,write_s);
    prepare(write_link_str,write_l);
    prepare(write_dat_str,write_d);

    prepare(clear_st_str,clear_s);
    prepare(clear_link_str,clear_l);
    prepare(clear_dat_str,clear_d);

    prepare(shift_st_str,shift_s);
    prepare(shift_link_str,shift_l);
    prepare(shift_dat_str,shift_d);

    prepare(setpage_st_str,page_s);
    prepare(setpage_link_str,page_l);
    prepare(setpage_dat_str,page_d);

    #undef prepare


    //load(current_file,abs(current_page));
  }

  void close(){
    if (!db) return;
    unload();
    
    sqlite3_finalize(read_s);
    sqlite3_finalize(read_l);
    sqlite3_finalize(write_s);
    sqlite3_finalize(write_l);
    sqlite3_finalize(clear_s);
    sqlite3_finalize(page_s);
    sqlite3_finalize(page_l);
    sqlite3_finalize(shift_s);
    sqlite3_finalize(shift_l);
    sqlite3_close(db);
    db = nullptr;
  }
  
  void init(int w,int h,int y,framebuffer::FB* FB){
    row_h = h/16+1;
    row_w = w/16+1;
    y_scroll = 0;
    this->h = h;
    this->y = y;
    fb = FB;
    open();
  }
  void save(){
    if (!db) return;
    if (loaded){
      if (edited){
        sql_run(clear_s,"si",current_file.c_str(),abs(current_page));
        
        sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
        int s = (int)rows.size();
        for (int j = 0 ; j < s ; j++)
          for (int i = 0 ; i < 16 ; i++)
            for (stroke& k : rows[j].vect[i])
              sql_run(write_s,"siiiiiiiii",current_file.c_str(),abs(current_page),k.ax,k.ay,k.bx,k.by,k.width,k.color,k.type,k.etc); 
        sqlite3_exec(db, "END TRANSACTION", NULL, NULL, NULL);
        
        edited = false;
      }
      
      if (linksedited){
        sql_run(clear_l,"si",current_file.c_str(),current_page);
        
        int s = (int)rows.size();
        sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
        for (int j = 0 ; j < s ; j++)
          for (file_link& l : rows[j].links)
            sql_run(write_l,"sisii",current_file.c_str(),abs(current_page),l.file.c_str(),l.x,l.y);
        sqlite3_exec(db, "END TRANSACTION", NULL, NULL, NULL);
      }
      
    }
  }

  void load(const std::string& file,int page){
    if (!db) return;
    unload();


    current_file = file;
    current_page = page;

    

    sql_bind(read_s,"si",file.c_str(),abs(page));
    int t;
    while ((t=sqlite3_step(read_s)) != SQLITE_DONE){
      if (t == SQLITE_ROW) {
        stroke st = {
          sqlite3_column_int(read_s,0),
          sqlite3_column_int(read_s,1),
          sqlite3_column_int(read_s,2),
          sqlite3_column_int(read_s,3),
          (char)sqlite3_column_int(read_s,4),
          (char)sqlite3_column_int(read_s,5),
          (char)sqlite3_column_int(read_s,6),
          (char)sqlite3_column_int(read_s,7),
        };
        add(st);
      }
      
    }

    sql_bind(read_l,"si",file.c_str(),abs(page));
    while ((t=sqlite3_step(read_l)) != SQLITE_DONE){
      if (t == SQLITE_BUSY) continue;
      if (t == SQLITE_ROW) {
        const unsigned char* s = sqlite3_column_text(read_l,0);
        std::string t((const char*)s);
        add_link(sqlite3_column_int(read_l,1),sqlite3_column_int(read_l,2),t);
        continue;
      }
    }
    
    loaded = true;
    edited = false;
    linksedited = false;

    y_scroll = 0;
          
  }

  ~grid() {
    unload();
    close();
  }
  
  void unload() {
    if (!db) return;
    save();
    rows.clear();
    loaded = false;
  }

  
  
  void add(stroke& st){
    int i = st.ax / row_w;
    int j = st.ay / row_h;
    if (j>=(int)rows.size()){
      rows.resize(j+1);
    }
    rows[j].vect[i].push_back(st);
    edited = true;
  }
  void add_link(int x,int y,std::string file){
    int j = y / row_h;
    if (j>=(int)rows.size()){
      rows.resize(j+1);
    }
    auto s = stbtext::get_text_size(file,link_size);
    rows[j].links.push_back(file_link{x,y,s.w,file});
    linksedited = true;
  }


  file_link* get_link(int x,int y){
    int j = y / row_h;
    
    for (int i = max(0,j-1); i <= min(j+1,rows.size()-1); i++){
      for (file_link& l : rows[i].links){
        if (x < l.x - 10 || x > l.x+l.w+10) continue;
        if (y < l.y - 10 || y > l.y + link_size + 10) continue;
        return &l;
      }
    }
    return nullptr;
  }

  void remove_link(int x,int y){
    int j = y / row_h;
        
    for (int i = max(0,j-1); i <= min(j+1,rows.size()-1); i++){
      for (int k = 0; k < rows[i].links.size(); k++){
        auto& l = rows[i].links[k];
        if (x < l.x - 10 || x > l.x+l.w+10) continue;
        if (y < l.y - 10 || y > l.y + link_size + 10) continue;
        rows[i].links.erase(rows[i].links.begin()+k);
        linksedited = true;
        return;
      }
    }
  }
  
  
  

  void draw(){
    int j_st = max(y_scroll/row_h,0);
    int s = (int)rows.size();
    if (j_st >= s) return;
    int j_end = min((y_scroll+h)/row_h,s-1);
    
    for (int j = j_st ; j <= j_end ; j++){
      for (int i = 0 ; i < 16 ; i++)
        for (stroke& k : rows[j].vect[i])
          k.draw(fb,y_scroll,y);
      for (file_link& l : rows[j].links)
        if (l.y > y_scroll)
          fb->draw_text(l.x,l.y-y_scroll+y,l.file,link_size);
    }
  }


  void remove(int x,int y,int r){
    int end_i = min((x+r+1)/row_w,15);
    int end_j = min((y+r+1)/row_h,rows.size()-1);
    int start_i = max((x-r-1)/row_w,0);
    int start_j = max((y-r-1)/row_h,0);
    for (int j = start_j;j<=end_j;j++)
      for (int i = start_i;i<=end_i;i++)
        for (int k = rows[j].vect[i].size()-1 ; k>= 0; k--){
          stroke& st = rows.at(j).vect[i].at(k);
          if (lensq(st.ax-x,st.ay-y) <= r*r) {
            st.undraw(fb,y_scroll,this->y);
            rows[j].vect[i].erase(rows[j].vect[i].begin()+k);
          }
        }
        
    edited = true;
  }
  void redraw(int x,int y,int w,int h){
    int end_i = (x+w)/row_w;
    int end_j = (y+h)/row_h;
    for (int j = y/row_h;j<=end_j;j++){
      for (int i = x/row_w;i<=end_i;i++)
        if (j < (int)rows.size())
          for (stroke& st : rows.at(j).vect[i]){
            st.draw(fb,y_scroll,this->y);
          }
      for (file_link& l : rows[j].links)
        if (l.y > y_scroll)
          fb->draw_text(l.x,l.y-y_scroll+this->y,l.file,link_size);
    }
  }

  void clear() {
    rows.clear();
    edited = true;
    linksedited = true;
    save();
    y_scroll = 0;
  }

  void cut(int x,int y,int r,std::vector<stroke>& out){
    int end_i = min((x+r+1)/row_w,15);
    int end_j = min((y+r+1)/row_h,rows.size()-1);

    int start_i = max((x-r-1)/row_w,0);
    int start_j = max((y-r-1)/row_h,0);
    for (int j = start_j;j<=end_j;j++)
      for (int i = start_i;i<=end_i;i++)
        for (int k = rows[j].vect[i].size()-1 ; k>= 0; k--){
          stroke& st = rows.at(j).vect[i].at(k);
          if (lensq(st.ax-x,st.ay-y) <= r*r) {
            out.push_back(st);
            st.undraw(fb,y_scroll,this->y);
            rows[j].vect[i].erase(rows[j].vect[i].begin()+k);
          }
        }
    edited = true;
  }
};
