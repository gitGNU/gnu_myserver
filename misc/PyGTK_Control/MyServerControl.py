# -*- coding: utf-8 -*-
'''
MyServer
Copyright (C) 2009 Free Software Foundation, Inc.
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
'''

import gtk
import gobject
import gtk.glade
import GUIConfig

class PyGTKControl():
    def __init__(self):
        self.gladefile = 'PyGTKControl.glade'
        self.widgets = gtk.glade.XML(self.gladefile, 'window')
        self.widgets.signal_autoconnect(self)
        self.construct_options()

    def on_window_destroy(self, widget):
        gtk.main_quit()

    def construct_options(self):
        def make_tab_name(text):
            return text.capitalize().replace('.', ' ').replace('_', ' ')
        def make_name(text, tab):
            if tab == 'other':
                return text
            return text[len(tab):].replace('.', ' ').replace('_', ' ').strip()

        segregated_options = {}
        for option in GUIConfig.options:
            tab = 'other'
            for prefix in GUIConfig.tabs:
                if option.startswith(prefix):
                    tab = prefix
                    break
            if not segregated_options.has_key(tab):
                segregated_options[tab] = []
            segregated_options[tab].append(option)
                
        tabs = {}
        for tab in GUIConfig.tabs + ['other', 'unknown']:
            options = segregated_options.get(tab, [])
            tabs[tab] = gtk.Table(max(1, len(options)), 3)
            for i in xrange(len(options)):
                option = options[i]
                label = gtk.Label(make_name(option, tab))
                label.set_tooltip_text(GUIConfig.options[option][0])
                check = gtk.CheckButton()
                if GUIConfig.options[option][1] == 'string':
                    field = gtk.Entry()
                elif GUIConfig.options[option][1] == 'integer':
                    field = gtk.SpinButton()
                elif GUIConfig.options[option][1] == 'bool':
                    field = gtk.combo_box_new_text()
                    field.append_text('yes')
                    field.append_text('no')
                    field.set_active(0)
                elif GUIConfig.options[option][1] == 'combobox':
                    field = gtk.combo_box_new_text()
                elif GUIConfig.options[option][1] == 'list':
                    tree  = gtk.TreeView(gtk.ListStore(gobject.TYPE_STRING))
                    new_name = gtk.Entry()
                    add_button = gtk.Button('add')
                    remove_button = gtk.Button('remove')
                    field = gtk.Table(3, 2)
                    field.attach(new_name, 0, 2, 0, 1)
                    field.attach(add_button, 0, 1, 1, 2)
                    field.attach(remove_button, 1, 2, 1, 2)
                    field.attach(tree, 0, 2, 2, 3)
                tabs[tab].attach(check, 0, 1, i, i + 1, gtk.FILL, gtk.FILL)
                tabs[tab].attach(label, 1, 2, i, i + 1, yoptions = gtk.FILL)
                tabs[tab].attach(field, 2, 3, i, i + 1, yoptions = gtk.FILL)
            self.widgets.get_widget('notebook').append_page(
                tabs[tab], gtk.Label(make_tab_name(tab)))

        self.widgets.get_widget('notebook').show_all()

PyGTKControl()
gtk.main()
