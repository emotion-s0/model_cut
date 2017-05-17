/* not mine. thanks Zach Gorman.<gormanjz@hotmail.com>
 * ---------------------------------------------------------------------------------------------------
 * We will not maintain the old version, 
 * in order to be simple. Only on the WDM driver model of the serial device
 * see c++ source code about http://www.codeguru.com/code/legacy/system/SerialEnum.zip - moecmks
 * ---------------------------------------------------------------------------------------------------
 */

#ifndef _cOmM_eNuM_H_
#define _cOmM_eNuM_H_

#ifdef _MSC_VER 
#include "stdbool.h"
#else 
#include <stdbool.h> /* bool include  **/
#endif 
#include <wchar.h> /* wchar_t include */

#if defined (__cplusplus)
extern "C" {
#endif 

struct secominfo_ansi {
  char *device_path;
  char *friendly_name;
  char *port_desc;
  bool usb_device;
  int comm_pos;
  struct secominfo_ansi *level;
};

struct secominfo_unicode {
  wchar_t *device_path;
  wchar_t *friendly_name;
  wchar_t *port_desc;
  bool usb_device;
  int comm_pos;
  struct secominfo_unicode *level; 
};

struct secominfo_section_ansi {
  int num;
  struct secominfo_ansi *level;
};

struct secominfo_section_unicode {
  int num;
  struct secominfo_unicode *level;
};

int alloc_secominfo_ansi (struct secominfo_section_ansi **sec, bool *complete);
int alloc_secominfo_unicode (struct secominfo_section_unicode **sec, bool *complete);
void dealloc_secominfo_ansi (struct secominfo_section_ansi *sec);
void dealloc_secominfo_unicode (struct secominfo_section_unicode *sec);

#if defined (__cplusplus)
}
#endif 

#endif 