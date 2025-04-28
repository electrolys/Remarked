#include "rmkit.h"
#include <tuple>
#include <vector>

#include <chrono>

#include "assets.h"

#include "drawing.hpp"
#include "common.hpp"
#include "grid.hpp"
#include <cstdio>



const int REM_LINK = -5;
const int MOVE_SEL = -4;
const int UNDO_SEL = -3;
const int LINK_SEL = -2;
const int NUL_ST = -1;


const int DRAW = 0;
const int ERASER = 1;
const int SELECT = 2;
const int LINK = 3;


const int tool_height = 48;

class NoteBook: public ui::Widget{
public:
    int px = -1,py = -1,tool = DRAW,block_touch = 0;
    int width = 2, pen_type = 0, eraser_width = 3, select_width = 3;
    int lines = 50;
    grid gr;

    bool rtl = false;

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

    std::vector<stroke> selection, clipboard;
    bool clipboard_full = false;
    
    int sel_x=-1,sel_y=-1,sel_w=0,sel_h=0;
    bool no_select = false;

    
    
    void load(std::string file = "Home",int page = 0){
      unselect();
      pagenum->undraw();
      pagenum->text = file+":"+std::to_string(abs(page)+1);
      pagenum->dirty = 1;
      gr.load(file,page);

      sql_run(gr.write_d,"sisi",file.c_str(),-1,"last_page",page);

      if (!sql_geti(gr.read_d,"sis",lines,file.c_str(),page,"vert_lines")){
        if (!sql_geti(gr.read_d,"sis",lines,file.c_str(),0,"vert_lines")){
          sql_run(gr.write_d,"sisi",file.c_str(),0,"vert_lines",lines);
        }
      }
      
      int b;
      if (sql_geti(gr.read_d,"sis",b,file.c_str(),-1,"RTL")){
        rtl = b;
      }
    }

    


    
    
