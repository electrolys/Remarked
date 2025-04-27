

#pragma once
#include "rmkit.h"


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



inline void my_draw_line_circle(framebuffer::FB* fb,int x0,int y0,int x1,int y1,int width,int color){
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
        my_draw_vert_fast(fb,x0, y0-width/2, (width/2)*2+1, color);
        my_draw_horiz_fast(fb,x0-width/2, y0, (width/2)*2+1, color);
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

inline void my_draw_line_vert(framebuffer::FB* fb,int x0,int y0,int x1,int y1,int width,int color){
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

inline void my_draw_half_tri(framebuffer::FB* fb,int x0,int y0,int x1,int y1,int x2,int y2,int color){
  
      int dx =  abs(x1-x0);
      int sx = x0<x1 ? 1 : -1;
      int dy = -abs(y1-y0);
      int sy = y0<y1 ? 1 : -1;
      int err = dx+dy;
      
}

inline void my_draw_tri(framebuffer::FB* fb,int x0,int y0,int x1,int y1,int x2,int y2,int color){
      fb->dirty = 1;

      #define swap(x,y) do {int t = x; x = y; y = t;} while(0)
      if (y0 > y1) {swap(y0,y1);swap(x0,x1);};
      if (y1 > y2) {swap(y1,y2);swap(x1,x2);};
      if (y0 > y2) {swap(y0,y2);swap(x0,x2);};
      #undef swap
      
      int dx =  abs(x2-x0);
      int sx = x0<x2 ? 1 : -1;
      int dy = y0-y2;
      const int sy = 1;
      int err = dx+dy;

      int adx =  abs(x1-x0);
      int asx = x0<x1 ? 1 : -1;
      int ady = y0-y1;
      const int asy = 1;
      int aerr = adx+ady;

      int bdx =  abs(x1-x0);
      int bsx = x0<x1 ? 1 : -1;
      int bdy = y0-y1;
      const int bsy = 1;
      int berr = bdx+bdy;
      
      while (true){

      
      }
}


inline void my_draw_line_horiz(framebuffer::FB* fb,int x0,int y0,int x1,int y1,int width,int color){
      fb->dirty = 1;
      int dx =  abs(x1-x0);
      int sx = x0<x1 ? 1 : -1;
      int dy = -abs(y1-y0);
      int sy = y0<y1 ? 1 : -1;
      int err = dx+dy;
      
      fb->update_dirty(fb->dirty_area, min(x0,x1)-width/2-1, min(y0,y1)-1);
      fb->update_dirty(fb->dirty_area, max(x0,x1)+width/2+1, max(y0,y1)+1);
      
      // fb->draw_circle(x0, y0, width/2, 1, color,true);
      while (true){
        // fb->_draw_rect_fast(x0-width/2, y0, width, 1, color);
        my_draw_horiz_fast(fb,x0-width/2, y0, width, color);
        
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




void get_fb_area(framebuffer::FB* fb,remarkable_color* out,int x,int y,int w,int h) {
  for (int i = 0 ; i < w*h ; i++)
    out[i] = WHITE;
  if (x >= fb->width || y >= fb->height || x+w < 0 || y+h < 0) return;
  for (int i = max(y,0); i < min(y+h,fb->height-1); i++)
    memcpy(&out[(i-y)*w-min(0,x)], &fb->fbmem[i*fb->width +max(x,0)] , min(min(w,x+w),fb->width-x)*sizeof(remarkable_color));
}

remarkable_color* get_fb_area(framebuffer::FB* fb,int x,int y,int w,int h) {
  remarkable_color* t = new remarkable_color[w*h];
  get_fb_area(fb,t,x,y,w,h);
  return t;
}


void set_fb_area(framebuffer::FB* fb, remarkable_color* buf,int x,int y,int w,int h) {
  if (x >= fb->width || y >= fb->height || x+w < 0 || y+h < 0) return;
  for (int i = max(y,0); i < min(y+h,fb->height-1); i++)
    memcpy(&fb->fbmem[i*fb->width +max(x,0)],&buf[(i-y)*w-min(0,x)] , min(min(w,x+w),fb->width-x)*sizeof(remarkable_color));
  fb->update_dirty(fb->dirty_area,max(x,0),max(y,0));
  fb->update_dirty(fb->dirty_area,min(x+w,fb->width),min(y+h,fb->height));
  fb->dirty = 1;
}

