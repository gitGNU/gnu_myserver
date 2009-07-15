options = {}

# Control
options['control.enabled'] = ('Set this to yes if you want to enable control protocol.', 'bool', )
options['control.admin'] = ('Set here the name for the control admin.', 'string', )
options['control.password'] = ('Define here a good, strong password for the admin.', 'string', )

# FTP
options['ftp.allow_anonymous'] = ('Allow anonymous login.', 'bool', )
options['ftp.anonymous_need_pass'] = ('If anonymous allowed, tells if needs pass.', 'bool', )
options['ftp.allow_asynchronous_cmds'] = ('Allow asynchronous cmds like Abor, Quit, Stat.', 'bool', )
options['ftp.allow_pipelining'] = ('Allow clients to send more than one command to server in one request.', 'bool', )
options['ftp.allow_store'] = ('Allow clients to change server files.', 'bool', )

# HTTP
options['http.use_error_file'] = ('Set this to yes to use personalized error pages from system directory.', 'bool', )
options['http.dir.css'] = ('Define the folder browsing style.', 'string', )
options['http.use_home_directory'] = ('Enable home directory per user', 'bool', )
options['http.home_directory'] = ('Define name for home directory browsing.', 'string', )
options['http.default_file'] = ('Default filename to send in a directory if the file isn\'t in the path then the directory content is sent.', 'list', )

# Server
options['server.language'] = ('Choose the language to use for the server.', 'combobox', )
options['server.verbosity'] = ('Verbosity on log file.', 'integer', )
options['server.static_threads'] = ('Number of serving threads always active.', 'integer', )
options['server.max_threads'] = ('Maximum number of serving threads that the scheduler can create.', 'integer', )
options['server.buffer_size'] = ('Dimension of every buffer in bytes.', 'integer', )
options['server.max_connections'] = ('Define the max number of connections to allow to server. 0 means allow infinite connections.', 'integer', )
options['server.max_accepted_connections'] = ('Define max number of connections to accept.', 'integer', )
options['server.max_log_size'] = ('max size of the log file in bytes.', 'integer', )
options['server.admin'] = ('Administrator e-mail.', 'string', )
options['server.max_files_cache'] = ('Max cache size for static files.', 'integer', )
options['server.max_file_cache'] = ('Max cache size for single static file.', 'integer', )
options['server.min_file_cache'] = ('Min cache size for single static file.', 'integer', )
options['server.max_servers'] = ('Maximum number of external servers that can be executed.', 'integer', )

# Log color
options['log_color.info_fg'] = ('Info log foreground colour.', 'string', )
options['log_color.info_bg'] = ('Info log background colour.', 'string', )
options['log_color.warning_fg'] = ('Warning log foreground colour.', 'string', )
options['log_color.warning_bg'] = ('Warning log background colour.', 'string', )
options['log_color.error_fg'] = ('Error log foreground colour.', 'string', )
options['log_color.error_bg'] = ('Error log background colour.', 'string', )

# Other
options['connection.timeout'] = ('Timeout for every client\'s connected to server. If the client doesn\'t request anything for n seconds the connection with the client is closed. Set this to 0 if you don\'t want to use keep-alive-connections', 'integer', )
options['gzip.threshold'] = ('Define the gzip compression threshold value.', 'integer', )
options['symlinks.follow'] = ('Define if links should be followed.', 'bool', )

# don't put 'other' or 'unknown' here
tabs = ['server', 'control', 'ftp', 'http', 'log_color']
