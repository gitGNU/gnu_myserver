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

from lxml import etree
from log import Log

class VHost():
    valid_protocols = set(['HTTP', 'FTP', 'HTTPS', 'ISAPI', 'CGI', 'SCGI',
                           'FastCGI', 'WinCGI'])
    
    def __init__(self, name, port, protocol, doc_root, sys_folder, access_log,
                 warning_log, ip = [], host = {}):
        self.set_name(name)
        self.set_port(port)
        self.set_protocol(protocol)
        self.set_doc_root(doc_root)
        self.set_sys_folder(sys_folder)
        self.set_access_log(access_log)
        self.set_warning_log(warning_log)
        self.ip = set()
        for ip_address in ip:
            self.add_ip(ip_address)
        self.host = {}
        for single_host in host.iteritems():
            self.add_host(single_host[0], single_host[1])

    def __eq__(self, other):
        return isinstance(other, VHost) and self.name == other.name and \
            self.port == other.port and self.protocol == other.protocol and \
            self.doc_root == other.doc_root and \
            self.sys_folder == other.sys_folder and \
            self.access_log == other.access_log and \
            self.warning_log == other.warning_log and self.ip == other.ip and \
            self.host == other.host

    def get_name(self):
        '''Get VHost name.'''
        return self.name

    def set_name(self, name):
        '''Set VHost name.'''
        if name is None:
            raise AttributeError('name is required and can\'t be None')
        self.name = name

    def get_doc_root(self):
        '''Get VHost doc root.'''
        return self.doc_root

    def set_doc_root(self, doc_root):
        '''Set VHost doc root.'''
        if doc_root is None:
            raise AttributeError('doc_root is required and can\'t be None')
        self.doc_root = doc_root

    def get_sys_folder(self):
        '''Get VHost sys folder.'''
        return self.sys_folder

    def set_sys_folder(self, sys_folder):
        '''Set VHost sys folder.'''
        if sys_folder is None:
            raise AttributeError('sys_folder is required and can\'t be None')
        self.sys_folder = sys_folder

    def get_protocol(self):
        '''Get VHost protocol.'''
        return self.protocol

    def set_protocol(self, protocol):
        '''Set VHost protocol.'''
        if protocol not in self.valid_protocols:
            raise AttributeError('protocol is not valid')
        if protocol is None:
            raise AttributeError('protocol is required and can\'t be None')
        self.protocol = protocol

    def get_port(self):
        '''Get VHost port.'''
        return self.port

    def set_port(self, port):
        '''Set VHost port.'''
        if port is None:
            raise AttributeError('port is required and can\'t be None')
        self.port = port

    def get_access_log(self):
        '''Get VHost access log.'''
        return self.access_log

    def set_access_log(self, access_log):
        '''Set VHost access log.'''
        if access_log is None:
            raise AttributeError('access_log is required and can\'t be None')
        if not isinstance(access_log, Log) or \
                access_log.get_log_type() != 'ACCESSLOG':
            raise AttributeError('given attribute is not an access log')
        self.access_log = access_log

    def get_warning_log(self):
        '''Get VHost warning_log.'''
        return self.warning_log

    def set_warning_log(self, warning_log):
        '''Set VHost warning_log.'''
        if warning_log is None:
            raise AttributeError('warning_log is required and can\'t be None')
        if not isinstance(warning_log, Log) or \
                warning_log.get_log_type() != 'WARNINGLOG':
            raise AttributeError('given attribute is not a warning log')
        self.warning_log = warning_log

    def get_ip(self):
        '''Get VHost ip set.'''
        return self.ip

    def add_ip(self, ip):
        '''Add ip to VHost ip set.'''
        self.ip.add(ip)

    def remove_ip(self, ip):
        '''Remove ip from VHost ip set.'''
        self.ip.remove(ip)

    def get_host(self):
        '''Get VHost host dict.'''
        return self.host

    def add_host(self, host, use_regex = None):
        '''Add host to VHost dict.'''
        self.host[host] = use_regex

    def remove_host(self, host):
        '''Remove host from VHost host dict.'''
        self.host.pop(host)
