#include "rmkit.h"
#include <tuple>
#include <vector>


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
    char width = 2,pen_type = 0;
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
        dirty = 1;     
      }
    }
    
    void load(std::string file = "Home",int page = 0){
      unselect();
      pagenum->undraw();
      pagenum->text = file+":"+std::to_string(abs(page)+1);
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
            if (abs(x) > abs(y)*2 && abs(x) > this->h/8){
              undraw();
              if (x > 0 && gr.current_page > -998){
                load(gr.current_file,gr.current_page-1);
              }
              if (x < 0 && gr.current_page < 998) {
                load(gr.current_file,gr.current_page+1);
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
        stroke t = stroke{px,py,e.x,e.y+gr.y_scroll-y,width,0,pen_type,etc};


        t.draw(fb,gr.y_scroll,y);
        if (merge_if_inline(prev_stroke, t)){
          gr.add(prev_stroke);
        }
        
        prev_stroke = t;
        px = t.bx;
        py = t.by;
      }
      else {
        prev_stroke = stroke{e.x,e.y+gr.y_scroll-y,e.x,e.y+gr.y_scroll-y,width,0,pen_type,etc};
        prev_stroke.draw(fb,gr.y_scroll,y);
        //gr.add(prev_stroke);
        px = prev_stroke.bx;
        py = prev_stroke.by;
      }
    }
    int state = 0;
    std::string sel_name;
    file_link* prev_link;
    void start_stroke(int tool,input::SynMotionEvent& e) {
      px = py = -1;
      state = tool;
      file_link* l;
      switch (state) {
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
        case ERASER:
          if ((prev_link = gr.get_link(e.x,e.y+gr.y_scroll-y))) {
            state = REM_LINK;
          }
        default:
          if (sel_x >= 0) {
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
          //load a supplementary file like a .md maybe
          break;
      }
      
      std::size_t i = l->file.find(":");

      std::string file;
      int page = 0;
      if (i != std::string::npos){
        file = l->file.substr(0,i);
        if (i+1 < l->file.length())
          page = std::stoi(l->file.substr(i+1))-1;
      }else {
        file = l->file;
        //get last page
      }
      
      
      
      load(i?file:gr.current_file,page);
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

        draw_sel();
        
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
    // scene->add(tool_button(N,REM_LINK,"-"));
   
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

    {
      ui::TextDropdown* drop = new ui::TextDropdown(368,0,128,tool_height,"Round");
      drop->dir = ui::TextDropdown::DIRECTION::DOWN;
      auto sect = drop->add_section("Pens");
      for (int i = 0 ; i < NUM_PEN; i++)
        sect->add_options(std::vector<std::string>{pen_names[i]});
      drop->events.selected += [=](int id){
        N->pen_type = id;
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
