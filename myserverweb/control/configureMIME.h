/*
*MyServer
*Copyright (C) 2002 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef CONFIGUREMIME_H
#define CONFIGUREMIME_H
#include "stdafx.h"
#include "resource.h" 
#include <wx/wx.h> 
#include <wx/taskbar.h>
#ifdef WIN32
#include <windows.h>
#endif
#define SOCKETLIBINCLUDED/*Prevent include socket headers file*/
#include "../include/MIME_manager.h"
#include "../include/cXMLParser.h"
          
extern const char VERSION_OF_SOFTWARE[];
#define MIMEWNDSIZE_X	420
#define MIMEWNDSIZE_Y	260

class configurationFrameMIME : public wxFrame
{
public:
	MIME_Manager mm;
	wxButton* btnOK;
	wxButton* btnCNL;
	wxButton* btnSAVE;
	wxButton* btnADDEXT;
	wxButton* btnADDMIME;
	wxListBox *actiontodoLB;
	wxTextCtrl *cgiManagerTB;
	wxListBox *mimeTypesLB;
	wxListBox *extensionsLB;
	configurationFrameMIME(wxWindow *parent,const wxString& title, const wxPoint& pos, const wxSize& size,long style = wxDEFAULT_FRAME_STYLE);
	void OnQuit(wxCommandEvent& event);
	void EXTtypeListEvt(wxCommandEvent& event);
	void cancel(wxCommandEvent& event);
	void ok(wxCommandEvent& event);
	void save(wxCommandEvent& event);
	void addExt(wxCommandEvent& event);
	void addMime(wxCommandEvent& event);
private:
	DECLARE_EVENT_TABLE()
};

#endif
