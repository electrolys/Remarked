#include "rmkit.h"
#include <tuple>
#include <vector>

#include "sqlite3.h"
#include <cstdio>

struct stroke{
	int ax,ay,bx,by;
	char width, color, type, etc;

	void undraw(framebuffer::FB* fb,int y_scroll,int y){
		if (ay < y_scroll || by < y_scroll) return;
		fb->draw_line_circle(ax,y+ay-y_scroll,bx,y+by-y_scroll,width,WHITE);
	}
	
	void draw(framebuffer::FB* fb,int y_scroll,int y){
		if (ay < y_scroll || by < y_scroll) return;
		fb->draw_line_circle(ax,y+ay-y_scroll,bx,y+by-y_scroll,width,color::SCALE_16[(int)color]);
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
		sql_run(shift_s,"sii",to.c_str(),to_page,1);
		sql_run(shift_l,"sii",to.c_str(),to_page,1);

		sql_run(page_s,"sisi",from.c_str(),from_page,to.c_str(),to_page);
		sql_run(page_l,"sisi",from.c_str(),from_page,to.c_str(),to_page);

		sql_run(shift_s,"sii",from.c_str(),from_page+1,-1);
		sql_run(shift_l,"sii",from.c_str(),from_page+1,-1);
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
		load(current_file,current_page);
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
				sql_run(clear_s,"si",current_file.c_str(),current_page);
				
				sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
				int s = (int)rows.size();
				for (int j = 0 ; j < s ; j++)
					for (int i = 0 ; i < 16 ; i++)
						for (stroke& k : rows[j].vect[i])
							sql_run(write_s,"siiiiiiiii",current_file.c_str(),current_page,k.ax,k.ay,k.bx,k.by,k.width,k.color,k.type,k.etc);	
				sqlite3_exec(db, "END TRANSACTION", NULL, NULL, NULL);
				
				edited = false;
			}
			
