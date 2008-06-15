# MyServer
# Copyright (C) 2002, 2003, 2004, 2007, 2008 The MyServer Team
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


import sys
import gobject
try:
    import pygtk
    pygtk.require("2.0")
except:
    pass
try:
    import gtk
    import gtk.glade
except:
    sys.exit(1)

class ConfigGUIGTK:
    """@package ConfigGUIGTK
    UI for configurtion tool"""

    def __init__(self):
        """Default constructor for GUI. No params"""

        # Connect the Glade file
        self.gladefile = "XMLGui.glade"
        self.wTree = gtk.glade.XML(self.gladefile)
        
        # Bind main window
        self.window = self.wTree.get_widget("ConfigUI")
        # Bind "destroy" event
        if (self.window):
            self.window.connect("destroy", gtk.main_quit)

        # Create the tree view widget structure (one column)
        self.treeview=self.wTree.get_widget("treeview1")
        self.treemodel=gtk.TreeStore(gobject.TYPE_STRING)
        self.treeview.set_model(self.treemodel)
        # Add tree view header
        self.treeview.set_headers_visible(gtk.TRUE)
        renderer=gtk.CellRendererText()
        column=gtk.TreeViewColumn("Default file names:",renderer, text=0)
        column.set_resizable(True)
        self.treeview.append_column(column)
        renderer=gtk.CellRendererText()
        
        # Add list items to tree view
        self.insert_row(self.treemodel,None,'default.html')
        self.insert_row(self.treemodel,None,'default.htm')
        self.insert_row(self.treemodel,None,'default.php')
        self.insert_row(self.treemodel,None,'index.html')
        self.insert_row(self.treemodel,None,'index.htm')
        self.insert_row(self.treemodel,None,'index.php')
        
        self.treeview.show()
    
        # dic - dictionary with pairs:
        #        "signal name" : event
        dic = { "on_btnAddFileName_clicked" : \
                    self.buttonAddefaultFileName_clicked,
                "on_btnRemoveFileName_clicked" : \
                    self.buttonRemoveDefultFileName_clicked
                }
        # Connect signals
        self.wTree.signal_autoconnect (dic)

    def insert_row(self, model,parent,
                   column):
        """Fuction used to add rows to treeview.
        @param model  what model tree view uses for storing data
        @param parent used to nest entries (like dir explorer)
        @param column the text you want to insert
        """
        myiter=model.insert_after(parent,None)
        model.set_value(myiter,0,column)
        return myiter

    def delete_rows(self, treeV):
        """ Function used to remove rows from treeview.
        @param treeV from which treeview we're deleting
        """
        selection = treeV.get_selection()
        model, selected = selection.get_selected()
        model.remove(selected)

    def buttonAddefaultFileName_clicked(self, button):
        """ Adds new entry, with default file name to tree view"""
        dia = gtk.Dialog("Add new file name")
        dia.show()
        entry = gtk.Entry()
        entry.show()
        entry.set_activates_default(gtk.TRUE)
        dia.vbox.pack_start(entry)
        dia.add_button(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL)
        dia.add_button(gtk.STOCK_OK, gtk.RESPONSE_OK)
        dia.set_default_response(gtk.RESPONSE_OK)
        response = dia.run()
        if response == gtk.RESPONSE_OK:
            name = entry.get_text()
        dia.destroy()
        self.insert_row(self.treemodel,None, name)
        
    def buttonRemoveDefultFileName_clicked(self, button):
        """ Removes selected entry """
        self.delete_rows(self.treeview)
        
if __name__ == "__main__":
    ConfigUI = ConfigGUIGTK()
    gtk.main()
