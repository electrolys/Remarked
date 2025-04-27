#pragma once
#include "rmkit.h"
#include "sqlite3.h"

static void sql_bind_v(sqlite3_stmt* stmt, const char* args, va_list varg) {
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

static void sql_bind(sqlite3_stmt* stmt, const char* args, ...) {
  va_list varg;
  va_start(varg,args);
  sql_bind_v(stmt,args,varg);
  va_end(varg);
}


static void sql_run(sqlite3_stmt* stmt, const char* args, ...) {
  va_list varg;
  va_start(varg,args);
  sql_bind_v(stmt,args,varg);
  va_end(varg);
  while (sqlite3_step(stmt) != SQLITE_DONE){}//TODO error handling
}

static bool sql_geti(sqlite3_stmt* stmt, const char* args, int& ret, ...) {
  va_list varg;
  va_start(varg,ret);
  sql_bind_v(stmt,args,varg);
  va_end(varg);
  
  int t;
  while ((t =sqlite3_step(stmt)) != SQLITE_DONE){
    if (t == SQLITE_BUSY) continue;
    if (t == SQLITE_ROW) {
      ret = sqlite3_column_int(stmt,0);  
      while (sqlite3_step(stmt) != SQLITE_DONE){}
      return true;
    }
  }//TODO error handling

  
  return false;
}


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



