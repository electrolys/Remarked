#include "grid.hpp"

stroke& stroke::undraw(framebuffer::FB* fb,int y_scroll,int y,int off_x,int off_y ){
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

stroke& stroke::draw(framebuffer::FB* fb,int y_scroll,int y,int off_x,int off_y){
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

const char* del_dat_str =
"delete from page_data where file=? and page=? and key=?;";


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


std::string link_render_text(const std::string& text) {

  std::string file = text;
  if (file[0] == '(') {
    std::size_t end = file.find(")");
    if (end!=std::string::npos){
      file = file.substr(1,end-1);
    }
  }
  return file;
  
}





void grid::move(std::string from, int from_page, std::string to, int to_page) {
  if (from == to) {
    sql_run(page_s,"sisi",from.c_str(),(from_page),to.c_str(),-1);
    sql_run(page_l,"sisi",from.c_str(),(from_page),to.c_str(),-1);
    sql_run(page_d,"sisi",from.c_str(),(from_page),to.c_str(),-1);

    sql_run(shift_s,"sii",from.c_str(),(from_page)+1,-1);
    sql_run(shift_l,"sii",from.c_str(),(from_page)+1,-1);
    sql_run(shift_d,"sii",from.c_str(),(from_page)+1,-1);

    sql_run(shift_s,"sii",to.c_str(),(to_page),1);
    sql_run(shift_l,"sii",to.c_str(),(to_page),1);
    sql_run(shift_d,"sii",to.c_str(),(to_page),1);

    sql_run(page_s,"sisi",from.c_str(),-1,to.c_str(),(to_page));
    sql_run(page_l,"sisi",from.c_str(),-1,to.c_str(),(to_page));
    sql_run(page_d,"sisi",from.c_str(),-1,to.c_str(),(to_page));
    return;
  }
  sql_run(shift_s,"sii",to.c_str(),(to_page),1);
  sql_run(shift_l,"sii",to.c_str(),(to_page),1);
  sql_run(shift_d,"sii",to.c_str(),(to_page),1);
  
  sql_run(page_s,"sisi",from.c_str(),(from_page),to.c_str(),(to_page));
  sql_run(page_l,"sisi",from.c_str(),(from_page),to.c_str(),(to_page));
  sql_run(page_d,"sisi",from.c_str(),(from_page),to.c_str(),(to_page));
  
  sql_run(shift_s,"sii",from.c_str(),(from_page)+1,-1);
  sql_run(shift_l,"sii",from.c_str(),(from_page)+1,-1);
  sql_run(shift_d,"sii",from.c_str(),(from_page)+1,-1);
}

void grid::del(std::string file, int page) {
  sql_run(clear_s,"si",file.c_str(),page);
  sql_run(clear_l,"si",file.c_str(),page);
  sql_run(clear_d,"si",file.c_str(),page);

  sql_run(shift_s,"sii",file.c_str(),(page)+1,-1);
  sql_run(shift_l,"sii",file.c_str(),(page)+1,-1);
  sql_run(shift_d,"sii",file.c_str(),(page)+1,-1);
}

void grid::open(){
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
  
  prepare(del_dat_str,del_d);

  #undef prepare


  //load(current_file,(current_page));
}

void grid::close(){
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

void grid::init(int w,int h,int y,framebuffer::FB* FB){
  row_h = h/16+1;
  row_w = w/16+1;
  y_scroll = 0;
  this->w = w;
  this->h = h;
  this->y = y;
  fb = FB;
  open();
}
void grid::save(){
  if (!db) return;
  if (loaded){
    if (edited){
      sql_run(clear_s,"si",current_file.c_str(),(current_page));
      
      sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
      int s = (int)rows.size();
      for (int j = 0 ; j < s ; j++)
        for (int i = 0 ; i < 16 ; i++)
          for (stroke& k : rows[j].vect[i])
            sql_run(write_s,"siiiiiiiii",current_file.c_str(),(current_page),k.ax,k.ay,k.bx,k.by,k.width,k.color,k.type,k.etc); 
      sqlite3_exec(db, "END TRANSACTION", NULL, NULL, NULL);
      
      edited = false;
    }
    
    if (linksedited){
      sql_run(clear_l,"si",current_file.c_str(),current_page);
      
      int s = (int)rows.size();
      sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
      for (int j = 0 ; j < s ; j++)
        for (file_link& l : rows[j].links)
          sql_run(write_l,"sisii",current_file.c_str(),(current_page),l.file.c_str(),l.x,l.y);
      sqlite3_exec(db, "END TRANSACTION", NULL, NULL, NULL);
    }
    
  }
}

void grid::load(const std::string& file,int page){
  if (!db) return;
  unload();


  current_file = file;
  current_page = page;

  

  sql_bind(read_s,"si",file.c_str(),(page));
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
  

  sql_bind(read_l,"si",file.c_str(),(page));
  while ((t=sqlite3_step(read_l)) != SQLITE_DONE){
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

grid::~grid() {
  unload();
  close();
}

void grid::unload() {
  if (!db) return;
  save();
  rows.clear();
  loaded = false;
}

const char* pen_names[] = {
  "Round",
  "Vertical",
  "Horizontal",
};
const int link_size = 32;

void grid::add(stroke& st){
  int i = st.ax / row_w;
  int j = st.ay / row_h;
  if (j>=(int)rows.size()){
    rows.resize(j+1);
  }
  rows[j].vect[i].push_back(st);
  edited = true;
}
void grid::add_link(int x,int y,std::string file){
  int j = y / row_h;
  if (j>=(int)rows.size()){
    rows.resize(j+1);
  }

  std::string vfile = link_render_text(file);
  
  auto s = stbtext::get_text_size(vfile,link_size);
  
  rows[j].links.push_back(file_link{x,y,s.w,file});
  linksedited = true;
}


file_link* grid::get_link(int x,int y){
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

void grid::remove_link(int x,int y){
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

void grid::draw(){    
  redraw(0,y_scroll,w,h);
}


void grid::remove(int x,int y,int r){
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

void grid::draw_link(file_link& l){
  fb->draw_text(l.x,l.y-y_scroll+y,link_render_text(l.file),link_size);
}

void grid::redraw(int x,int y,int w,int h){
  int end_i = min((x+w)/row_w,15);
  int end_j = min((y+h)/row_h,rows.size()-1);
  
  for (int j = max(y/row_h,0);j<=end_j;j++){
    for (int i = max(x/row_w,0);i<=end_i;i++)
      for (stroke& st : rows.at(j).vect[i]){
        st.draw(fb,y_scroll,this->y);
      }
    for (file_link& l : rows[j].links)
      if (l.y > y_scroll)
        draw_link(l);
  }
}

void grid::clear() {
  rows.clear();
  edited = true;
  linksedited = true;
  save();
  y_scroll = 0;
}

void grid::cut(int x,int y,int r,std::vector<stroke>& out){
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