    NoteBook(int w,int h,int y) : ui::Widget(0,y,w,h){
        pagenum = new ui::Button(w-160-64,0,160,y,"Home:1");
        

        pagenum->mouse.click += [this] (input::SynMotionEvent&){
          load(gr.current_file);
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
            if (abs(x) > abs(y)*2 && abs(x) > this->h/8){
              undraw();

              int to_page = gr.current_page;
              if (rtl){
                to_page = gr.current_page-1;
                if (x > 0)
                  to_page = gr.current_page+1;
              }else {
                to_page = gr.current_page+1;
                if (x > 0)
                  to_page = gr.current_page-1;
              }
              
              
              if (to_page >= 0 && to_page <= 9998){
                load(gr.current_file,to_page);
              }
              rerender();
            }
            
            if (abs(y) > abs(x)*2 && abs(y) > this->h/8){
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

        load();


        sql_geti(gr.read_d,"sis",width,":",0,"pen_width");
        sql_geti(gr.read_d,"sis",eraser_width,":",0,"eraser_width");
        sql_geti(gr.read_d,"sis",select_width,":",0,"select_width");
        sql_geti(gr.read_d,"sis",pen_type,":",0,"pen_type");
    }

    

    
    
    
    stroke prev_stroke;

    bool merge_if_inline(stroke& a, stroke& b){
      if (a.bx == a.ax && a.by == a.ay) {
        b = stroke{a.ax,a.ay,b.bx,b.by,a.width,a.color,a.type,a.etc};
        return false;
      }
      int dx =  abs(a.bx-a.ax);
      int sx = a.ax<a.bx ? 1 : -1;
      int dy = -abs(a.by-a.ay);
      int sy = a.ay<a.by ? 1 : -1;
      int err = dx+dy;
      int x = a.ax;
      int y = a.ay;

      int olen = 1000000000;
      int ox = -1;
      int oy = -1;
      while (true) {
        int l = lensq(x-b.bx,y-b.by);
        if (l < olen){
          olen = l;
          ox = x;
          oy = y;
        } else 
          break;
        int e2 = 2*err;
        if (e2 >= dy) {
          err+= dy;
          x += sx;
        }
        if (e2 <= dx){
          err += dx;
          y += sy;
        }
      }

      if (olen <= 0) {
        b = stroke{a.ax,a.ay,ox,oy,a.width,a.color,a.type,a.etc};
        return false;
      }
      return true;
    }
    
    void add_stroke(input::SynMotionEvent& e){
      char etc = 0;
      switch (pen_type) {
        //Eventually there may be pen specific data to set
      }
      
      if (px >= 0){
        stroke t = stroke{px,py,e.x,e.y+gr.y_scroll-y,width*2,0,pen_type,etc};


        t.draw(fb,gr.y_scroll,y);
        if (merge_if_inline(prev_stroke, t)){
          gr.add(prev_stroke);
        }
        
        prev_stroke = t;
        px = t.bx;
        py = t.by;
      }
      else {
        prev_stroke = stroke{e.x,e.y+gr.y_scroll-y,e.x,e.y+gr.y_scroll-y,width*2,0,pen_type,etc};
        prev_stroke.draw(fb,gr.y_scroll,y);
        //gr.add(prev_stroke);
        px = prev_stroke.bx;
        py = prev_stroke.by;
      }
    }
    int state = 0;
    std::string sel_name;
    file_link* prev_link;

    remarkable_color *buf_sel = nullptr, *buf_back = nullptr;

    void set_sel_bounds() {
      int xmax = 0, ymax = 0;
      
      if (!selection.size()) {sel_w = 0;return;}

      for (stroke& s : selection) {
        if (!sel_w) {
          sel_x = min(s.ax-s.width/2,s.bx-s.width/2);
          sel_y = min(s.ay-s.width/2,s.by-s.width/2);
          xmax = max(s.ax+s.width/2,s.bx+s.width/2);
          ymax = max(s.ay+s.width/2,s.by+s.width/2);
          sel_w = 1;
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

      fb->draw_rect(sel_x, sel_y-gr.y_scroll+y, sel_w, sel_h, WHITE, true);
      for (stroke& s : selection)
        s.draw(fb,gr.y_scroll,y,sel_x,sel_y);
      fb->draw_rect(sel_x, sel_y-gr.y_scroll+y, sel_w, sel_h, BLACK, false);
      buf_sel = get_fb_area(fb,sel_x,sel_y-gr.y_scroll+y,sel_w,sel_h);

      fb->draw_rect(sel_x, sel_y-gr.y_scroll+y, sel_w, sel_h, WHITE, true);
      if (lines) {
        int y_s = ((sel_y-1) / lines + 1) * lines;
        for (int i = y_s ; i < sel_y + sel_h; i+=lines)
          fb->draw_line(sel_x,i-gr.y_scroll+y,sel_x+sel_w,i-gr.y_scroll+y,1,color::SCALE_16[8]);
      }
      gr.redraw(sel_x,sel_y,sel_w,sel_h);
      buf_back = get_fb_area(fb,sel_x,sel_y-gr.y_scroll+y,sel_w,sel_h);
      
    }
    void draw_sel() {
      if (sel_w){
        get_fb_area(fb,buf_back,sel_x,sel_y-gr.y_scroll+y,sel_w,sel_h);
        set_fb_area(fb,buf_sel,sel_x,sel_y-gr.y_scroll+y,sel_w,sel_h,WHITE);
        dirty = 0;
      }
    }
    void undraw_sel() {
      if (sel_w) {
        set_fb_area(fb,buf_back,sel_x,sel_y-gr.y_scroll+y,sel_w,sel_h);
      }
      // fb->draw_rect(sel_x, sel_y-gr.y_scroll+y, sel_w, sel_h, WHITE, true);
      // if (lines) {
        // int y_s = ((sel_y-1) / lines + 1) * lines;
        // for (int i = y_s ; i < sel_y + sel_h; i+=lines)
          // fb->draw_line(sel_x,i-gr.y_scroll+y,sel_x+sel_w,i-gr.y_scroll+y,1,color::SCALE_16[8]);
      // }
      // gr.redraw(sel_x,sel_y,sel_w,sel_h);
    }

    void unselect() {
      if (sel_w) {
        
        delete buf_sel;
        delete buf_back;
        buf_sel = buf_back = nullptr;
        
        for (stroke& s : selection) {
          s.ax += sel_x;
          s.bx += sel_x;
          s.ay += sel_y;
          s.by += sel_y;
          gr.add(s);
        }
        sel_w = 0;
        selection.clear();
        dirty = 1;   
      }
    }

    void start_stroke(int tool,input::SynMotionEvent& e) {
      px = py = -1;
      state = tool;
      file_link* l;
      switch (state) {
        case SELECT:
          if (sel_w && e.x < sel_x+sel_w && e.x >= sel_x && e.y+gr.y_scroll-y < sel_y+sel_h && e.y+gr.y_scroll-y >= sel_y){
            state = MOVE_SEL;
            break;
          }
          if (clipboard_full) {
            if (e.y > y+h-link_size-5){
              if (e.x < w/2){
                dirty = 1;
                if (sel_w){
                  unselect();
                  rerender();
                }
                selection = clipboard;
                set_sel_bounds();
                sel_x = e.x - sel_w/2;
                sel_y = e.y+gr.y_scroll-y - sel_h/2;
                draw_sel();
                state = MOVE_SEL;
                break;
              } else {
                dirty = 1;
                clipboard_full = false;                
                clipboard.clear();

                if (sel_w){
                  unselect();
                  rerender();
                }
                state = NUL_ST;
                break;
              }
            }
          }
          if (sel_w){
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

            fb->draw_rect(sel_x, sel_y-gr.y_scroll+y, sel_w, sel_h, WHITE, true);
            gr.redraw(sel_x,sel_y,sel_w,sel_h);
            buf_back = get_fb_area(fb,sel_x,sel_y-gr.y_scroll+y,sel_w,sel_h);
            
            break;
          }
          unselect();
          break;
        case ERASER:
          if ((prev_link = gr.get_link(e.x,e.y+gr.y_scroll-y))) {
            state = REM_LINK;
          }
        default:
          if (sel_w) {
            unselect();
            rerender();
          }
          break;
      }
    }

    void update_stroke(input::SynMotionEvent& e){
      switch (state) {
        case DRAW:
          if (px < 0 || lensq(e.x-px,e.y-py) > 1){
            add_stroke(e);
          }
          break;
        case ERASER:
          px = e.x;
          py = e.y;
          gr.remove(e.x,e.y+gr.y_scroll-y,eraser_width*8);
          break;
        case SELECT:
          gr.cut(e.x,e.y+gr.y_scroll-y,select_width*8,selection);
          break;
        case MOVE_SEL:
          if (px < 0 || lensq(e.x-px,e.y-py) > 4){
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
          if (px < 0 || lensq(e.x-px,e.y-py) > 4){
            if (px >= 0) {
              undraw_sel();            
              sel_x += e.x-px;
              sel_y += e.y-py;
              get_fb_area(fb,buf_back,sel_x,sel_y-gr.y_scroll+y,sel_w,sel_h);
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
          gr.add(prev_stroke);
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
          if (py > y+h-link_size-5) {
            clipboard = selection;
            clipboard_full = true;
            dirty = 1;
            selection.clear();
            unselect();
            rerender();
            
          }
          state = NUL_ST;
          break;
        case LINK_SEL:
          gr.add_link(sel_x,sel_y,sel_name);
          sel_name = "";
          state = NUL_ST;
          sel_x = -1;
          rerender();
          delete buf_back;
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
          if (prev_link == gr.get_link(px,py+gr.y_scroll-y)) {
            gr.remove_link(px,py+gr.y_scroll-y);
            dirty = 1;
          }
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

    

    void load_link(file_link* l) {

      switch (l->file[0]) {
        case '@':
          //load a supplementary file like a .md or .pdf at some point
          break;
      }
      
      std::size_t i = l->file.find(":");

      std::string file = gr.current_file;
      int page = 0;
      if (i != std::string::npos){
        if (i > 0)
          file = l->file.substr(0,i);
        if (i+1 < l->file.length())
          page = std::stoi(l->file.substr(i+1))-1;
      }else {
        file = l->file;
        sql_geti(gr.read_d,"sis",page,file.c_str(),-1,"last_page");
      }
      
      
      load(file,page);
    }
    int link_x,link_y;
    ui::Keyboard kb;
    void on_mouse_click(input::SynMotionEvent& e){
      if (input::is_touch_event(e)){
        file_link* l = gr.get_link(e.x,e.y+gr.y_scroll-y);
        if (l){
          load_link(l);
          rerender();
        }
      }
    }
    
    void on_mouse_move(input::SynMotionEvent& e){
        if (input::is_wacom_event(e)){ 
          if (!((e.left && e.left!=-1) || (e.eraser && e.eraser!=-1))) {
            if (e.right && e.right!=-1) {
              if (is_started) {
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
       if (input::is_touch_event(e) && px!=-1) {
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

        draw_sel();

        if (clipboard_full) {
          fb->draw_rect(0,y+h-link_size,w,1,BLACK,true);
          fb->draw_rect(0,y+h-link_size,w,link_size,WHITE,true);
          fb->draw_text(w/4-20,y+h-link_size,"[Paste]",link_size);
          fb->draw_text(w/2+w/4+20,y+h-link_size,"[Clear]",link_size);
        }
        
        fb->dirty = 1;
        fb->waveform_mode = WAVEFORM_MODE_GC16;
        int marker = fb->perform_redraw(true);
        fb->wait_for_redraw(marker);
        dirty = 0;
    }
};




ui::Button* tool_button(NoteBook* N,ui::RangeInput* range,int id,const char* ch, icons::Icon icon) {
  ui::Button* b = new ui::Button(id*32,0,32,tool_height,ch);
  b->icon = icon;
  b->text = "";

  int* var = &N->width;

  switch (id) {
    case ERASER: var = &N->eraser_width; break;
    case SELECT: var = &N->select_width; break;
  }
  
  b->mouse.click += [N, id, range, var] (input::SynMotionEvent&){
    N->fb->draw_rect(N->tool*32,tool_height,32,4,WHITE,true);
    N->fb->draw_rect(id*32,tool_height,32,4,BLACK,true);
    N->tool = id;
    range->set_value(*var);
    range->dirty = 1;
  }; 

  
  return b;
}


struct SettingsStuff {
  bool running = true;
  ui::Scene scene;
  ui::Button* rtl;
  ui::RangeInput* lines;
  SettingsStuff(NoteBook* N, ui::Scene main_scene,int w,int h) {
    scene = ui::make_scene();

    {
      ui::Button* b = new ui::Button(1,0,256,tool_height,"<--");
      b->mouse.click += [=] (input::SynMotionEvent&) {
        
        ui::MainLoop::set_scene(main_scene);
        N->fb->clear_screen();
        N->refresh_screen();
      }; 
      scene->add(b);
    }
    {
      ui::Button* b = new ui::Button(0,tool_height*1,256,tool_height,N->rtl?"RTL":"LTR");
      b->mouse.click += [=] (input::SynMotionEvent&) {
        N->rtl = !N->rtl;
        sql_run(N->gr.write_d,"sisi",N->gr.current_file.c_str(),-1,"RTL",N->rtl);
        b->text = N->rtl?"RTL":"LTR";
        b->dirty = 1;
      }; 
      rtl = b;
      scene->add(b);
    }
    {
      ui::RangeInput* range = new ui::RangeInput(w-256+85, tool_height*1, 123, tool_height);
      range->set_range(0, 15);
      range->set_value(N->lines/10);
      range->events.change += [=] (float){
        N->lines = range->get_value()*10;
        sql_run(N->gr.write_d,"sisi",N->gr.current_file.c_str(),N->gr.current_page,"vert_lines",N->lines);
      };
      lines = range;
      scene->add( range );


      ui::Button* b = new ui::Button(w-128+85,tool_height*1,43,tool_height,"Def");
      b->mouse.click += [=] (input::SynMotionEvent&) {
        sql_run(N->gr.del_d,"sis",N->gr.current_file.c_str(),N->gr.current_page,"vert_lines");
        if (!sql_geti(N->gr.read_d,"sis",N->lines,N->gr.current_file.c_str(),0,"vert_lines")){
          sql_run(N->gr.write_d,"sisi",N->gr.current_file.c_str(),0,"vert_lines",N->lines);
        }
        
        range->set_value(N->lines/10);
      }; 
      scene->add(b);

      ui::Text* text = new ui::Text(w-256,tool_height*1+10,80,tool_height,"Rule");
      scene->add(text);
    }
    {
      ui::Button* b = new ui::Button(w-256,tool_height*2,128,tool_height,"Clear");
      b->mouse.click += [=] (input::SynMotionEvent&) {
        N->gr.clear();
        N->dirty = 1;    
      }; 
      scene->add(b);
    }
    {
      ui::Button* b = new ui::Button(w-256,tool_height*3,128,tool_height,"Delete");
      b->mouse.click += [=] (input::SynMotionEvent&) {
        N->gr.save();
        N->gr.del(N->gr.current_file,N->gr.current_page);
        N->load(N->gr.current_file,N->gr.current_page);
      }; 
      scene->add(b);
    }
    
    {
      ui::Button* b = new ui::Button(w-128,h-tool_height,128,tool_height,"Exit");
      b->mouse.click += [=] (input::SynMotionEvent&) {
        N->gr.close();
        running = false;
      };
      scene->add(b);
    }
    
  }

  ui::Button* settings_button(int x,int y,int w,int h,NoteBook* N) {
    ui::Button* b = new ui::Button(x,y,w,h,"Settings");
    b->mouse.click += [=] (input::SynMotionEvent&){

      rtl->text = N->rtl?"RTL":"LTR";
      lines->set_value(N->lines/10);

      ui::MainLoop::set_scene(scene);
      N->fb->clear_screen();
      N->refresh_screen();
      
    };
    b->text = "";
    b->icon = ICON(assets::fa_Cog_png);
    return b;
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


    
    NoteBook* N = new NoteBook(w,h-tool_height,tool_height);
    scene->add(N);
    scene->add(N->pagenum);

    
   
    {
      ui::Button* b = new ui::Button(160,0,32,tool_height,"Cut");
      b->mouse.click += [=] (input::SynMotionEvent&){
        N->gr.save();
        N->gr.move(N->gr.current_file,N->gr.current_page,"Copies",0);
        N->load(N->gr.current_file,N->gr.current_page);
        N->rerender();
      }; 
      b->text = "";
      b->icon = ICON(assets::fa_Cut_png);
      scene->add(b);
    }
    
    {
      ui::Button* b = new ui::Button(192,0,32,tool_height,"Paste");
      b->mouse.click += [=] (input::SynMotionEvent&){
        N->gr.save();
        N->gr.move("Copies",0,N->gr.current_file,N->gr.current_page);
        N->load(N->gr.current_file,N->gr.current_page);
        N->rerender();
      };
      b->text = "";
      b->icon = ICON(assets::fa_Paste_png); 
      scene->add(b);
    }
    

    {
      ui::RangeInput* range = new ui::RangeInput(240, 0, 128, tool_height);
      
      range->set_range(1, 10);
      range->set_value(N->width);
      range->events.change += [=] (float){
        switch (N->tool){
          case DRAW:{
            N->width = range->get_value();
            sql_run(N->gr.write_d,"sisi",":",0,"pen_width",N->width);
          }break;
          case ERASER:{
            N->eraser_width = range->get_value();
            sql_run(N->gr.write_d,"sisi",":",0,"eraser_width",N->eraser_width);
          }break;
          case SELECT:{
            N->select_width = range->get_value();
            sql_run(N->gr.write_d,"sisi",":",0,"select_width",N->select_width);
          }break;
        }
      };
      scene->add( range );

      scene->add(tool_button(N,range,DRAW,"Pen",ICON(assets::fa_Pen_png)));
      scene->add(tool_button(N,range,ERASER,"Erase",ICON(assets::fa_Erase_png)));
      scene->add(tool_button(N,range,SELECT,"Select",ICON(assets::fa_Select_png)));
      scene->add(tool_button(N,range,LINK,"Link",ICON(assets::fa_Link_png)));
    }

    SettingsStuff settings(N,scene,w,h);
    scene->add( settings.settings_button(w-32,0,32,tool_height,N));

    {
      ui::Button* b = new ui::Button(w-64,0,32,tool_height,"Home");
      b->mouse.click += [=] (input::SynMotionEvent&){
        N->load();
        N->rerender();
      };
      b->text = "";
      b->icon = ICON(assets::fa_Home_png); 
      scene->add(b);
    }

    {
      ui::TextDropdown* drop = new ui::TextDropdown(368, 0, 128, tool_height, pen_names[N->pen_type]);
      drop->dir = ui::TextDropdown::DIRECTION::DOWN;
      auto sect = drop->add_section( "Pens" );
      for (int i = 0 ; i < NUM_PEN; i++)
        sect->add_options(std::vector<std::string>{pen_names[i]});
      drop->events.selected += [=](int id) {
        N->pen_type = id;
        sql_run(N->gr.write_d,"sisi",":",0,"pen_type",id);
      };
      scene->add(drop);
    }
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
              ui::MainLoop::set_scene(sleep_scene);
              N->refresh_screen();
              N->gr.close();
            } else {
              N->fb->clear_screen();
              ui::MainLoop::set_scene(scene);
              
              N->gr.open();
              N->load(N->gr.current_file,N->gr.current_page);

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
    while (settings.running){
        ui::MainLoop::main();
        ui::MainLoop::redraw();
        ui::MainLoop::read_input();
    }
}
