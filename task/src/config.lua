local config = {}

config.programs = {}

-- config.programs["test"] = {
-- 	cmd = "/tests",
-- 	numprocs = 8,
-- 	umask = "044",
-- 	workingdir = "/tmp",
-- 	autostart = true,
-- 	autorestart = "unexpected",
-- 	exitcodes = 0,
-- 	startretries = 3,
-- 	starttime = 5,
-- 	stopsignal = "USR1",
-- 	stoptime = 10,
-- 	stdout = "/tmp/vgsworker.stdout",
-- 	stderr = "/tmp/vgsworker.stderr",
-- }

config.programs.cat = {
	cmd = "cat",
	autostart = true,
	numprocs = 1,
}

config.programs.urandom_cat = {
	cmd = "cat /dev/urandom",
	autostart = true,
	numprocs = 1,
	stdout = "/dev/null",
}

config.programs.echo = {
	cmd = "echo 'Hello, World!'",
	autostart = true,
	stderr = "/tmp/echo.stderr",
	stdout = "/tmp/echo.stdout",
	numprocs = 8,
}

config.programs.ls = {
	cmd = "ls",
	autostart = true,
	workingdir = "/tmp",
	autorestart = "always",
	startretries = 5,
	numprocs = 1,
	stdout = "/tmp/ls.stdout",
	stderr = "/tmp/ls.stderr",
}

config.programs.sleep = {
	cmd = "sleep 10",
	-- autostart = true,
	numprocs = 3,
	stoptime = 5,
	stopsignal = "SIGTERM",
}

-- config.programs.test = {
-- 	cmd = "./test.sh",
-- 	autostart = true,
-- 	stoptime = 4,
-- 	stopsignal = "SIGTERM",
-- }

config.programs.bash = {
	cmd = "bash",
	autostart = false,
	stopsignal = "SIGTERM",
	env = {
		TUQUI = "TUQUI",
		TERM = "xterm",
	},
}

config.programs.zsh = {
	cmd = "zsh",
	autostart = true,
	autorestart = "always",
	startretries = 5,
	stoptime = 5,
	stopsignal = "SIGTERM",
	env = {
		TUQUI = "TUQUI",
		TERM = "xterm",
	},
}

config.programs.spam_test = {
	cmd = "yes spam",
	autostart = true,
	numprocs = 1,
}

return config
