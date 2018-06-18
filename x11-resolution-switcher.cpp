#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char running = 0;

int catcher( Display *disp, XErrorEvent *xe ){
  printf("Window gone away\n");
  return 0;
}

int set_resolution(Display *dpy, Window root, int width, int height) {

  int nsize;
  int size = -1;
  double rate = -1;

  XRRScreenConfiguration *sc = XRRGetScreenInfo (dpy, root);
  if(!sc){
    fprintf(stderr, "Failed to get current screen config");
    return 1;
  }
  Rotation current_rotation;
  SizeID current_size = XRRConfigCurrentConfiguration (sc, &current_rotation);
  XRRScreenSize *sizes = XRRConfigSizes(sc, &nsize);
  if(width && height){
    for (size = 0; size < nsize; size++){
      if (sizes[size].width == width && sizes[size].height == height)
        break;
    }
    if (size >= nsize){
      fprintf (stderr, "Size %dx%d not found in available modes\n", width, height);
      return 1;
    }
  }
  else
    size = 0;
  short current_rate = XRRConfigCurrentRate (sc);

  XSelectInput (dpy, root, StructureNotifyMask);
  XRRSelectInput (dpy, root, RRScreenChangeNotifyMask);
  if((XRRSetScreenConfigAndRate (dpy, sc, root, (SizeID) size, 1, current_rate, CurrentTime) == RRSetConfigFailed)){
    fprintf (stderr, "Failed to change the screen configuration!\n");
    return 1;
  }
  XRRFreeScreenConfigInfo(sc);
   
}

int set_default_resolution(Display *dpy, Window root) {
  return set_resolution(dpy, root, 0, 0);
}

main(int argc, char** argv){

  char daemonize = 0;
  for(int i = 1; i < argc; i++){
    if(!strcmp(argv[i], "-d") || !strcmp(argv[i], "--daemon"))
      daemonize = 1;
  }

  if(daemonize)
    daemon(0,0);
  char res_changed = 0;
  Atom real;
  int format;
  unsigned long extra;
  unsigned long n;
  unsigned char* data;
  XEvent e;
  Window root;

  Status status;
  Window curr_window;

  Display *d = XOpenDisplay(0);
  if(!d) {
    printf("couldn't get default display\n");
    exit(1);
  }

  running =1;

  XSetErrorHandler( catcher );
  root = DefaultRootWindow(d);
  while (running){
    try{
      XSelectInput(d, root, PropertyChangeMask);
      XNextEvent( d, &e );
      if (e.type == PropertyNotify ) {
        if (!strcmp( XGetAtomName( d, e.xproperty.atom ), "_NET_ACTIVE_WINDOW" ) ) {
          XGetWindowProperty(d, root, e.xproperty.atom, 0, ~0, false, AnyPropertyType, &real, &format, &n, &extra, &data);
          curr_window = (Window)*(unsigned long *)data;
          XClassHint hint;
          XGetClassHint(d, curr_window, &hint);
          printf("Current window hint name: %s\n", hint.res_name);
          if(!strcmp(hint.res_name, "streaming_client") && !res_changed){
            printf("Changing resolution\n");
            set_resolution(d, root, 1920, 1080);
            res_changed = 1;
          }
          else if(res_changed){
            printf("Restoring resolution\n");
            set_default_resolution(d, root);
            res_changed = 0;
          }
        }
      }
    }
    catch(...){
      printf("Ignoring error\n");
    }
  }

  return 0;
}
