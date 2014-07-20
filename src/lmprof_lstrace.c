/*
** Author: Pablo Musa
** Creation Date: jul 20 2014
** Last Modification: ago 15 2014
** See Copyright Notice in COPYRIGHT
**
*/

#include <stdlib.h>
#include <stdio.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

static int findfield (lua_State *L, int objidx, int level) {
  if (level == 0 || !lua_istable(L, -1))
    return 0;  /* not found */
  lua_pushnil(L);  /* start 'next' loop */
  while (lua_next(L, -2)) {  /* for each pair in table */
    if (lua_type(L, -2) == LUA_TSTRING) {  /* ignore non-string keys */
      if (lua_rawequal(L, objidx, -1)) {  /* found object? */
        lua_pop(L, 1);  /* remove value (but keep name) */
        return 1;
      }
      else if (findfield(L, objidx, level - 1)) {  /* try recursively */
        lua_remove(L, -2);  /* remove table (but keep name) */
        lua_pushliteral(L, ".");
        lua_insert(L, -2);  /* place '.' between the two names */
        lua_concat(L, 3);
        return 1;
      }
    }
    lua_pop(L, 1);  /* remove value */
  }
  return 0;  /* not found */
}

static int pushglobalfuncname (lua_State *L, lua_Debug *ar) {
  int top = lua_gettop(L);
  lua_getinfo(L, "f", ar);  /* push function */
  lua_pushglobaltable(L);
  if (findfield(L, top + 1, 2)) {
    lua_copy(L, -1, top + 1);  /* move name to proper place */
    lua_pop(L, 2);  /* remove pushed values */
    return 1;
  }
  else {
    lua_settop(L, top);  /* remove function and global table */
    return 0;
  }
}

static void pushfuncname (lua_State *L, lua_Debug *ar) {
  if (*ar->namewhat != '\0')  /* is there a name? */
    lua_pushfstring(L, "function " LUA_QS, ar->name);
  else if (*ar->what == 'm')  /* main? */
      lua_pushliteral(L, "main chunk");
  else if (*ar->what == 'C') {
    if (pushglobalfuncname(L, ar)) {
      lua_pushfstring(L, "function " LUA_QS, lua_tostring(L, -1));
      lua_remove(L, -2);  /* remove name */
    }
    else
      lua_pushliteral(L, "?");
  }
  else
    lua_pushfstring(L, "function <%s:%d>", ar->short_src, ar->linedefined);
}

const char* lmprof_lstrace_getfuncinfo(lua_State *L, lua_Debug *ar) {
  const char *funcinfo;
  int top = lua_gettop(L);
  lua_getinfo(L, "Slnt", ar);
  lua_pushfstring(L, "\n\t%s:", ar->short_src);
  if (ar->currentline > 0)
    lua_pushfstring(L, "%d:", ar->currentline);
  lua_pushliteral(L, " in ");
  pushfuncname(L, ar);
  if (ar->istailcall) 
    lua_pushliteral(L, "\n\t(...tail calls...)");
  lua_concat(L, lua_gettop(L) - top);
  funcinfo = lua_tostring(L, -1); /* do not pop the string - only after use */
  return funcinfo;
}

/*
static const char* get_lstrace (lua_State *L) {
  lua_Debug ar;
  int level = 1;
  const char *lstrace;
  lua_State *LT = luaL_newstate();
  int top = lua_gettop(LT);

  lua_pushliteral(LT, "FULL stack traceback:");
  while (lua_getstack(L, level++, &ar)) {
      lua_getinfo(L, "Slnt", &ar);
      lua_pushfstring(LT, "\n\t%s:", ar.short_src);
      if (ar.currentline > 0)
        lua_pushfstring(LT, "%d:", ar.currentline);
      lua_pushliteral(LT, " in ");
      pushfuncname(LT, &ar);
      if (ar.istailcall)
        lua_pushliteral(LT, "\n\t(...tail calls...)");
      lua_concat(LT, lua_gettop(LT) - top);
  }
  lua_concat(LT, lua_gettop(LT) - top);
  * copy to original state, otherwise it will be lost *
  lua_pushstring(L, lua_tostring(LT, -1));
  lua_close(LT);
  lstrace = lua_tostring(L, -1);

  return lstrace;
}
*/

int lmprof_lstrace_write (lua_State *L, const char *filename) {
  lua_Debug ar;
  int level = 1;
  FILE *f = fopen(filename, "w");
  if (f == NULL) {
    return 0;
  }

  fprintf(f, "%s", "FULL stack traceback:");

  while (lua_getstack(L, level++, &ar)) {
    fprintf(f, "%s", lmprof_lstrace_getfuncinfo(L, &ar));
    lua_pop(L, 1); /* pop string after use */
  }

  fclose(f);

  return 1;
}

