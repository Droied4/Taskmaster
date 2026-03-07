local config = {}

config.programs = {
	nginx_server = {
		cmd = "/usr/bin/nginx -g 'daemon off;'",
		numprocs = 3,
		autostart = true,
		starttime = 10,
	},

	epic_program_xd = {
		cmd = "/bin/ls -la",
	},
}

config.programs.echo = {
	cmd = "/bin/echo 'Hello, World!'",
	numprocs = 1,
	autostart = true,
	starttime = 5,
	env = {
		tuqui = "asdf",
		taca = 123,
	},
}

config.programs["nginx"] = {
	cmd = "/usr/local/bin/nginx -c /etc/nginx/test.conf",
	numprocs = 1,
	umask = "022",
	workingdir = "/tmp",
	autostart = true,
	autorestart = "unexpected",
	exitcodes = { 0, 2 },
	startretries = 3,
	starttime = 5,
	stopsignal = "TERM",
	stoptime = 10,
	stdout = "/tmp/nginx.stdout",
	stderr = "/tmp/nginx.stderr",
	env = {
		STARTED_BY = "taskmaster",
		ANSWER = "42",
	},
}

config.programs["vogsphere"] = {
	cmd = "/usr/local/bin/vogsphere-worker --no-prefork",
	numprocs = 8,
	umask = "077",
	workingdir = "/tmp",
	autostart = true,
	autorestart = "unexpected",
	exitcodes = 0,
	startretries = 3,
	starttime = 5,
	stopsignal = "USR1",
	stoptime = 10,
	stdout = "/tmp/vgsworker.stdout",
	stderr = "/tmp/vgsworker.stderr",
}

config.programs["test"] = {
	cmd = "/tests",
	numprocs = 8,
	umask = "044",
	workingdir = "/tmp",
	autostart = true,
	autorestart = "unexpected",
	exitcodes = 0,
	startretries = 3,
	starttime = 5,
	stopsignal = "USR1",
	stoptime = 10,
	stdout = "/tmp/vgsworker.stdout",
	stderr = "/tmp/vgsworker.stderr",
}

return config
