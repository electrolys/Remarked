#include "rmkit.h"
#include <tuple>
#include <vector>

#include "sqlite3.h"
#include <cstdio>


inline int my_abs(int x){
  return x<0?-x:x;
}


inline void my_draw_rect_fast(framebuffer::FB* fb, int o_x, int o_y, int w, int h, int color) {
  fb->dirty = 1;

  if (o_y >= fb->height || o_x >= fb->width || o_y < 0 || o_x < 0)
    return;

  for (int j = 0; j < h; j++) {
    if (j+o_y >= fb->height)
      break;

    for (int i = 0; i < w; i++) {
      if (i+o_x >= fb->width)
        break;
      fb->_set_pixel(i+o_x, j+o_y, color);
    }
  }
}

inline void my_draw_horiz_fast(framebuffer::FB* fb, int o_x, int o_y, int l, int color) {
  fb->dirty = 1;

  if (o_y >= fb->height || o_x >= fb->width || o_y < 0 || o_x < 0)
    return;

  for (int i = 0; i < l; i++) {
    if (i+o_x >= fb->width)
      break;
    fb->_set_pixel(i+o_x, o_y, color);
  }
  
}

inline void my_draw_vert_fast(framebuffer::FB* fb, int o_x, int o_y, int l, int color) {
  fb->dirty = 1;

  if (o_y >= fb->height || o_x >= fb->width || o_y < 0 || o_x < 0)
    return;

  for (int i = 0; i < l; i++) {
    if (i+o_y >= fb->height)
      break;
    fb->_set_pixel(o_x, i+o_y, color);
  }
  
}

inline void my_draw_circle_fast(framebuffer::FB* fb, int o_x, int o_y, int r, int color) {
  fb->dirty = 1;

  if (o_y-r >= fb->height || o_x-r >= fb->width || o_y+r < 0 || o_x+r < 0)
    return;

  for (int j = max(-r,-o_y); j <= r; j++) {
    if (j+o_y >= fb->height)
      break;

    for (int i = max(-r,-o_x); i <= r; i++) {
      if (i+o_x >= fb->width)
        break;
      if (i*i+j*j <= r*r)
        fb->_set_pixel(i+o_x, j+o_y, color);
    }
  }
}

void my_draw_line_circle(framebuffer::FB* fb,int x0,int y0,int x1,int y1,int width,int color){
      fb->dirty = 1;
      int dx =  abs(x1-x0);
      int sx = x0<x1 ? 1 : -1;
      int dy = -abs(y1-y0);
      int sy = y0<y1 ? 1 : -1;
      int err = dx+dy;
      fb->update_dirty(fb->dirty_area, min(x0,x1)-width/2-1, min(y0,y1)-width/2-1);
      fb->update_dirty(fb->dirty_area, max(x0,x1)+width/2+1, max(y0,y1)+width/2+1);
      my_draw_circle_fast(fb,x0, y0, width / 2, color);

      while (true){
        my_draw_vert_fast(fb,x0, y0-width/2, width, color);
        my_draw_horiz_fast(fb,x0-width/2, y0, width, color);
        if (x0==x1 && y0==y1) {
          my_draw_circle_fast(fb,x0, y0, width / 2, color);
          break;
        }
        int e2 = 2*err;
        if (e2 >= dy) {
          err += dy;
          x0 += sx;
        }
        if (e2 <= dx){
          err += dx;
          y0 += sy;
        }
      }
}

void my_draw_line_vert(framebuffer::FB* fb,int x0,int y0,int x1,int y1,int width,int color){
      fb->dirty = 1;
      int dx =  abs(x1-x0);
      int sx = x0<x1 ? 1 : -1;
      int dy = -abs(y1-y0);
      int sy = y0<y1 ? 1 : -1;
      int err = dx+dy;
      
      fb->update_dirty(fb->dirty_area, min(x0,x1)-1, min(y0,y1)-width/2-1);
      fb->update_dirty(fb->dirty_area, max(x0,x1)+1, max(y0,y1)+width/2+1);
      
      // fb->draw_circle(x0, y0, width/2, 1, color,true);
      while (true){
        // fb->_draw_rect_fast(x0-width/2, y0, width, 1, color);
        my_draw_vert_fast(fb,x0, y0-width/2, width, color);
        
        if (x0==x1 && y0==y1) {
          // fb->draw_circle(x0, y0, width/2, 1, color,true);
          break;
        }
        int e2 = 2*err;
        if (e2 >= dy) {
          err += dy;
          x0 += sx;
        }
        if (e2 <= dx){
          err += dx;
          y0 += sy;
        }
      }
}



