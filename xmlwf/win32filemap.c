/*
The contents of this file are subject to the Mozilla Public License
Version 1.0 (the "License"); you may not use this file except in
compliance with the License. You may obtain a copy of the License at
http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS"
basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
License for the specific language governing rights and limitations
under the License.

The Original Code is expat.

The Initial Developer of the Original Code is James Clark.
Portions created by James Clark are Copyright (C) 1998
James Clark. All Rights Reserved.

Contributor(s):
*/

#define STRICT 1
#include <windows.h>
#include <stdio.h>
#include "filemap.h"

static void win32perror(const char *);

int filemap(const char *name,
	    void (*processor)(const void *, size_t, const char *, void *arg),
	    void *arg)
{
  HANDLE f;
  HANDLE m;
  DWORD size;
  DWORD sizeHi;
  void *p;

  f = CreateFile(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
			  FILE_FLAG_SEQUENTIAL_SCAN, NULL);
  if (f == INVALID_HANDLE_VALUE) {
    win32perror(name);
    return 0;
  }
  size = GetFileSize(f, &sizeHi);
  if (size == (DWORD)-1) {
    win32perror(name);
    return 0;
  }
  if (sizeHi) {
    fprintf(stderr, "%s: bigger than 2Gb\n", name);
    return 0;
  }
  /* CreateFileMapping barfs on zero length files */
  if (size == 0) {
    static const char c = '\0';
    processor(&c, 0, name, arg);
    CloseHandle(f);
    return 1;
  }
  m = CreateFileMapping(f, NULL, PAGE_READONLY, 0, 0, NULL);
  if (m == NULL) {
    win32perror(name);
    CloseHandle(f);
    return 0;
  }
  p = MapViewOfFile(m, FILE_MAP_READ, 0, 0, 0);
  if (p == NULL) {
    win32perror(name);
    CloseHandle(m);
    CloseHandle(f);
    return 0;
  }
  processor(p, size, name, arg); 
  UnmapViewOfFile(p);
  CloseHandle(m);
  CloseHandle(f);
  return 1;
}

static
void win32perror(const char *s)
{
  LPVOID buf;
  if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		    NULL,
		    GetLastError(),
		    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		    (LPTSTR) &buf,
		    0,
		    NULL)) {
    fprintf(stderr, "%s: %s", s, buf);
    fflush(stderr);
    LocalFree(buf);
  }
  else
    fprintf(stderr, "%s: unknown Windows error\n", s);
}
