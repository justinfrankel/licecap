/* Cockos SWELL (Simple/Small Win32 Emulation Layer for Linux
   Copyright (C) 2006-2008, Cockos, Inc.

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
  

    This file provides basic APIs for browsing for files, directories, and messageboxes.

    These APIs don't all match the Windows equivelents, but are close enough to make it not too much trouble.

 
    (GTK version)
  */


#ifndef SWELL_PROVIDED_BY_APP

#include <gtk/gtk.h>

#include "swell.h"

void BrowseFile_SetTemplate(int dlgid, DLGPROC dlgProc, struct SWELL_DialogResourceIndex *reshead)
{
}

// return true
bool BrowseForSaveFile(const char *text, const char *initialdir, const char *initialfile, const char *extlist,
                       char *fn, int fnsize)
{
  return false;
}

bool BrowseForDirectory(const char *text, const char *initialdir, char *fn, int fnsize)
{
  return false;
}



char *BrowseForFiles(const char *text, const char *initialdir, 
                     const char *initialfile, bool allowmul, const char *extlist)
{
  return NULL;
}


static void messagebox_callback( GtkWidget *widget, gpointer   data )
{
  if (data) *(char*)data=1;
  gtk_main_quit ();
}

int MessageBox(HWND hwndParent, const char *text, const char *caption, int type)
{
  char has_clicks[3]={0,};
  int ids[3]={0,};
  const char *lbls[3]={0,};
  if (type == MB_OKCANCEL)
  {
    ids[0]=IDOK; lbls[0]="OK";
    ids[1]=IDCANCEL; lbls[1]="Cancel";
  }
  else if (type == MB_YESNO)
  {
    ids[0]=IDYES; lbls[0]="Yes";
    ids[1]=IDNO; lbls[1]="No";
  }
  else if (type == MB_YESNOCANCEL)
  {
    ids[0]=IDYES; lbls[0]="Yes";
    ids[1]=IDNO; lbls[1]="No";
    ids[2]=IDCANCEL; lbls[2]="Cancel";
  }
  else // MB_OK?
  {
    ids[0]=IDOK; lbls[0]="OK";
  }	

  GtkWidget *window;
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_modal(GTK_WINDOW(window), true);
  gtk_window_set_destroy_with_parent(GTK_WINDOW(window), true);
  gtk_window_set_resizable(GTK_WINDOW(window), false);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_title(GTK_WINDOW(window), caption);
  g_signal_connect (G_OBJECT (window), "destroy",
		      G_CALLBACK (messagebox_callback), NULL);

  gtk_container_set_border_width (GTK_CONTAINER (window), 10);
  
  GtkWidget *outer_container = gtk_vbox_new(false, 4);
  {
    GtkWidget *label = gtk_label_new(text);
    gtk_container_add(GTK_CONTAINER(outer_container),label);
    gtk_widget_show(label);
  }
  {
    GtkWidget *con = gtk_hbutton_box_new();
    int x;
    for(x=0;x<3&&ids[x];x++)
    {
      GtkWidget *but = gtk_button_new_with_label(lbls[x]);
      g_signal_connect(G_OBJECT(but), "clicked", G_CALLBACK(messagebox_callback), &has_clicks[x]);
      gtk_container_add(GTK_CONTAINER(con), but);
      gtk_widget_show(but);
    }
    gtk_container_add(GTK_CONTAINER(outer_container),con);
    gtk_widget_show(con);
  }
 
  
  gtk_container_add(GTK_CONTAINER(window), outer_container);
  gtk_widget_show(outer_container);
  gtk_widget_show(window);
  
  gtk_main ();
  
  int x;
  for(x=0;x<3 && ids[x]; x++)
  {
    if (has_clicks[x]) return ids[x];
  }
  if (x>0) x--;
  return ids[x];
}

#endif