struct stroke{
  int ax,ay,bx,by;
  char width, color, type, etc;

  stroke& undraw(framebuffer::FB* fb,int y_scroll,int y,int off_x = 0,int off_y = 0){
    if (ay+off_y-width/2 < y_scroll || by+off_y-width/2 < y_scroll) return *this;
    my_draw_line_circle(fb,ax+off_x,y+off_y+ay-y_scroll,bx+off_x,y+off_y+by-y_scroll,width,WHITE);
    return *this;
  }
  
  stroke& draw(framebuffer::FB* fb,int y_scroll,int y,int off_x = 0,int off_y = 0){
    if (ay+off_y < y_scroll || by+off_y < y_scroll) return *this;
    my_draw_line_circle(fb,ax+off_x,y+off_y+ay-y_scroll,bx+off_x,y+off_y+by-y_scroll,width,color::SCALE_16[(int)color]);
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

const char* write_link_str = "insert into file_links (file, page, to_file, to_page, x, y) values (?, ?, ?, ?, ?, ?);";
const char* read_link_str = "select to_file, to_page, x, y from file_links where file=? and page=?;";

const char* clear_st_str = 
"delete from pen_strokes where file=? and page=?;";
const char* clear_link_str =
"delete from file_links where file=? and page=?;";


const char* init_st_str = 
"PRAGMA synchronous = OFF;\n"
"PRAGMA journal_mode = MEMORY;\n"
"create table if not exists file_links (file text, page int, to_file text, to_page int, x int, y int) strict;\n" // I have to_page just incase but frankly I don't want to use it cause it'll cause a lot of problems with inaccurate links once I have page deleting
"create table if not exists pen_strokes (file text, page int, ax int, ay int, bx int, by int, size int, color int, type int, etc int) strict;";


const char* shift_st_str = 
"update pen_strokes set page = page + ?3 where file = ?1 and page >= ?2;";

const char* setpage_st_str = 
"update pen_strokes set file = ?3, page = ?4 where file = ?1 and page = ?2;";

const char* shift_link_str = 
"update file_links set page = page + ?3 where file = ?1 and page >= ?2;";

const char* setpage_link_str = 
"update file_links set file = ?3, page = ?4 where file = ?1 and page = ?2;";



const int link_size = 32;

inline int lensq(int x,int y){return x*x+y*y;}
inline int min(int x,int y){return x<y?x:y;}
inline int max(int x,int y){return x>y?x:y;}


void error_msg(framebuffer::FB* fb, std::string t){
  fb->clear_screen();
  fb->draw_text(0,0,t,50);
  fb->update_mode = UPDATE_MODE_FULL;
  fb->waveform_mode = WAVEFORM_MODE_GC16;
  int marker = fb->perform_redraw(true);
  fb->wait_for_redraw(marker);
}


void sql_bind_v(sqlite3_stmt* stmt, const char* args, va_list varg) {
  sqlite3_reset(stmt);
  if (args)
    for (int i = 0 ; args[i]; i++) 
      switch (args[i]) {
        case 'I':
          sqlite3_bind_int64(stmt,i+1,va_arg(varg,sqlite3_int64));
          break;
        case 'i':
          sqlite3_bind_int(stmt,i+1,va_arg(varg,int));
          break;
        case 'S': {
          int s = va_arg(varg,int);
          sqlite3_bind_text(stmt,i+1,va_arg(varg,const char*),s,SQLITE_TRANSIENT);
          break;
        }
        case 's':
          sqlite3_bind_text(stmt,i+1,va_arg(varg,const char*),-1,SQLITE_TRANSIENT);
          break;
        case 'b': {
          int s = va_arg(varg,int);
          sqlite3_bind_blob(stmt,i+1,va_arg(varg,const void*),s,SQLITE_TRANSIENT);
          break;
        }
          
        case 'B': {
          sqlite3_uint64 s = va_arg(varg,sqlite3_uint64);
          sqlite3_bind_blob64(stmt,i+1,va_arg(varg,const void*),s,SQLITE_TRANSIENT);
          break;
        }
        case 'D':
        case 'd':
        case 'F':
        case 'f':
          sqlite3_bind_double(stmt,i+1,va_arg(varg,double));
          break;
        case 'N':
        case 'n':
          sqlite3_bind_null(stmt,i+1);
          break;
        case 'Z':
          sqlite3_bind_zeroblob64(stmt,i+1,va_arg(varg,sqlite3_uint64));
          break;
        case 'z':
          sqlite3_bind_zeroblob(stmt,i+1,va_arg(varg,int));
          break;
      }
}

void sql_bind(sqlite3_stmt* stmt, const char* args, ...) {
  va_list varg;
  va_start(varg,args);
  sql_bind_v(stmt,args,varg);
  va_end(varg);
}


void sql_run(sqlite3_stmt* stmt, const char* args, ...) {
  va_list varg;
  va_start(varg,args);
  sql_bind_v(stmt,args,varg);
  va_end(varg);
  while (sqlite3_step(stmt) != SQLITE_DONE){}//TODO error handling
}



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
  sqlite3_stmt* read_s, *write_s, *clear_s;
  sqlite3_stmt* read_l, *write_l, *clear_l;
  
  sqlite3_stmt* shift_s, *page_s, *shift_l, *page_l;

  void move(std::string from, int from_page, std::string to, int to_page) {
    sql_run(shift_s,"sii",to.c_str(),my_abs(to_page),1);
    sql_run(shift_l,"sii",to.c_str(),my_abs(to_page),1);

    sql_run(page_s,"sisi",from.c_str(),my_abs(from_page),to.c_str(),my_abs(to_page));
    sql_run(page_l,"sisi",from.c_str(),my_abs(from_page),to.c_str(),my_abs(to_page));

    sql_run(shift_s,"sii",from.c_str(),my_abs(from_page)+1,-1);
    sql_run(shift_l,"sii",from.c_str(),my_abs(from_page)+1,-1);
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
    
    if (sqlite3_prepare_v2(db,read_st_str,-1,&read_s,NULL))
      error_msg(fb,(const char*)sqlite3_errmsg(db));
    if (sqlite3_prepare_v2(db,read_link_str,-1,&read_l,NULL))
      error_msg(fb,(const char*)sqlite3_errmsg(db));
    if (sqlite3_prepare_v2(db,write_st_str,-1,&write_s,NULL))
      error_msg(fb,(const char*)sqlite3_errmsg(db));
    if (sqlite3_prepare_v2(db,write_link_str,-1,&write_l,NULL))
      error_msg(fb,(const char*)sqlite3_errmsg(db));
    if (sqlite3_prepare_v2(db,clear_st_str,-1,&clear_s,NULL))
      error_msg(fb,(const char*)sqlite3_errmsg(db));
    if (sqlite3_prepare_v2(db,clear_link_str,-1,&clear_l,NULL))
      error_msg(fb,(const char*)sqlite3_errmsg(db));
    if (sqlite3_prepare_v2(db,shift_st_str,-1,&shift_s,NULL))
      error_msg(fb,(const char*)sqlite3_errmsg(db));
    if (sqlite3_prepare_v2(db,shift_link_str,-1,&shift_l,NULL))
      error_msg(fb,(const char*)sqlite3_errmsg(db));
    if (sqlite3_prepare_v2(db,setpage_st_str,-1,&page_s,NULL))
      error_msg(fb,(const char*)sqlite3_errmsg(db));
    if (sqlite3_prepare_v2(db,setpage_link_str,-1,&page_l,NULL))
      error_msg(fb,(const char*)sqlite3_errmsg(db));
    load(current_file,my_abs(current_page));
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
        sql_run(clear_s,"si",current_file.c_str(),my_abs(current_page));
        
        sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
        int s = (int)rows.size();
        for (int j = 0 ; j < s ; j++)
          for (int i = 0 ; i < 16 ; i++)
            for (stroke& k : rows[j].vect[i])
              sql_run(write_s,"siiiiiiiii",current_file.c_str(),my_abs(current_page),k.ax,k.ay,k.bx,k.by,k.width,k.color,k.type,k.etc); 
        sqlite3_exec(db, "END TRANSACTION", NULL, NULL, NULL);
        
        edited = false;
      }
      
      if (linksedited){
        sql_run(clear_l,"si",current_file.c_str(),current_page);
        
        int s = (int)rows.size();
        sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
        for (int j = 0 ; j < s ; j++)
          for (file_link& l : rows[j].links)
            sql_run(write_l,"sisiii",current_file.c_str(),my_abs(current_page),l.file.c_str(),0,l.x,l.y);
        sqlite3_exec(db, "END TRANSACTION", NULL, NULL, NULL);
      }
      
    }
  }

  void load(const std::string& file,int page){
    if (!db) return;
    unload();


    current_file = file;
    current_page = page;

    

    sql_bind(read_s,"si",file.c_str(),my_abs(page));
    int t = 0;
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


    sql_bind(read_l,"si",file.c_str(),my_abs(page));
    t = 0;
    while ((t=sqlite3_step(read_l)) != SQLITE_DONE){
      if (t == SQLITE_BUSY) continue;
      if (t == SQLITE_ROW) {
        const unsigned char* s = sqlite3_column_text(read_l,0);
        std::string t((const char*)s);
        add_link(sqlite3_column_int(read_l,2),sqlite3_column_int(read_l,3),t);
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
    int end_i = (x+r+1)/row_w;
    int end_j = (y+r+1)/row_h;
    for (int j = (y-r-1)/row_h;j<=end_j;j++)
      for (int i = (x-r-1)/row_w;i<=end_i;i++)
        if (j < (int)rows.size())
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

  void cut(int x,int y,int r,std::vector<stroke>& out){
    int end_i = (x+r+1)/row_w;
    int end_j = (y+r+1)/row_h;
    for (int j = (y-r-1)/row_h;j<=end_j;j++)
      for (int i = (x-r-1)/row_w;i<=end_i;i++)
        if (j < (int)rows.size())
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


const int MOVE_SEL = -4;
const int UNDO_SEL = -3;
const int LINK_SEL = -2;
const int NUL_ST = -1;

const int DRAW = 0;
const int ERASER = 1;
const int SELECT = 2;
const int LINK = 3;
const int REM_LINK = 4;

const int tool_height = 48;

class NoteBook: public ui::Widget{
public:
    int px = -1,py = -1,tool = DRAW,block_touch = 0;
    char width = 2;
    char eraser_width = 3;
    int lines = 25*2;
    grid gr;

    ui::Button* pagenum;

    

    int drag_x=-1,drag_y=-1;

    bool erased = false,click_start = false;

    void refresh_screen(){
      ui::MainLoop::refresh();
      
      fb->update_mode = UPDATE_MODE_FULL;
      fb->waveform_mode = WAVEFORM_MODE_GC16;
      int marker = fb->perform_redraw(true);
      fb->wait_for_redraw(marker);
      dirty = 1;
    }

    void rerender(){
      ui::MainLoop::refresh();
      fb->update_mode = UPDATE_MODE_FULL;
      render();
    }

    std::vector<stroke> selection;
    int sel_x=-1,sel_y=-1,sel_w=-1,sel_h=-1;
    bool no_select = false;

    void unselect() {
      if (sel_x >= 0) {
        for (stroke& s : selection) {
          s.ax += sel_x;
          s.bx += sel_x;
          s.ay += sel_y;
          s.by += sel_y;
          gr.add(s);
        }
        sel_x = -1;
        selection.clear();        
      }
    }
    
    void load(std::string file = "Home",int page = 0){
      unselect();
      pagenum->undraw();
      pagenum->text = file+":"+std::to_string(my_abs(page)+1);
      pagenum->dirty = 1;
      gr.load(file,page);
    }

    
    NoteBook(int w,int h,int y) : ui::Widget(0,y,w,h){
        pagenum = new ui::Button(w-128,0,128,y,"Home:1");

        pagenum->mouse.click += [this] (input::SynMotionEvent&){
          load();
          rerender();
        };
        gr.init(w,h,y,fb);
        dirty = 1;

        gestures.drag_start += PLS_LAMBDA(auto& e){
          if (input::is_touch_event(e)) {
            drag_x = e.x;
            drag_y = e.y;
          }
        };

        gestures.drag_end += PLS_LAMBDA(auto& e){
          if (input::is_touch_event(e) && drag_x != -1){
            int x = e.x - drag_x;
            int y = e.y - drag_y;
            if (my_abs(x) > my_abs(y)*2 && my_abs(x) > this->h/8){
              undraw();
              if (x > 0 && gr.current_page > -998){
                load(gr.current_file,gr.current_page-1);
              }
              if (x < 0 && gr.current_page < 998) {
                load(gr.current_file,gr.current_page+1);
              }
              rerender();
            }
            
            if (my_abs(y) > my_abs(x)*2 && my_abs(y) > this->h/8){
              gr.y_scroll += (y<0 ? 1 : -1) * (this->h/2);
              if (gr.y_scroll < 0) gr.y_scroll = 0;
              dirty = 1;
            }
            drag_x = -1;
          }
        };

        kb.events.done += [this](auto& e){
          this->dirty = 1;
          if (e.text.length() > 0)
          this->gr.add_link(this->link_x,this->link_y,e.text);
        };

        
    }

    

    void set_sel_bounds() {
      int xmax = 0, ymax = 0;
      
      if (!selection.size()) {sel_x = -1;return;}

      for (stroke& s : selection) {
        if (sel_x == -1 ) {
          sel_x = min(s.ax-s.width/2,s.bx-s.width/2);
          sel_y = min(s.ay-s.width/2,s.by-s.width/2);
          xmax = max(s.ax+s.width/2,s.bx+s.width/2);
          ymax = max(s.ay+s.width/2,s.by+s.width/2);
        } else {
          sel_x = min(min(s.ax-s.width/2,sel_x),s.bx-s.width/2);
          sel_y = min(min(s.ay-s.width/2,sel_y),s.by-s.width/2);
          xmax = max(max(s.ax+s.width/2,xmax),s.bx+s.width/2);
          ymax = max(max(s.ay+s.width/2,ymax),s.by+s.width/2);
        }
      }
       
      sel_w = xmax - sel_x + 4;
      sel_h = ymax - sel_y + 4;
      sel_x -= 2;
      sel_y -= 2;
      for (stroke& s : selection) {
        s.ax = s.ax - sel_x;
        s.ay = s.ay - sel_y; 
        s.bx = s.bx - sel_x;
        s.by = s.by - sel_y;
      }
      sel_x = max(sel_x,0);
      
    }
    
    void draw_sel() {
      if (sel_x >= 0){
        for (stroke& s : selection)
          s.draw(fb,gr.y_scroll,y,sel_x,sel_y);
        fb->draw_rect(sel_x, sel_y-gr.y_scroll+y, sel_w, sel_h, BLACK, false);
        dirty = 0;
      }
    }
    void undraw_sel() {
      fb->draw_rect(sel_x, sel_y-gr.y_scroll+y, sel_w, sel_h, WHITE, true);
      if (lines) {
        int y_s = ((sel_y-1) / lines + 1) * lines;
        for (int i = y_s ; i < sel_y + sel_h; i+=lines)
          fb->draw_line(sel_x,i-gr.y_scroll+y,sel_x+sel_w,i-gr.y_scroll+y,1,color::SCALE_16[8]);
      }
      gr.redraw(sel_x,sel_y,sel_w,sel_h);
    }

    
    
    int state = 0;
    std::string sel_name;
    void start_stroke(int tool,input::SynMotionEvent& e) {
      px = py = -1;
      state = tool;
      file_link* l;
      switch (state) {
        case DRAW:
          gr.add(stroke{e.x,e.y+gr.y_scroll-y,e.x,e.y+gr.y_scroll-y,width,0,0,0}.draw(fb,gr.y_scroll,y));
          break;
        case ERASER:
          break;
        case SELECT:
          if (sel_x >= 0 && e.x < sel_x+sel_w && e.x >= sel_x && e.y+gr.y_scroll-y < sel_y+sel_h && e.y+gr.y_scroll-y >= sel_y){
            state = MOVE_SEL;
            break;
          }
          if (sel_x >= 0){
            state = UNDO_SEL;
            break;
          }
          if ((l = gr.get_link(e.x,e.y+gr.y_scroll-y))) {
            sel_x = l->x;
            sel_y = l->y;
            sel_w = l->w;
            sel_h = link_size;
            
            sel_name = l->file;

            gr.remove_link(e.x,e.y+gr.y_scroll-y);
            state = LINK_SEL;
            break;
          }
          unselect();
          break;
      }
    }

    void update_stroke(input::SynMotionEvent& e){
      switch (state) {
        case DRAW:
          if (px < 0 || lensq(e.x-px,e.y-py) > max(min(16,(width/3)*(width/3)),1)){
            if (px >= 0)
              gr.add(stroke{px,py+gr.y_scroll-y,e.x,e.y+gr.y_scroll-y,width,0,0,0}.draw(fb,gr.y_scroll,y));
            px = e.x;
            py = e.y;
          }
          break;
        case ERASER:
          px = -2;
          gr.remove(e.x,e.y+gr.y_scroll-y,eraser_width*8);
          break;
        case SELECT:
          gr.cut(e.x,e.y+gr.y_scroll-y,eraser_width*8,selection);
          break;
        case MOVE_SEL:
          if (px < 0 || lensq(e.x-px,e.y-py) > 32){
            if (px >= 0) {
              undraw_sel();
              sel_x += e.x-px;
              sel_y += e.y-py;
              draw_sel();
            }
            px = e.x;
            py = e.y;
          }
          break;
        case LINK_SEL:
          if (px < 0 || lensq(e.x-px,e.y-py) > 32){
            if (px >= 0) {
              undraw_sel();            
              sel_x += e.x-px;
              sel_y += e.y-py;
              fb->draw_text(sel_x,sel_y-gr.y_scroll+y,sel_name,link_size);
            }
            px = e.x;
            py = e.y;
          }
          break;
        case LINK:
        case REM_LINK:
          px = e.x;
          py = e.y;
      }
    }

    void end_stroke(){
      switch (state) {
        case DRAW:
          break;
        case ERASER:
          dirty = 1;
          break;
        case SELECT:
          set_sel_bounds();
          draw_sel();
          state = NUL_ST;
          break;
        case UNDO_SEL:
          unselect();
          rerender();
          state = NUL_ST;
          break;
        
        case MOVE_SEL:
          state = NUL_ST;
          break;
        case LINK_SEL:
          
          gr.add_link(sel_x,sel_y,sel_name);
          sel_name = "";
          state = NUL_ST;
          sel_x = -1;
          rerender();
          break;
        case LINK:
          kb.set_text("");
          kb.show();
          
          link_x = px;
          link_y = py+gr.y_scroll-y-link_size;
          ui::MainLoop::refresh();
          state = NUL_ST;
          break;
        case REM_LINK:
          gr.remove_link(px,py+gr.y_scroll-y);
          dirty = 1;
          state = NUL_ST;
          break;
      }
    }

    
    void on_mouse_enter(input::SynMotionEvent& e){
        if (input::is_wacom_event(e)){
          px = py = -1;
          is_started = false;
          if (e.left && e.left!=-1) {
            px = e.x;
            py = e.y;
          }
        }
    }

    bool is_started = false;  
    void on_mouse_leave(input::SynMotionEvent& e){
        if (input::is_wacom_event(e)){
          if (is_started) end_stroke();
          is_started = false;
          px = py = -1;
        }
    }

    void on_mouse_up(input::SynMotionEvent& e){
        if (input::is_wacom_event(e)){
          if (is_started) end_stroke();
          is_started = false;
          px = py = -1;
        }
    }

    int link_x,link_y;
    ui::Keyboard kb;
    void on_mouse_click(input::SynMotionEvent& e){
      if (input::is_touch_event(e)){
        file_link* l = gr.get_link(e.x,e.y+gr.y_scroll-y);
        if (l){
          load(l->file);
          rerender();
        }
      }
    }
    
    void on_mouse_move(input::SynMotionEvent& e){
        if (input::is_wacom_event(e)){ 
          if (!((e.left && e.left!=-1) || (e.eraser && e.eraser!=-1))){
            if (e.right && e.right!=-1){
              if (is_started){
                end_stroke();
                is_started = false;
                px = py = -1;
              }
              drag_x = -1;
            }
            return;
          }
          drag_x = -1;
          if (!is_started) {
            if (e.eraser && e.eraser!=-1)
              start_stroke(ERASER,e);
            else
              start_stroke(tool,e);
            is_started = true;
            return;
          }
          
          update_stroke(e);
        }
    }

    
    void handle_motion_event(input::SynMotionEvent &e){
       if (input::is_touch_event(e) && px!=-1){
          e.stop_propagation();
          return;
       }
    }

    

    
    void render(){
        fb->draw_rect(x, y, w, h, WHITE, true);
        
        if (lines) {
          int y_s = lines - gr.y_scroll % lines;
          for (int i = y_s ; i < h; i+=lines) {
            fb->draw_line(0,y+i,w,y+i,1,color::SCALE_16[8]);
          }
        }
        gr.draw();
        
        fb->draw_line(0,y,w,y,1,BLACK);

        fb->draw_rect(tool*32,tool_height,32,4,BLACK,true);

        fb->dirty = 1;
        fb->waveform_mode = WAVEFORM_MODE_GC16;
        int marker = fb->perform_redraw(true);
        fb->wait_for_redraw(marker);
        dirty = 0;
    }
};




ui::Button* tool_button(NoteBook* N,int id,const char* ch) {
  ui::Button* b = new ui::Button(id*32,0,32,tool_height,ch);
  b->mouse.click += [N, id] (input::SynMotionEvent&){
    N->fb->draw_rect(N->tool*32,tool_height,32,4,WHITE,true);
    N->fb->draw_rect(id*32,tool_height,32,4,BLACK,true);
    N->tool = id;
  }; 
  return b;
}

int main(int,char**){    
    auto scene = ui::make_scene();
    auto sleep_scene = ui::make_scene();
    ui::MainLoop::set_scene(scene);
    bool sleep = false;
    
    auto fb = framebuffer::get();
    fb->clear_screen();
    fb->redraw_screen();

    tuple<int,int> s = fb->get_display_size();
    int w = std::get<0>(s);
    int h = std::get<1>(s);


    
    NoteBook* N = new NoteBook(w,h-tool_height,tool_height);
    scene->add(N);
    scene->add(N->pagenum);

    scene->add(tool_button(N,DRAW,"W"));
    scene->add(tool_button(N,ERASER,"E"));
    scene->add(tool_button(N,SELECT,"S"));
    scene->add(tool_button(N,LINK,"+"));
    scene->add(tool_button(N,REM_LINK,"-"));
   
    {
      ui::Button* b = new ui::Button(160,0,32,tool_height,"X");
      b->mouse.click += [=] (input::SynMotionEvent&){
        N->gr.save();
        N->gr.move(N->gr.current_file,N->gr.current_page,"Copies",0);
        N->load(N->gr.current_file,N->gr.current_page);
        N->rerender();
      }; 
      scene->add(b);
    }
    
    {
      ui::Button* b = new ui::Button(192,0,32,tool_height,"V");
      b->mouse.click += [=] (input::SynMotionEvent&){
        N->gr.save();
        N->gr.move("Copies",0,N->gr.current_file,N->gr.current_page);
        N->load(N->gr.current_file,N->gr.current_page);
        N->rerender();
      }; 
      scene->add(b);
    }
    

    // ui::TextDropdown* D = new ui::TextDropdown(300,0,100,48,"Whyyy");
    // scene->add(D);

    ui::RangeInput* range = new ui::RangeInput(240, 0, 128, tool_height);
    range->percent = 0;
    range->set_range(1, 10);
    range->events.change += [=](float){
      N->width = range->get_value()*2;
    };
    scene->add(range);
    

    ui::MainLoop::hide_overlay(NULL);


    ui::MainLoop::motion_event += PLS_DELEGATE(N->handle_motion_event);

    
    ui::MainLoop::key_event += [&](input::SynKeyEvent& e) {
      if (e.is_pressed){
        switch (e.key) {
          case KEY_POWER:
            sleep = !sleep;
            if (sleep) {
              N->kb.hide();
              N->fb->clear_screen();
              N->gr.save();
              ui::MainLoop::set_scene(sleep_scene);
              N->refresh_screen();
              N->gr.close();
            }
            else {
              N->fb->clear_screen();
              ui::MainLoop::set_scene(scene);
              N->gr.open();
              N->refresh_screen();
            }
            break;
          default:
            ui::MainLoop::handle_key_event(e);
            return;
        }
        e.stop_propagation();
      }
    };
    while (true){
        ui::MainLoop::main();
        ui::MainLoop::redraw();
        ui::MainLoop::read_input();
    }
}
