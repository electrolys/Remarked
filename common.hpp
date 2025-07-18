#pragma once
#include "rmkit.h"
#include "sqlite3.h"

void sql_bind_v(sqlite3_stmt* stmt, const char* args, va_list varg);
void sql_bind(sqlite3_stmt* stmt, const char* args, ...);
void sql_run(sqlite3_stmt* stmt, const char* args, ...);
bool sql_geti(sqlite3_stmt* stmt, const char* args, int& ret, ...);


inline int lensq(int x,int y){return x*x+y*y;}
inline int min(int x,int y){return x<y?x:y;}
inline int max(int x,int y){return x>y?x:y;}

void error_msg(framebuffer::FB* fb, std::string t);