			if (linksedited){
				sql_run(clear_l,"si",current_file.c_str(),current_page);
				
				int s = (int)rows.size();
				sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
				for (int j = 0 ; j < s ; j++)
					for (file_link& l : rows[j].links)
						sql_run(write_l,"sisiii",current_file.c_str(),current_page,l.file.c_str(),0,l.x,l.y);
				sqlite3_exec(db, "END TRANSACTION", NULL, NULL, NULL);
			}
			
		}
  }

  void load(const std::string& file,int page){
  	if (!db) return;
		unload();


  	current_file = file;
  	current_page = page;

		

		sql_bind(read_s,"si",file.c_str(),page);
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


		sql_bind(read_l,"si",file.c_str(),page);
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
				if (y < l.y - link_size - 10 || y > l.y + 10) continue;
				return &l;
			}
		}
		return nullptr;
	}

	void remove_link(int x,int y){
		int j = y / row_h;
				
		for (int i = max(0,j-1); i <= min(j+1,rows.size()-1); i++){
			for (int k = rows[i].links.size()-1; k >= 0; k--){
				auto& l = rows[i].links[k];
				if (x < l.x - 10 || x > l.x+l.w+10) continue;
				if (y < l.y - link_size - 10 || y > l.y + 10) continue;
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
				if (l.y > y_scroll + link_size)
					fb->draw_text(l.x,l.y-y_scroll+y-link_size,l.file,link_size);
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
};

inline int my_abs(int x){
	return x<0?-x:x;
}




class NoteBook: public ui::Widget{
public:
    const int DRAW = 0;
    const int ERASER = 1;
    const int SELECT = 2;
    const int LINK = 3;
    const int REM_LINK = 4;
    const int NUM_TOOLS = 3; // I want LINK and REM_LINK in their own buttons
    int px = -1,py = -1,tool = DRAW,prev_tool,block_touch = 0;
		char width = 2;
		char eraser_width = 3;
		int lines = 25*2;
    // bool full_redraw;
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

		void load(std::string file = "Home",int page = 0){
			pagenum->undraw();
			pagenum->text = file+":"+std::to_string(page);
			pagenum->dirty = 1;
   		gr.load(file,page);
		}

		
    NoteBook(int w,int h,int y) : ui::Widget(0,y,w,h){
				pagenum = new ui::Button(w-128,0,128,y,"Home:0");

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
	        		if (x > 0 && gr.current_page > 0){
		        		load(gr.current_file,gr.current_page-1);
	        		}
	        		if (x < 0 && gr.current_page < 999) {
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
   				
       		//add link to link buffer if name is not empty TODO
       		
       		
       	};

        
    }
    void on_mouse_enter(input::SynMotionEvent& e){
				if (input::is_wacom_event(e)){
					px = py = -1;
					if ((e.left && e.left!=-1) || (e.eraser && e.eraser!=-1)) {
		        px = e.x;
		        py = e.y;
	        }  
			  }
    }

    void on_mouse_leave(input::SynMotionEvent& e){
				if (input::is_wacom_event(e)){
	        px = py = -1;
				  if (erased) {dirty = 1;erased=false;}  
			  }
    }

    void on_mouse_up(input::SynMotionEvent& e){
				if (input::is_wacom_event(e)){
	        px = py = -1;
				  if (erased) {dirty = 1;erased=false;}  

					if (click_start) {
						click_start = false;
						if (tool == LINK){
					   	kb.set_text("");
					   	kb.show();
							
							link_x = e.x;
							link_y = e.y+gr.y_scroll-y;
					   	ui::MainLoop::refresh();
					   	tool = prev_tool;
				   	}
				   	if (tool == REM_LINK){
				   		gr.remove_link(e.x,e.y+gr.y_scroll-y);
				   		dirty = 1;
				   		tool = prev_tool;
				   	}
			   	}
			  }

			  
    }
    void on_mouse_down(input::SynMotionEvent& e){
 				if (input::is_wacom_event(e)){
 	        px = py = -1;
 	        if (tool == DRAW) { // this mess is for dots since I wanted to make sure that strokes would have a minimum size otherwise
	 	        stroke st = stroke{e.x,e.y+gr.y_scroll-y,e.x,e.y+gr.y_scroll-y,width,0,0,0};
	       		gr.add(st); 
       		}
       		if (e.left && e.left!=-1) 
	       		if (tool == LINK || tool == REM_LINK)
					   	click_start = true;
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
				// using if statements rather than ignore event, as ignore event seems to cause a bunch of writing problems, with it just ignoring a lot of strokes
				// also I am using gestures and stuff so I can't ignore the touch events
				if (input::is_wacom_event(e)){ 
	        if (!((e.left && e.left!=-1) || (e.eraser && e.eraser!=-1)))
	        	if (e.right && e.right!=-1){
			       	drag_x = -1;
							return;
						}
			    drag_x = -1;
	        
	    		
	        if ((e.eraser && e.eraser!=-1) || tool==ERASER){
	        		px = -2;
	            gr.remove(e.x,e.y+gr.y_scroll-y,eraser_width*8);
	            erased = true;
	        } else if (px < 0 || lensq(e.x-px,e.y-py) > min(16,(width/2)*(width/2))){
						if (tool==DRAW && px >= 0){
               stroke st = stroke{px,py+gr.y_scroll-y,e.x,e.y+gr.y_scroll-y,width,0,0,0};
	             gr.add(st);
	             st.draw(fb,gr.y_scroll,y);
	          }
	        
	        	px = e.x;
	        	py = e.y;						
	        }
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

        fb->dirty = 1;
        fb->waveform_mode = WAVEFORM_MODE_GC16;
        int marker = fb->perform_redraw(true);
        fb->wait_for_redraw(marker);
        dirty = 0;
    }
};



class ToolDropdown : public ui::TextDropdown{
	public:
	NoteBook* NB;
	ToolDropdown(int x, int y, int w, int h, NoteBook* nb): ui::TextDropdown(x,y,w,h,"Tools"){
		auto t = add_section("Tool");
		t->add_options(std::vector<std::string>{"Write","Erase","Select"});
		auto tt = add_section("Size");
		tt->add_options(std::vector<std::string>{"Fine","Normal","Wide","Ex Wide","Fill","Ex Fill"});
		NB = nb;
		dir = DIRECTION::DOWN;
		text = "Write";
	}
	const int widths[6] = { 2,4,6,10,17,27 };
	void on_select(int i){
		if (i >= NB->NUM_TOOLS) {
			NB->width = widths[i-NB->NUM_TOOLS];
			return;
		}
		NB->tool = i;
	}
};





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


		const int tool_height = 48;
    NoteBook* N = new NoteBook(w,h-tool_height,tool_height);
    scene->add(N);

		
		
		ToolDropdown* T = new ToolDropdown(0,0,128,tool_height,N);
		scene->add(T);
		scene->add(N->pagenum);

		{
			ui::Button* b = new ui::Button(128,0,32,tool_height,"+");
			b->mouse.click += [=] (input::SynMotionEvent&){
				if (N->tool != N->LINK && N->tool != N->REM_LINK)
					N->prev_tool = N->tool;
				N->tool = N->LINK;
				T->text = "+Link";
				T->dirty = 1;
			}; 
			scene->add(b);
		}
		{
			ui::Button* b = new ui::Button(160,0,32,tool_height,"-");
			b->mouse.click += [=] (input::SynMotionEvent&){
				if (N->tool != N->LINK && N->tool != N->REM_LINK)
					N->prev_tool = N->tool;	
				N->tool = N->REM_LINK;
				T->text = "-Link";
				T->dirty = 1;
			}; 
			scene->add(b);
		}

		{
			ui::Button* b = new ui::Button(192,0,32,tool_height,"X");
			b->mouse.click += [=] (input::SynMotionEvent&){
				N->gr.save();
				N->gr.move(N->gr.current_file,N->gr.current_page,"Copies",0);
				N->load(N->gr.current_file,N->gr.current_page);
				N->rerender();
			}; 
			scene->add(b);
		}
		{
			ui::Button* b = new ui::Button(224,0,32,tool_height,"V");
			b->mouse.click += [=] (input::SynMotionEvent&){
				N->gr.save();
				N->gr.move("Copies",0,N->gr.current_file,N->gr.current_page);
				N->load(N->gr.current_file,N->gr.current_page);
				N->rerender();
			}; 
			scene->add(b);
		}
		





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
