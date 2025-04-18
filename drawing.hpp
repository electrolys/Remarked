

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

inline void my_draw_line_line(framebuffer::FB* fb,int x0,int y0,int x1,int y1,int xdir,int ydir,int color){
      fb->dirty = 1;
      int dx =  abs(x1-x0);
      int sx = x0<x1 ? 1 : -1;
      int dy = -abs(y1-y0);
      int sy = y0<y1 ? 1 : -1;
      int err = dx+dy;
      fb->update_dirty(fb->dirty_area, min(x0,x1)-abs(xdir), min(y0,y1)-abs(ydir));
      fb->update_dirty(fb->dirty_area, max(x0,x1)+abs(xdir), max(y0,y1)+abs(ydir));
      // my_draw_circle_fast(fb,x0, y0, width / 2, color);

      while (true){
        // my_draw_vert_fast(fb,x0, y0-width/2, width, color);
        // my_draw_horiz_fast(fb,x0-width/2, y0, width, color);
        fb->draw_line(x0-xdir,y0-ydir,x0+xdir,y0+ydir,2,color);
        if (x0==x1 && y0==y1) {
          // my_draw_circle_fast(fb,x0, y0, width / 2, color);
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

